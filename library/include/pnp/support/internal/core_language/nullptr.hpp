#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

PNP_INTERNAL_NS_BEGIN
constexpr bool
    valid_null_id(
        const std::u8string_view null_id_arg
    ) noexcept
{
    return not null_id_arg.empty() && valid_utf8(null_id_arg)
           && no_surrounding_whitespace(null_id_arg);
}

PNP_INTERNAL_NS_END

struct null_id_default_base
{
    static constexpr std::u8string_view null_id = u8"nullptr";
};

struct nullptr_pnp_default : null_id_default_base
{};

template <typename Nullptr_Pnp_Defaults>
struct nullptr_pnp_options
{
    std::u8string null_id = std::u8string(Nullptr_Pnp_Defaults::null_id);
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;

    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept
    {
        return null_id.size();
    }
};

template <typename Nullptr_Pnp_Defaults>
concept valid_nullptr_pnp_c
    = valid_default_options_c<nullptr_pnp_options<Nullptr_Pnp_Defaults>>;

template <typename Nullptr_Pnp_Defaults = nullptr_pnp_default>
requires valid_nullptr_pnp_c<Nullptr_Pnp_Defaults>
struct nullptr_pnp
    : public pnp_configuration<nullptr_pnp_options<Nullptr_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<nullptr_pnp_options<Nullptr_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    // The text this pnp always prints, and the only text it parses.
    [[nodiscard]]
    constexpr std::u8string_view
        token() const noexcept
    {
        return _options.null_id;
    }

    constexpr void
        run_printer(
            const std::same_as<std::nullptr_t> auto& object_arg,
            text_sink&                               sink_obj_arg
        ) const;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&   parser_input_arg,
            std::nullptr_t& object_answer,
            parser_run_key  run_key_arg
        ) const;
};

PNP_SUPPORT_NS_END
PNP_NS_END


PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename Nullptr_Pnp_Defaults>
constexpr const char*
    nullptr_pnp_options<Nullptr_Pnp_Defaults>::first_error() const noexcept
{
    using namespace PNP_INTERNAL_NAMESPACE;
    if (not valid_utf8(null_id))
    {
        return "nullptr_pnp_options null id must be valid UTF-8";
    }
    if (not no_surrounding_whitespace(null_id))
    {
        return "nullptr_pnp_options null id must not start or end with "
               "whitespace";
    }
    if (null_id.empty())
    {
        return "nullptr_pnp_options null id must be non-empty";
    }
    return nullptr;
}

template <typename Nullptr_Pnp_Defaults>
requires valid_nullptr_pnp_c<Nullptr_Pnp_Defaults>
constexpr void
    nullptr_pnp<Nullptr_Pnp_Defaults>::run_printer(
        const std::same_as<std::nullptr_t> auto&,
        text_sink& sink_obj_arg
    ) const
{
    sink_obj_arg.write(std::u8string_view(_options.null_id));
}

template <typename Nullptr_Pnp_Defaults>
requires valid_nullptr_pnp_c<Nullptr_Pnp_Defaults>
constexpr basic_parser_return_value
    nullptr_pnp<Nullptr_Pnp_Defaults>::run_parser(
        parser_input&   parser_input_arg,
        std::nullptr_t& object_answer,
        parser_run_key  run_key_arg
    ) const
{
    if (not parser_input_arg.advance_if_starts_with(_options.null_id))
    {
        return parser_input_arg.fail(
            u8"Expected \"" + _options.null_id + u8"\"."
        );
    }
    object_answer = nullptr;
    return {};
}

PNP_SUPPORT_NS_END
PNP_NS_END