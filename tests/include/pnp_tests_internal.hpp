#pragma once
#include "pnp/sources.hpp"
#include "pnp/sources/testing/piece_source.hpp"
#include "pnp/sources/string_source.hpp"
#include "pnp/sources/value_reader.hpp"
#include "pnp_tests_internal.hpp"

#include <catch2/catch_test_macros.hpp>

namespace pnp_tests
{
using error_code = pnp::parse_result_error::error_enum;

template <typename F>
std::optional<pnp::parse_result_error>
    thrown_error(
        F&& function_arg
    )
{
    try
    {
        std::forward<F>(function_arg)();
    }
    catch (const pnp::parse_exception& exception)
    {
        return exception.error();
    }
    return std::nullopt;
}

// The code of the parse_exception thrown by action_arg, or std::nullopt if it
// throws none.
inline std::optional<error_code>
    thrown_code(
        const std::function<void()>& action_arg
    )
{
    auto res = thrown_error(action_arg);
    return res.has_value()
               ? std::make_optional(res.value().details().error_code)
               : std::nullopt;
}

// The position of the parse_exception thrown by action_arg.
inline std::optional<std::size_t>
    thrown_position(
        const std::function<void()>& action_arg
    )
{
    auto res = thrown_error(action_arg);
    return res.has_value() ? std::make_optional(res.value().position())
                           : std::nullopt;
}

// One code unit, as a string: for building text that isn't valid UTF-8, or
// that splits a character across pieces.
inline std::u8string
    byte(
        const unsigned char byte_arg
    )
{
    return std::u8string(1, static_cast<char8_t>(byte_arg));
}
inline std::u8string
bytes(const std::string_view text_arg)
{
    return std::u8string(text_arg.begin(), text_arg.end());
}

inline constexpr auto is_digit = [](const char8_t c)
{
    return c >= u8'0' && c <= u8'9';
};

// A parser that moves over text it never looked at: a bug, reported as a
// parse_exception.
struct overreaching_parser
{
    constexpr pnp::basic_parser_return_value
        run_parser(
            pnp::parser_input& input_arg,
            int&,
            pnp::parser_run_key
        ) const
    {
        input_arg.advance(100);
        return {};
    }
};

// A parser that throws something other than a parse_exception: passes through.
struct broken_parser
{
    pnp::basic_parser_return_value
        run_parser(
            pnp::parser_input&,
            int&,
            pnp::parser_run_key
        ) const
    {
        throw std::runtime_error("broken");
    }
};

// Reads a run of digits and the whitespace after it; value_arg is how many
// digits there were.
struct digits_parser
{
    pnp::basic_parser_return_value
        run_parser(
            pnp::parser_input& input_arg,
            int&               value_arg,
            pnp::parser_run_key
        ) const
    {
        const auto digits = input_arg.peek_while(is_digit);
        if (digits.get().empty())
        {
            return input_arg.fail(u8"expected digits");
        }
        value_arg = static_cast<int>(digits.get().size());
        input_arg.advance(digits.get().size());
        input_arg.skip_whitespace();
        return {};
    }
};

// Reads 2'000 "12345 " elements, then fails.
struct consume_then_fail
{
    pnp::basic_parser_return_value
        run_parser(
            pnp::parser_input& input_arg,
            int&,
            pnp::parser_run_key
        ) const
    {
        for (int i = 0; i < 2'000; ++i)
        {
            const auto digits = input_arg.peek_while(is_digit);
            input_arg.advance(digits.get().size());
            input_arg.skip_whitespace();
        }
        return input_arg.fail(u8"failed after reading");
    }
};

// A source that reports success twice without adding anything, then hands over
// its text.
struct stalling_source : public pnp::buffered_text_source
{
    inline stalling_source()
        : pnp::buffered_text_source(false)
    {}

    int           stalls = 2;
    std::u8string text   = u8"ok";
protected:
    read_result
        read_piece(const std::span<char8_t> into_arg) override
    {
        if (stalls > 0)
        {
            --stalls;
            return read_result{ .code_units_written = 0, .more_content = true };
        }
        if (text.empty())
        {
            return read_result{ .code_units_written = 0, .more_content = false };
        }
        const std::size_t count = std::min(text.size(), into_arg.size());
        std::char_traits<char8_t>::copy(into_arg.data(), text.data(), count);
        text.erase(0, count);
        return read_result{ .code_units_written = count, .more_content = true };
    }
};

// Hands over the same piece count_arg times, without waiting.
struct repeating_source : public pnp::buffered_text_source
{
    repeating_source(
        std::u8string     piece_arg,
        const std::size_t count_arg
    )
        : pnp::buffered_text_source(true)
        , piece(std::move(piece_arg))
        , remaining(count_arg)
    {}

    std::u8string piece;
    std::size_t   remaining;
protected:
    read_result
        read_piece(const std::span<char8_t> into_arg) override
    {
        if (remaining == 0)
        {
            return read_result{ .code_units_written = 0, .more_content = false };
        }
        const std::size_t count
            = std::min(piece.size() - piece_offset, into_arg.size());
        std::char_traits<char8_t>::copy(
            into_arg.data(), piece.data() + piece_offset, count
        );
        piece_offset += count;
        if (piece_offset == piece.size())
        {
            piece_offset = 0;
            --remaining;
        }
        return read_result{ .code_units_written = count, .more_content = true };
    }

    std::size_t piece_offset = 0;
};

// A source that may wait, like std::cin, and hands over only the pieces it's
// given. Reading any further throws: a test fails if a function reads more
// than it needs to decide.
struct trap_source : public pnp::buffered_text_source
{
    explicit trap_source(
        std::vector<std::u8string> pieces_arg
    )
        : pnp::buffered_text_source(false), pieces(std::move(pieces_arg))
    {}

    std::vector<std::u8string> pieces;
    std::size_t                next = 0;
protected:
    read_result
        read_piece(const std::span<char8_t> into_arg) override
    {
        if (next == pieces.size())
        {
            throw std::logic_error(
                "trap_source: read further than the test allows"
            );
        }
        const std::u8string& current = pieces[next];
        const std::size_t    count
            = std::min(current.size() - piece_offset, into_arg.size());
        std::char_traits<char8_t>::copy(
            into_arg.data(), current.data() + piece_offset, count
        );
        piece_offset += count;
        if (piece_offset == current.size())
        {
            piece_offset = 0;
            ++next;
        }
        return read_result{ .code_units_written = count, .more_content = true };
    }

    std::size_t piece_offset = 0;
};

// Reads one "12345" and the whitespace after it, then releases what's done.
// Returns how much text the source held afterwards.
inline std::size_t
    read_one(
        pnp::parser_input&      input_arg,
        const pnp::text_source& source_arg
    )
{
    {
        const auto digits = input_arg.peek_while(is_digit);
        REQUIRE(digits.get() == u8"12345");
        input_arg.advance(digits.get().size());
    } // digits' hold is gone before releasing
    input_arg.skip_whitespace();
    input_arg.release_consumed();
    return source_arg.buffered().size();
}
} // namespace pnp_tests

namespace pnp_tests
{
inline bool
    succeeded(
        const pnp::parse_outcome& result_arg
    )
{
    return result_arg.succeeded();
}

template <typename T>
bool
    succeeded(
        const pnp::parse_outcome_with_value<T>& result_arg
    )
{
    return succeeded(result_arg.outcome);
}

inline bool
    succeeded_or_remains(
        const pnp::parse_outcome& outcome_arg
    )
{
    using enum pnp::parse_outcome::parse_outcome_state_enum;
    return outcome_arg.parse_outcome_state == success_and_all_input_consumed
           || outcome_arg.parse_outcome_state == success_but_input_remains;
}

template <typename T>
inline bool
    succeeded_or_remains(
        const pnp::parse_outcome_with_value<T>& outcome_arg
    )
{
    return succeeded_or_remains(outcome_arg.outcome);
}

inline std::u8string
    code_unit(
        const unsigned char unit_arg
    )
{
    return std::u8string(1, static_cast<char8_t>(unit_arg));
}

template <typename T>
inline std::u8string
    first_message(
        const pnp::parse_outcome_with_value<T>& error_arg
    )
{
    if (error_arg.outcome.parser_error_messages.size() > 0)
    {
        return error_arg.outcome.parser_error_messages[0];
    }
    else
    {
        return std::u8string{};
    }
}

inline const std::u8string half_a_character(1, static_cast<char8_t>(0xC3));

// Runs a construction that should throw std::invalid_argument, and returns its
// message.
template <typename Make>
std::string
    invalid_argument_message(
        Make&& make_arg
    )
{
    try
    {
        make_arg();
    }
    catch (const std::invalid_argument& error)
    {
        return error.what();
    }
    return "<no std::invalid_argument thrown>";
}
} // namespace pnp_tests

namespace pnp_tests
{
// The piece sizes each input is split into: one byte splits it everywhere, and
// a few larger sizes split it at different places relative to its tokens.
inline constexpr std::array<std::size_t, 4> piece_sizes{1, 2, 3, 5};

// Parses text_arg whole, then from sources handing it over in pieces of each
// size, and checks every parse gives the same result: the same state, at the
// same position, with the same value.
template <typename Parser, typename T>
void
    check_parses_in_pieces_with(
        const Parser&            parser_arg,
        const std::u8string_view text_arg
    )
{
    const auto whole = pnp::try_parse<T>(text_arg, parser_arg);
    for (const std::size_t piece_size : piece_sizes)
    {
        INFO("piece size " << piece_size);
        pnp::testing::piece_source source(std::u8string(text_arg), piece_size);
        const auto pieces = pnp::try_parse<T>(source, parser_arg);
        CHECK(
            pieces.outcome.parse_outcome_state
            == whole.outcome.parse_outcome_state
        );
        CHECK(pieces.outcome.position == whole.outcome.position);
        if (succeeded(whole))
        {
            CHECK(pieces.value == whole.value);
        }
    }
}

template <typename T>
void
    check_parses_in_pieces(
        const std::u8string_view text_arg
    )
{
    check_parses_in_pieces_with<pnp::default_parser<T>, T>(
        pnp::default_parser<T>{}, text_arg
    );
}

// Prints value_arg, then checks that parsing it back in pieces gives the value
// back.
template <typename T>
void
    check_round_trip_in_pieces(
        const T& value_arg
    )
{
    const std::u8string text = pnp::to_u8string(value_arg);
    INFO("text " << std::string(text.begin(), text.end()));
    REQUIRE(succeeded(pnp::try_parse<T>(text)));
    check_parses_in_pieces<T>(text);
    for (const std::size_t piece_size : piece_sizes)
    {
        pnp::testing::piece_source source(text, piece_size);
        const auto        parsed = pnp::try_parse<T>(source);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value_arg);
    }
}

// Parses all of text_arg from a stream, a line at a time.
template <typename Parser, typename T>
pnp::parse_outcome_with_value<T>
    parse_lines_with(
        const Parser&      parser_arg,
        const std::string& text_arg
    )
{
    std::istringstream  stream(text_arg);
    // A minimum release of 1, and a minimum block size of 1.
    pnp::istream_source source(stream, true, 1, 1);
    return pnp::try_parse<T>(source, parser_arg);
}
} // namespace pnp_tests

namespace pnp_tests
{
// Everything a reader produced: the values it returned, then the failure it
// stopped at, if any.
template <typename T>
struct read_outcome
{
    std::vector<T>                    values;
    std::optional<pnp::parse_outcome> failure;
};

// Reads values until the reader finishes.
template <typename T, typename Parser = pnp::default_parser<T>>
read_outcome<T>
    read_all(
        pnp::text_source& source_arg,
        Parser            parser_arg = {}
    )
{
    pnp::value_reader<T, Parser> reader(source_arg, std::move(parser_arg));
    read_outcome<T>              outcome;
    while (const auto result = reader.next())
    {
        if (not succeeded(*result))
        {
            outcome.failure = result->outcome;
            break;
        }
        outcome.values.push_back(result->value);
    }
    CHECK(reader.finished());
    CHECK(not reader.next().has_value()); // and stays finished
    return outcome;
}

// The same, from text.
template <typename T, typename Parser = pnp::default_parser<T>>
read_outcome<T>
    read_all_from(
        const std::u8string_view text_arg,
        Parser                   parser_arg = {}
    )
{
    pnp::string_view_source source(text_arg);
    return read_all<T, Parser>(source, std::move(parser_arg));
}

// Hands over a fixed list of pieces, counting how many it's been asked for.
struct counting_source : public pnp::buffered_text_source
{
    explicit counting_source(
        std::vector<std::u8string> pieces_arg
    )
        : pnp::buffered_text_source(false), pieces(std::move(pieces_arg))
    {}

    std::vector<std::u8string> pieces;
    std::size_t                given = 0;
protected:
    read_result
        read_piece(const std::span<char8_t> into_arg) override
    {
        if (given >= pieces.size())
        {
            return read_result{ .code_units_written = 0, .more_content = false };
        }
        const std::u8string& current = pieces[given];
        const std::size_t    count
            = std::min(current.size() - piece_offset, into_arg.size());
        std::char_traits<char8_t>::copy(
            into_arg.data(), current.data() + piece_offset, count
        );
        piece_offset += count;
        if (piece_offset == current.size())
        {
            piece_offset = 0;
            ++given;
        }
        return read_result{ .code_units_written = count, .more_content = true };
    }

    std::size_t piece_offset = 0;
};
} // namespace pnp_tests

namespace pnp_tests
{
// What parsing one value from pieces did: whether it succeeded, the value, and
// how many pieces it asked for.
template <typename T>
struct read_ahead_outcome
{
    bool        succeeded = false;
    T           value{};
    std::size_t pieces_read = 0;
};

// Parses one value from the pieces, as if typed, with T's default parser.
template <typename T>
read_ahead_outcome<T>
    parse_from_pieces(
        std::vector<std::u8string> pieces_arg
    )
{
    counting_source       source(std::move(pieces_arg));
    pnp::parser_input     input(source);
    read_ahead_outcome<T> outcome;
    outcome.succeeded
        = input.run_parser(pnp::default_parser<T>{}, outcome.value).succeeded();
    outcome.pieces_read = source.given;
    return outcome;
}
} // namespace pnp_tests