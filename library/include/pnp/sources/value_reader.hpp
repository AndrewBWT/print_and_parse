#pragma once
#include "pnp/core/concepts.hpp"
#include "pnp/core/parse/default_parser.hpp"
#include "pnp/core/parse/parser_input.hpp"
#include "pnp/core/parse/parser_result.hpp"
#include "pnp/core/parse/text_source.hpp"
#include "pnp/internal/macros.hpp"
#include "pnp/support/internal/utility.hpp" // quoted, valid_utf8, no_surrounding_whitespace

#include <optional>
#include <stdexcept>
#include <string>
#include <unicode_bridge.hpp>
#include <utility>

PNP_NS_BEGIN

// Reads a series of values from a source, one at a time: one value per line,
// several on a line, or separated by a token. Each value is available as soon
// as its text has arrived, and its text is released from the source once it's
// parsed, so memory stays proportional to one value however long the input is.
//
//   pnp::line_source          source(file);
//   pnp::value_reader<record> reader(source);
//   while (const auto result = reader.next())
//   {
//       if (not succeeded(*result)) { report(*result); break; }
//       process(result->_value);
//   }
//
// The source must outlive the reader.
template <typename T, typename Parser = default_parser<T>>
requires parser_c<Parser, T> && parse_constructible_c<T>
struct value_reader
{
public:
    // Throws std::invalid_argument if the options are invalid.
    explicit value_reader(text_source& source_arg, Parser parser_arg = {});

    // The sole reader of its source: copying would leave two readers drawing
    // from one source. 
    value_reader(const value_reader&) = delete;
    value_reader&
        operator=(const value_reader&)
        = delete;
    value_reader(value_reader&&) noexcept = default;
    value_reader&
        operator=(value_reader&&) noexcept
        = default;

    ~value_reader() = default;

    // The next value, or a failure, with positions in the whole input;
    // std::nullopt once there are no more values. After a failure, the reader
    // stops: every later call returns std::nullopt, since where the next value
    // would start can't be known.
    [[nodiscard]]
    std::optional<parse_outcome_with_value<T>>
        next();

    // Whether the reader has finished, at the end of the input or after a
    // failure.
    [[nodiscard]]
    bool
        finished() const noexcept;
private:
    text_source* _source;
    Parser           _parser;
    bool             _finished = false;
};

PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN

template <typename T, typename Parser>
requires parser_c<Parser, T> && parse_constructible_c<T>
value_reader<T, Parser>::value_reader(
    text_source& source_arg,
    Parser           parser_arg
)
    : _source(&source_arg), _parser(std::move(parser_arg))
{}

template <typename T, typename Parser>
requires parser_c<Parser, T> && parse_constructible_c<T>
bool
    value_reader<T, Parser>::finished() const noexcept
{
    return _finished;
}

template <typename T, typename Parser>
requires parser_c<Parser, T> && parse_constructible_c<T>
std::optional<parse_outcome_with_value<T>>
    value_reader<T, Parser>::next()
{
    using namespace std;
    using enum parse_outcome::parse_outcome_state_enum;
    if (_finished)
    {
        return std::nullopt;
    }

    parse_outcome_with_value<T> result;
    auto&                       state = result.outcome;
    auto fail = [&](const basic_parser_return_value& failure_arg
                ) -> std::optional<parse_outcome_with_value<T>>
    {
        state.position              = failure_arg.position();
        state.parser_error_messages = failure_arg.messages();
        state.parse_outcome_state   = failure_parser_error;
        _finished                   = true;
        return std::move(result);
    };
    {
        parser_input input(*_source);
        try
        {
            // 1. Whatever separates this value from the last. Asking whether
            // the input has ended
            //    may wait for more, which is expected: the caller has asked for
            //    another value.
            input.skip_whitespace();
            if (input.at_end())
            {
                _finished = true;
                return std::nullopt;
            }

            // 2. The value.
            if (const basic_parser_return_value parsed
                = input.run_parser(_parser, result.value);
                parsed.failed())
            {
                return fail(parsed);
            }

            // 3. Straight after it: whitespace, the separator, or the end. Only
            // one character is
            //    looked at, so on interactive input, a value is returned as
            //    soon as the character after it arrives, without waiting for
            //    the next value.
            if (const optional<pair<char32_t, size_t>> after
                = input.peek_char();
                after.has_value()
                && not unicode_bridge::is_whitespace(after.value().first))
            {
                return fail(
                    input.fail(std::u8string(u8"Expected whitespace or the end "
                                             u8"of the input after the value."))
                );
            }

            state.parse_outcome_state = success_and_all_input_consumed;
            state.position            = input.position();
        }
        catch (const parse_exception& exception)
        {
            state.position            = exception.error().position();
            state.error               = exception.error().details();
            state.parse_outcome_state = failure_parser_exception;
            _finished                 = true;
            return result;
        }
    }
    // Reports a failure, and stops the reader.


    // 4. Releases the value's text from the source. Nothing reads from input
    // after this, so the
    //    positions it recorded can't go stale.
    _source->release_up_to(state.position);
    return result;
}

PNP_NS_END