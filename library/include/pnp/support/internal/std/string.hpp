#pragma once
#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <unicode_bridge.hpp>
#include <utility>

PNP_NS_BEGIN

PNP_SUPPORT_NS_BEGIN
constexpr bool
    close_delimiter_is_escaped(
        const std::u8string_view delimiter_arg
    ) noexcept
{
    auto       itt = delimiter_arg.begin();
    const auto first
        = unicode_bridge::next_char32_and_increment_iterator_no_error(
            itt, delimiter_arg.end()
        );
    return first.has_value()
           && (first.value() == U'"' || first.value() == U'\'');
}

struct string_pnp_default
{
    static constexpr std::u8string_view open_delimiter  = u8"\"";
    static constexpr std::u8string_view close_delimiter = u8"\"";
};

// ------------------------------------------------------------ options

template <typename String_Pnp_Defaults>
struct string_pnp_options
{
    std::u8string open_delimiter
        = std::u8string(String_Pnp_Defaults::open_delimiter);
    std::u8string close_delimiter
        = std::u8string(String_Pnp_Defaults::close_delimiter);

    // The first rule these options break, or nullptr if they're valid.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;
};

template <typename String_Pnp_Defaults>
concept valid_string_pnp_c
    = valid_default_options_c<string_pnp_options<String_Pnp_Defaults>>;

template <typename String, typename String_Pnp_Defaults>
requires unicode_bridge::is_char_type_c<typename String::value_type>
         && valid_string_pnp_c<String_Pnp_Defaults>
struct generic_string_pnp
    : public pnp_configuration<string_pnp_options<String_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<string_pnp_options<String_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    using char_type   = typename String::value_type;
    using traits_type = typename String::traits_type;
    constexpr void
        run_printer(
            const std::same_as<String> auto& object_arg,
            text_sink&                       sink_obj_arg
        ) const;
    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            String&        object_answer,
            parser_run_key run_key_arg
        ) const;
};

PNP_INTERNAL_NS_BEGIN
template <typename CharT, typename Traits, typename String_Pnp_Defaults>
constexpr void
    print_delimited_text(
        const string_pnp_options<String_Pnp_Defaults>& options_arg,
        const std::basic_string_view<CharT, Traits>    view_arg,
        text_sink&                                     sink_obj_arg
    );
PNP_INTERNAL_NS_END
PNP_SUPPORT_NS_END
PNP_NS_END

PNP_NS_BEGIN

PNP_SUPPORT_NS_BEGIN
template <typename String_Pnp_Defaults>
constexpr const char*
    string_pnp_options<String_Pnp_Defaults>::first_error() const noexcept
{
    using namespace PNP_INTERNAL_NAMESPACE;
    if (not valid_utf8(open_delimiter))
    {
        return "string_pnp_options open delimiter must be valid UTF-8";
    }
    if (not valid_utf8(close_delimiter))
    {
        return "string_pnp_options close delimiter must be valid UTF-8";
    }
    if (not no_surrounding_whitespace(open_delimiter))
    {
        return "string_pnp_options open delimiter must not start or end with "
               "whitespace";
    }
    if (not no_surrounding_whitespace(close_delimiter))
    {
        return "string_pnp_options close delimiter must not start or end with "
               "whitespace";
    }
    if (open_delimiter.empty())
    {
        return "string_pnp_options open delimiter must be non-empty";
    }
    if (close_delimiter.empty())
    {
        return "string_pnp_options close delimiter must be non-empty";
    }
    if (not close_delimiter_is_escaped(close_delimiter))
    {
        return "string_pnp_options close delimiter must start with '\"' or "
               "'\\'', the characters always escaped "
               "inside strings";
    }
    return nullptr;
}

template <typename String, typename String_Pnp_Defaults>
requires unicode_bridge::is_char_type_c<typename String::value_type>
         && valid_string_pnp_c<String_Pnp_Defaults>
constexpr void
    generic_string_pnp<String, String_Pnp_Defaults>::run_printer(
        const std::same_as<String> auto& object_arg,
        text_sink&                       sink_obj_arg
    ) const
{
    PNP_INTERNAL_NAMESPACE::print_delimited_text(
        _options,
        std::basic_string_view<typename String::value_type>(object_arg),
        sink_obj_arg
    );
}

template <typename String, typename String_Pnp_Defaults>
requires unicode_bridge::is_char_type_c<typename String::value_type>
         && valid_string_pnp_c<String_Pnp_Defaults>
constexpr basic_parser_return_value
    generic_string_pnp<String, String_Pnp_Defaults>::run_parser(
        parser_input&  parser_input_arg,
        String&        object_answer,
        parser_run_key run_key_arg
    ) const
{
    if (not parser_input_arg.advance_if_starts_with(_options.open_delimiter))
    {
        return parser_input_arg.fail(
            u8"Expected opening delimiter \"" + _options.open_delimiter
            + u8"\"."
        );
    }
    const auto content
        = parser_input_arg.peek_until(_options.close_delimiter, u8"\\");
    if (not content.has_value())
    {
        return parser_input_arg.fail(
            u8"Expected closing delimiter \"" + _options.close_delimiter
            + u8"\", but the input ended."
        );
    }
    const auto text     = content.value().get();
    auto       curr_itt = text.begin();
    const auto end_itt  = text.end();
    String     return_value;
    // The closing delimiter is just after text, so every character in text is
    // part of the string.
    while (curr_itt != end_itt)
    {
        auto next_char = unicode_bridge::next_formatted_unicode_char<char_type>(
            curr_itt, end_itt
        );
        if (not next_char.has_value())
        {
            parser_input_arg.advance(
                static_cast<std::size_t>(curr_itt - text.begin())
            );
            return parser_input_arg.fail(
                u8"Could not read a character in the string. The Unicode "
                u8"parser returned the following error: "
                + next_char.error().message()
            );
        }
        auto& [chars, new_itt] = next_char.value();
        return_value.append(chars);
        curr_itt = new_itt;
    }

    parser_input_arg.advance(text.size());
    parser_input_arg.advance_if_starts_with(_options.close_delimiter
    ); // found by peek_until, so always there
    object_answer = std::move(return_value);
    return {};
}

PNP_INTERNAL_NS_BEGIN
template <typename CharT, typename Traits, typename String_Pnp_Defaults>
constexpr void
    print_delimited_text(
        const string_pnp_options<String_Pnp_Defaults>& options_arg,
        const std::basic_string_view<CharT, Traits>    view_arg,
        text_sink&                                     sink_obj_arg
    )
{
    sink_obj_arg.write(std::u8string_view(options_arg.open_delimiter));
    sink_obj_arg.write(
        unicode_bridge::to_formatted_unicode_string<char8_t>(view_arg)
    );
    sink_obj_arg.write(std::u8string_view(options_arg.close_delimiter));
}

PNP_INTERNAL_NS_END
PNP_SUPPORT_NS_END
PNP_NS_END
