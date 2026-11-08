#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/utility.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <unicode_bridge.hpp>
#include <utility>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN
PNP_INTERNAL_NS_BEGIN

// For sum types: values that are one of several alternatives, each written
// between its own markers, so the parser can tell which alternative follows
// from the first characters. An optional is nullopt or {5}; an expected is
// expected(1) or unexpected("e"). An alternative with nothing to print (an
// empty optional) is just its opening marker.

// One alternative's text, and how to describe it in error messages.
struct alternative_text
{
    std::u8string_view marker_description; // "opening delimiter", "null token"
    std::u8string_view open_delimiter;     // "expected("
    std::u8string_view close_delimiter;    // ")"
    std::u8string_view value_description;  // "the value of an expected"; empty
                                           // when there's no value
};

// Whether the parser can always tell the alternatives apart: the non-empty
// opening markers are pairwise prefix-free, and at most one alternative has an
// empty one. That alternative is the fallback, read when no other marker
// matches, so its own text must never start with another marker; that can't be
// checked, so each type that allows it documents it.
template <std::size_t N>
[[nodiscard]]
constexpr bool
    alternatives_distinguishable(
        const std::array<std::u8string_view, N>& open_delimiters_arg
    ) noexcept;

// Prints one alternative: its opening marker, its value (if it has one), its
// closing marker.
template <typename Print_Value>
constexpr void
    print_alternative(
        const alternative_text& text_arg,
        Print_Value&&           print_value_arg,
        text_sink&              sink_obj_arg
    );

// Parses whichever alternative the input starts with. On success, chosen_arg is
// its index. parse_value_arg(index, input) parses that alternative's value and
// returns the result; for an alternative with no value it should just succeed.
// Its failure is returned with the value's description added.
template <std::size_t N, typename Parse_Value>
constexpr basic_parser_return_value
    parse_alternatives(
        const std::array<alternative_text, N>& alternatives_arg,
        parser_input&                          parser_input_arg,
        std::size_t&                           chosen_arg,
        Parse_Value&&                          parse_value_arg
    );

// ============================================================ definitions

template <std::size_t N>
constexpr bool
    alternatives_distinguishable(
        const std::array<std::u8string_view, N>& open_delimiters_arg
    ) noexcept
{
    std::size_t empty_count = 0;
    for (std::size_t first = 0; first < N; ++first)
    {
        if (open_delimiters_arg[first].empty())
        {
            ++empty_count;
            continue;
        }
        for (std::size_t second = first + 1; second < N; ++second)
        {
            const std::u8string_view a = open_delimiters_arg[first];
            const std::u8string_view b = open_delimiters_arg[second];
            if (not b.empty() && (a.starts_with(b) || b.starts_with(a)))
            {
                return false;
            }
        }
    }
    return empty_count <= 1;
}

template <typename Print_Value>
constexpr void
    print_alternative(
        const alternative_text& text_arg,
        Print_Value&&           print_value_arg,
        text_sink&              sink_obj_arg
    )
{
    sink_obj_arg.write(text_arg.open_delimiter);
    std::forward<Print_Value>(print_value_arg)();
    sink_obj_arg.write(text_arg.close_delimiter);
}

template <std::size_t N, typename Parse_Value>
constexpr basic_parser_return_value
    parse_alternatives(
        const std::array<alternative_text, N>& alternatives_arg,
        parser_input&                          parser_input_arg,
        std::size_t&                           chosen_arg,
        Parse_Value&&                          parse_value_arg
    )
{
    // The alternative whose marker the input starts with; the markers are
    // validated to be prefix-free, so at most one can match. Failing that, the
    // one with no marker, if any.
    constexpr std::size_t none     = N;
    std::size_t           chosen   = none;
    std::size_t           fallback = none;
    for (std::size_t idx = 0; idx < N; ++idx)
    {
        if (alternatives_arg[idx].open_delimiter.empty())
        {
            fallback = idx;
        }
        else if (parser_input_arg.advance_if_starts_with(
                     alternatives_arg[idx].open_delimiter
                 ))
        {
            chosen = idx;
            break;
        }
    }
    if (chosen == none)
    {
        chosen = fallback;
    }

    if (chosen == none)
    {
        // "Expected the null token "nullptr" or the opening delimiter "{"."
        std::u8string message = u8"Expected ";
        for (std::size_t idx = 0; idx < N; ++idx)
        {
            if (idx != 0)
            {
                message += (idx + 1 == N) ? u8" or " : u8", ";
            }
            message += u8"the "
                       + std::u8string(alternatives_arg[idx].marker_description)
                       + u8" " + quoted(alternatives_arg[idx].open_delimiter);
        }
        message += u8".";
        return parser_input_arg.fail(std::move(message));
    }

    const alternative_text& text = alternatives_arg[chosen];
    if (auto value_result
        = std::forward<Parse_Value>(parse_value_arg)(chosen, parser_input_arg);
        value_result.failed())
    {
        if (not text.value_description.empty())
        {
            value_result.add_msg(
                u8"while parsing " + std::u8string(text.value_description)
            );
        }
        return value_result;
    }

    if (not parser_input_arg.advance_if_starts_with(text.close_delimiter))
    {
        return parser_input_arg.fail(
            u8"Expected the closing delimiter " + quoted(text.close_delimiter)
            + u8"."
        );
    }

    chosen_arg = chosen;
    return {};
}

PNP_INTERNAL_NS_END
PNP_SUPPORT_NS_END
PNP_NS_END