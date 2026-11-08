#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <unicode_bridge.hpp>
// For the u8strings.
#include <string>
// For the u8string_view's.
#include <string_view>
// For std::same_as
#include <concepts>
#include <cstddef>
#include <optional>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

struct bool_pnp_vals_words_lc
{
    static constexpr std::u8string_view true_value  = u8"true";
    static constexpr std::u8string_view false_value = u8"false";
};

struct bool_pnp_vals_words_uc
{
    static constexpr std::u8string_view true_value  = u8"TRUE";
    static constexpr std::u8string_view false_value = u8"FALSE";
};

struct bool_pnp_vals_words_uc_first
{
    static constexpr std::u8string_view true_value  = u8"True";
    static constexpr std::u8string_view false_value = u8"False";
};

struct bool_pnp_vals_letter_lc
{
    static constexpr std::u8string_view true_value  = u8"t";
    static constexpr std::u8string_view false_value = u8"f";
};

struct bool_pnp_vals_letter_uc
{
    static constexpr std::u8string_view true_value  = u8"T";
    static constexpr std::u8string_view false_value = u8"F";
};

struct bool_pnp_vals_answer_lc
{
    static constexpr std::u8string_view true_value  = u8"yes";
    static constexpr std::u8string_view false_value = u8"no";
};

struct bool_pnp_vals_answer_uc
{
    static constexpr std::u8string_view true_value  = u8"YES";
    static constexpr std::u8string_view false_value = u8"NO";
};

struct bool_pnp_vals_answer_uc_first
{
    static constexpr std::u8string_view true_value  = u8"Yes";
    static constexpr std::u8string_view false_value = u8"No";
};

template <typename Bool_Pnp_Defaults>
struct bool_pnp_options
{
    std::u8string true_value  = std::u8string(Bool_Pnp_Defaults::true_value);
    std::u8string false_value = std::u8string(Bool_Pnp_Defaults::false_value);

    // The first rule these options break, or nullptr if they're valid.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;
    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept;
};

template <typename Bool_Pnp_Defaults>
concept valid_bool_pnp_c
    = valid_default_options_c<bool_pnp_options<Bool_Pnp_Defaults>>;

template <typename Bool_Pnp_Defaults = bool_pnp_vals_words_lc>
requires valid_bool_pnp_c<Bool_Pnp_Defaults>
struct bool_pnp : public pnp_configuration<bool_pnp_options<Bool_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<bool_pnp_options<Bool_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;
    constexpr void
        run_printer(
            const std::same_as<bool> auto& object_arg,
            text_sink&                     sink_obj_arg
        ) const;
    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            bool&          object_answer,
            parser_run_key run_key_arg
        ) const;
};

PNP_SUPPORT_NS_END
PNP_NS_END

PNP_NS_BEGIN

PNP_SUPPORT_NS_BEGIN
template <typename Bool_Pnp_Defaults>
constexpr const char*
    bool_pnp_options<Bool_Pnp_Defaults>::first_error() const noexcept
{
    using namespace PNP_INTERNAL_NAMESPACE;
    if (not valid_utf8(true_value))
    {
        return "bool_pnp_options true value must be valid UTF-8";
    }
    if (not valid_utf8(false_value))
    {
        return "bool_pnp_options false value must be valid UTF-8";
    }
    if (not no_surrounding_whitespace(true_value))
    {
        return "bool_pnp_options true value must not start or end with "
               "whitespace";
    }
    if (not no_surrounding_whitespace(false_value))
    {
        return "bool_pnp_options false value must not start or end with "
               "whitespace";
    }
    if (true_value.empty())
    {
        return "bool_pnp_options true value must be non-empty";
    }
    if (false_value.empty())
    {
        return "bool_pnp_options false value must be non-empty";
    }
    // Also rules out identical words. With neither a prefix of the other, at
    // most one can match, so the parser can check them in a fixed order.
    if (true_value.starts_with(false_value)
        || false_value.starts_with(true_value))
    {
        return "bool_pnp_options true and false values must not be prefixes of "
               "each other";
    }
    return nullptr;
}

template <typename Bool_Pnp_Defaults>
constexpr std::optional<std::size_t>
    bool_pnp_options<Bool_Pnp_Defaults>::fixed_output_width() const noexcept
{
    if (true_value.size() != false_value.size())
    {
        return std::nullopt;
    }
    return true_value.size();
}

template <typename Bool_Pnp_Defaults>
requires valid_bool_pnp_c<Bool_Pnp_Defaults>
constexpr void
    bool_pnp<Bool_Pnp_Defaults>::run_printer(
        const std::same_as<bool> auto& object_arg,
        text_sink&                     sink_obj_arg
    ) const
{
    sink_obj_arg.write(object_arg ? _options.true_value : _options.false_value);
}

template <typename Bool_Pnp_Defaults>
requires valid_bool_pnp_c<Bool_Pnp_Defaults>
constexpr basic_parser_return_value
    bool_pnp<Bool_Pnp_Defaults>::run_parser(
        parser_input&  parser_input_arg,
        bool&          object_answer,
        parser_run_key run_key_arg
    ) const
{
    if (parser_input_arg.advance_if_starts_with(_options.true_value))
    {
        object_answer = true;
        return {};
    }
    if (parser_input_arg.advance_if_starts_with(_options.false_value))
    {
        object_answer = false;
        return {};
    }
    else
    {
        return parser_input_arg.fail(
            u8"Could not construct bool. Expected either \""
            + _options.true_value + u8"\" representing true, or \""
            + _options.false_value + u8"\" representing false."
        );
    }
}

PNP_SUPPORT_NS_END
PNP_NS_END