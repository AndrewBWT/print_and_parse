#pragma once

#include "pnp/core/concepts.hpp"
#include "pnp/core/parse/default_parser.hpp"
#include "pnp/core/parse/parse_outcome.hpp"
#include "pnp/core/parse/parser_input.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

PNP_NS_BEGIN
/*!
 * @brief A concept that checks some arbitrary type Input can be used to
 * construct a parser_input object. For example, strings, or sources of text
 * (e.g. through files or std::cin).
 * @tparam Input The type to construct parser_input from.
 */
template <typename Input>
concept parse_input_c = std::constructible_from<parser_input, Input&>;

/*!
 * The following should be noted about each function:
 * - They all parse the whole input as a single value.
 * - Whitespace is skipped before and after parsing. See parser_input for
 * details on what is whitespace.
 * - The functions throw exceptions in very specific circumstances. Invalid
 * UTF-8, and a parser asking for something impossible (an invalid_parser_
 * code), are caught and reported as a failed parse, not thrown as exceptions.
 * The following exceptions can be thrown:
 *   - The constructor for parser_input can throw a std::logic_error if the
 * source is already being read by a parser_input object.
 *   - The constructor for parser_input can throw a std::invalid_argument
 * exception if fraction_of_buffer_threshold is outside 0.05 to 0.95.
 *   - std::logic_errors can be thrown by several functions in parser_input.
 * These errors are due to inconsistent internal logic, and should not happen.
 * If they are found, please report them.
 *   - Other types of exception regarding bad IO, bad memory allocation, etc.
 * - Every function returns one of two types; the first is a simple flat result
 * type. The second is a templated version of the first with the (potentially)
 * parsed value. See pnp/core/parse/parse_outcome.hpp for details on the flat
 * return type.
 * - For sources, on success the value and whitespace after the parsed value are
 * discarded, and the next parse carries on from there.
 * - For sources, on failure nothing is discarded from the input. However, as
 * the parser was running, text may have been discarded already. Therefore, it
 * is not possible to just start again with the same source.
 * - For a source that may wait for input, such as std::cin, these functions can
 * block - specifically when checking for the end of the input text.
 */
template <typename T, typename Input>
requires parser_c<default_parser<T>, T> && parse_input_c<Input>
constexpr parse_outcome
    parse(Input&& input_arg, T& object_arg);
/*!
 * @brief The core free function that provides parsing functionality.
 *
 * It runs the parser_input object created from input_arg through the parser
 * parser_arg. parser_arg should only override the value of object_arg if it
 * successfully parsed the input.
 *
 * @tparam Parser The type of the parser.
 * @tparam T The type to be parsed.
 * @tparam Input The input type to construct parser_input from.
 * @param input_arg The input.
 * @param object_arg A reference to the object to be overridden by the parser.
 * @param parser_arg The parser. Defaults to {}.
 * @return A parse_outcome object describing how the parse went.
 */
template <typename Parser, typename T, typename Input>
requires parser_c<Parser, T> && parse_input_c<Input>
constexpr parse_outcome
    parse(
        Input&&       input_arg,
        T&            object_arg,
        const Parser& parser_arg = Parser{}
    );
/*!
 * @brief Runs the parser_input object created from input_arg through
 * the Parser parser_arg. The return value will contain a
 * make_placeholder<T>() created object of type T if parser_arg fails.
 *
 * See make_placeholder and parse_outcome_with_value for C arrays.
 *
 * @tparam T The type to be parsed.
 * @tparam Parser The Parser type used. Defaults to default_parser<T>.
 * @tparam Input The input type to construct parser_input from.
 * @param input_arg The input.
 * @param parser_arg The parser. Defaults to Parser{}.
 * @return A parse_outcome_with_value object describing how the parse went
 * containing the parsed value or its placeholder.
 */
template <typename T, typename Parser = default_parser<T>, typename Input>
requires parser_c<Parser, T> && parse_constructible_c<T> && parse_input_c<Input>
constexpr parse_outcome_with_value<T>
    try_parse(Input&& input_arg, const Parser& parser_arg = Parser{});

PNP_NS_END

// Definitions.

PNP_NS_BEGIN
template <typename T, typename Input>
requires parser_c<default_parser<T>, T> && parse_input_c<Input>
constexpr parse_outcome
    parse(
        Input&& input_arg,
        T&      object_arg
    )
{
    return parse(input_arg, object_arg, default_parser<T>{});
}

template <typename Parser, typename T, typename Input>
requires parser_c<Parser, T> && parse_input_c<Input>
constexpr parse_outcome
    parse(
        Input&&       input_arg,
        T&            object_arg,
        const Parser& parser_arg
    )
{
    using namespace std;
    using enum parse_outcome::parse_outcome_state_enum;
    parse_outcome return_value;
    {
        // parser_input in its own scope so it's destroyed, freeing the
        // source, before discard is called.
        parser_input input(input_arg);
        try
        {
            // Skip whitespace at the start of the input.
            input.skip_whitespace();
            // Run the parser; if it fails, return the error. It is assumed a
            // failing parser does not change object_arg.
            if (basic_parser_return_value result
                = input.run_parser(parser_arg, object_arg);
                result.failed())
            {
                return_value.position            = result.position();
                return_value.parse_outcome_state = failure_parser_error;
                return_value.parser_error_messages
                    = std::move(result).messages();
                return_value.parsers_error_context
                    = input.get_context(result.position());
                return return_value;
            }
            // If successful, skip additional whitespace at the end. This is so
            // that things like "5 " do not state that there is input remaining.
            input.skip_whitespace();
            return_value.position = input.position();
            return_value.parse_outcome_state
                = input.at_end() ? success_and_all_input_consumed
                                 : success_but_input_remains;
        }
        catch (const parse_exception& error)
        {
            // Some parsing functionality can throw exceptions - e.g. if invalid
            // UTF8 is read. This branch captures that.
            auto& error_obj                  = error.error();
            return_value.position            = error_obj.position();
            return_value.error               = error_obj.details();
            return_value.parse_outcome_state = failure_parser_exception;
            return_value.parsers_error_context
                = input.get_context(error_obj.position());
        }
    }
    // If the source is an text_source.
    if constexpr (derived_from<remove_cvref_t<Input>, text_source>)
    {
        if (not return_value.failed())
        {
            input_arg.release_up_to(return_value.position);
        }
    }
    return return_value;
}


template <typename T, typename Parser, typename Input>
requires parser_c<Parser, T> && parse_constructible_c<T> && parse_input_c<Input>
constexpr parse_outcome_with_value<T>
    try_parse(
        Input&&       input_arg,
        const Parser& parser_arg
    )
{
    parse_outcome_with_value<T> return_value;
    return_value.outcome
        = parse(std::forward<Input>(input_arg), return_value.value, parser_arg);
    return return_value;
}

PNP_NS_END