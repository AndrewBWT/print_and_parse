#pragma once

#include "pnp/core.hpp"
// For the u8strings.
#include <string>
// For the u8string_view's.
#include <string_view>
// For std::same_as
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <concepts>
#include <cstddef>
#include <iterator>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

struct char_pnp_single_quoted
{
    static constexpr std::u8string_view open_delimiter  = u8"'";
    static constexpr std::u8string_view close_delimiter = u8"'";
};

template <typename Char_Pnp_Defaults>
struct char_pnp_options
{
    std::u8string open_delimiter
        = std::u8string(Char_Pnp_Defaults::open_delimiter);
    std::u8string close_delimiter
        = std::u8string(Char_Pnp_Defaults::close_delimiter);
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;
};

template <typename Char_Pnp_Defaults>
concept valid_char_pnp_c
    = valid_default_options_c<char_pnp_options<Char_Pnp_Defaults>>;

template <typename CharT, typename Char_Pnp_Defaults = char_pnp_single_quoted>
requires unicode_bridge::is_char_type_c<CharT>
         && valid_char_pnp_c<Char_Pnp_Defaults>
struct generic_char_pnp
    : public pnp_configuration<char_pnp_options<Char_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<char_pnp_options<Char_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;
    constexpr void
        run_printer(
            const std::same_as<CharT> auto& object_arg,
            text_sink&                      sink_obj_arg
        ) const;
    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            CharT&         object_answer,
            parser_run_key run_key_arg
        ) const;
};

template <typename T>
inline constexpr bool enable_default_char_pnp = false;
PNP_SUPPORT_NS_END

template <typename CharT>
requires PNP_SUPPORT_NAMESPACE::enable_default_char_pnp<CharT>
struct default_printer<CharT>
    : public PNP_SUPPORT_NAMESPACE::generic_char_pnp<CharT>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_char_pnp<CharT>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename CharT>
requires PNP_SUPPORT_NAMESPACE::enable_default_char_pnp<CharT>
struct default_parser<CharT>
    : public PNP_SUPPORT_NAMESPACE::generic_char_pnp<CharT>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_char_pnp<CharT>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END
PNP_NS_BEGIN

PNP_SUPPORT_NS_BEGIN
template <typename Char_Pnp_Defaults>
constexpr const char*
    char_pnp_options<Char_Pnp_Defaults>::first_error() const noexcept
{
    using namespace PNP_INTERNAL_NAMESPACE;
    if (not valid_utf8(open_delimiter))
    {
        return "char_pnp_options open delimiter must be valid UTF-8";
    }
    if (not valid_utf8(close_delimiter))
    {
        return "char_pnp_options close delimiter must be valid UTF-8";
    }
    if (not no_surrounding_whitespace(open_delimiter))
    {
        return "char_pnp_options open delimiter must not start or end with "
               "whitespace";
    }
    if (not no_surrounding_whitespace(close_delimiter))
    {
        return "char_pnp_options close delimiter must not start or end with "
               "whitespace";
    }
    if (open_delimiter.empty())
    {
        return "char_pnp_options open delimiter must be non-empty";
    }
    if (close_delimiter.empty())
    {
        return "char_pnp_options close delimiter must be non-empty";
    }
    return nullptr;
}

template <typename CharT, typename Char_Pnp_Defaults>
requires unicode_bridge::is_char_type_c<CharT>
         && valid_char_pnp_c<Char_Pnp_Defaults>
constexpr void
    generic_char_pnp<CharT, Char_Pnp_Defaults>::run_printer(
        const std::same_as<CharT> auto& object_arg,
        text_sink&                      sink_obj_arg
    ) const
{
    sink_obj_arg.write(std::u8string_view(_options.open_delimiter));
    sink_obj_arg.write(unicode_bridge::to_formatted_unicode_string<char8_t>(
        std::basic_string_view<CharT>(&object_arg, 1)
    ));
    sink_obj_arg.write(std::u8string_view(_options.close_delimiter));
}

template <typename CharT, typename Char_Pnp_Defaults>
requires unicode_bridge::is_char_type_c<CharT>
         && valid_char_pnp_c<Char_Pnp_Defaults>
constexpr basic_parser_return_value
    generic_char_pnp<CharT, Char_Pnp_Defaults>::run_parser(
        parser_input&  parser_input_arg,
        CharT&         object_answer,
        parser_run_key run_key_arg
    ) const
{
    using namespace std;
    using namespace unicode_bridge;
    if (not parser_input_arg.advance_if_starts_with(_options.open_delimiter))
    {
        return parser_input_arg.fail(
            u8"Expected opening delimiter \"" + _options.open_delimiter
            + u8"\"."
        );
    }

    // A char holds exactly one formatted character, which may itself be the
    // closing delimiter
    // (> between < and >), so decode one first, rather than searching for the
    // delimiter. Enough is made available for the longest formatted character,
    // an escape such as \U0010FFFF.
    // A char holds exactly one formatted character, which may itself be the
    // closing delimiter
    // (> between < and >), so decode one first, rather than searching for the
    // delimiter. Only as much is read as the character needs: its first code
    // unit gives a first guess at its length (an escape is at least two; a
    // literal character's first byte says exactly how long it is), and if that
    // isn't enough to decode it, one more code unit at a time, up to the
    // longest a formatted character can be (an escape such as \u{10FFFF}).
    // Reading further could wait on interactive input for text that isn't part
    // of the character.
    // The first character, read whole: if its code units arrive in separate
    // pieces, this reads up to its last one, and no further. Throws
    // invalid_utf8 if the text isn't valid UTF-8.
    const optional<pair<char32_t, size_t>> first = parser_input_arg.peek_char();
    if (not first.has_value())
    {
        return parser_input_arg.fail(
            u8"Expected a character, but the input ended."
        );
    }
    std::size_t n_chars = 1;
    if (first.value().first == U'\\')
    {
        n_chars = parser_input_arg.starts_with(u8"\\x")   ? 4
                  : parser_input_arg.starts_with(u8"\\u") ? 6
                  : parser_input_arg.starts_with(u8"\\U") ? 10
                                                          : 2;
    }
    const auto text_opt = parser_input_arg.peek_chars(n_chars);
    if (not text_opt.has_value())
    {
        return parser_input_arg.fail(
            u8"Expected a character, but the input ended part-way through it."
        );
    }
    auto& text        = text_opt.value();
    auto  current_itt = text.get().begin();
    auto  decoded_char
        = next_formatted_unicode_char<CharT>(current_itt, text.get().end());
    if (not decoded_char.has_value())
    {
        return parser_input_arg.fail(
            u8"Could not read a character. The Unicode parser returned the "
            u8"following error: "
            + decoded_char.error().message()
        );
    }
    auto& [char_extracted, new_itt] = decoded_char.value();
    if (char_extracted.size() != 1)
    {
        return parser_input_arg.fail(
            u8"The character \""
            + to_formatted_unicode_string<char8_t>(char_extracted)
            + u8"\" needs "
            + PNP_INTERNAL_NAMESPACE::size_to_u8string(char_extracted.size())
            + u8" code units, but the target type holds only one."
        );
    }
    parser_input_arg.advance(
        static_cast<size_t>(std::distance(text.get().begin(), new_itt))
    );
    if (not parser_input_arg.advance_if_starts_with(_options.close_delimiter))
    {
        return parser_input_arg.fail(
            u8"Expected closing delimiter \"" + _options.close_delimiter
            + u8"\"."
        );
    }
    object_answer = char_extracted[0];
    return {};
}

PNP_SUPPORT_NS_END
PNP_NS_END