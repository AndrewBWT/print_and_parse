#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/std/string.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"

#include <unicode_bridge.hpp>

#include <concepts>
#include <string_view>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN
template <typename View, typename String_Pnp_Defaults>
    requires unicode_bridge::is_char_type_c<typename View::value_type>&& valid_string_pnp_c<String_Pnp_Defaults>
struct generic_string_view_printer : public pnp_configuration<string_pnp_options<String_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<string_pnp_options<String_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;
    constexpr void
        run_printer(const std::same_as<View> auto& object_arg, text_sink& sink_obj_arg) const;
};

PNP_SUPPORT_NS_END

PNP_NS_END


PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename View, typename String_Pnp_Defaults>
    requires unicode_bridge::is_char_type_c<typename View::value_type>&& valid_string_pnp_c<String_Pnp_Defaults>
constexpr void
generic_string_view_printer<View, String_Pnp_Defaults>::run_printer(
    const std::same_as<View> auto& object_arg,
    text_sink& sink_obj_arg
) const
{
    PNP_INTERNAL_NAMESPACE::print_delimited_text(_options, object_arg, sink_obj_arg);
}

PNP_SUPPORT_NS_END
PNP_NS_END