#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/alternatives.hpp"
#include "pnp/support/internal/utility.hpp"

#include <array>
#include <string>
#include <string_view>
#include <unicode_bridge.hpp>
#include <utility>


PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN
PNP_INTERNAL_NS_BEGIN

// For types that are either a fixed token or a value of any form: a null
// pointer or what it points to, an empty optional or what it holds. The value
// is written between delimiters, so the parser can tell which case it's reading
// from the first characters: nullptr against unique_ptr(5), nullopt against
// {5}. Each type's options keep their own natural names; these functions share
// the mechanism.

// The text a token-or-value type works with, and how to describe it in error
// messages.
struct token_or_value_text
{
    std::u8string_view token;             // nullptr
    std::u8string_view token_description; // null token
    std::u8string_view open_delimiter;    // unique_ptr(
    std::u8string_view close_delimiter;   // )
    std::u8string_view value_description; // the pointee of a smart pointer
};

template <typename P>
concept token_pnp_c = requires (const P& pnp_arg) {
    { pnp_arg.token() } noexcept -> std::convertible_to<std::u8string_view>;
};
constexpr std::array<alternative_text, 2>
    token_or_value_alternatives(const token_or_value_text& text_arg) noexcept;
// Whether the parser can tell the token from the opening delimiter: neither may
// be a prefix of the other. With an empty opening delimiter, a value written
// straight after nothing can't be told apart from the token by its first
// characters, so that case is allowed, and documented by each type, rather than
// checked.
[[nodiscard]]
constexpr bool
    token_and_delimiter_distinguishable(
        std::u8string_view token_arg,
        std::u8string_view open_delimiter_arg
    ) noexcept;

// Prints the token, or the value between the delimiters.
template <typename Print_Value>
constexpr void
    print_token_or_value(
        const token_or_value_text& text_arg,
        bool                       has_value_arg,
        Print_Value&&              print_value_arg,
        text_sink&    sink_obj_arg
    );

// Parses the token, or the value between the delimiters. On success,
// is_token_arg says which it was; parse_value_arg is called only for a value,
// and should parse it from the input it's given, returning the result. Its
// failure is returned with the value's description added.
template <typename Parse_Value>
constexpr basic_parser_return_value
    parse_token_or_value(
        const token_or_value_text& text_arg,
        parser_input&              parser_input_arg,
        bool&                      is_token_arg,
        Parse_Value&&              parse_value_arg
    );

// ============================================================ definitions
constexpr std::array<alternative_text, 2>
    token_or_value_alternatives(
        const token_or_value_text& text_arg
    ) noexcept
{
    return {
        {
         {.marker_description = text_arg.token_description,
             .open_delimiter     = text_arg.token},
         {.marker_description = u8"opening delimiter",
             .open_delimiter     = text_arg.open_delimiter,
             .close_delimiter    = text_arg.close_delimiter,
             .value_description  = text_arg.value_description},
         }
    };
}

constexpr bool
    token_and_delimiter_distinguishable(
        const std::u8string_view token_arg,
        const std::u8string_view open_delimiter_arg
    ) noexcept
{
    return alternatives_distinguishable(
        std::array{token_arg, open_delimiter_arg}
    );
}

template <typename Print_Value>
constexpr void
    print_token_or_value(
        const token_or_value_text& text_arg,
        const bool                 has_value_arg,
        Print_Value&&              print_value_arg,
        text_sink&    sink_obj_arg
    )
{
    const auto alternatives = token_or_value_alternatives(text_arg);
    if (has_value_arg)
    {
        print_alternative(
            alternatives[1],
            std::forward<Print_Value>(print_value_arg),
            sink_obj_arg
        );
    }
    else
    {
        print_alternative(
            alternatives[0],
            []
            {
            },
            sink_obj_arg
        );
    }
}

template <typename Parse_Value>
constexpr basic_parser_return_value
    parse_token_or_value(
        const token_or_value_text& text_arg,
        parser_input&              parser_input_arg,
        bool&                      is_token_arg,
        Parse_Value&&              parse_value_arg
    )
{
    std::size_t chosen = 0;
    if (auto result = parse_alternatives(
            token_or_value_alternatives(text_arg),
            parser_input_arg,
            chosen,
            [&](const std::size_t index_arg,
                parser_input&     input_arg) -> basic_parser_return_value
            {
                if (index_arg == 0)
                {
                    return {}; // the token has no value
                }
                return parse_value_arg(input_arg);
            }
        );
        result.failed())
    {
        return result;
    }
    is_token_arg = (chosen == 0);
    return {};
}

PNP_INTERNAL_NS_END
PNP_SUPPORT_NS_END
PNP_NS_END