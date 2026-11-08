#pragma once
#include "pnp/support/internal/core_language/pointer.hpp"
#include "pnp/support/internal/core_language/char_traits.hpp"

#include <type_traits>

PNP_NS_BEGIN

// Character pointers: as C strings.
template <typename Pointer>
    requires PNP_SUPPORT_NAMESPACE::c_string_pointer_c<Pointer>
&& PNP_SUPPORT_NAMESPACE::enable_default_c_string_pnp<std::remove_cv_t<std::remove_pointer_t<Pointer>>>
struct default_printer<Pointer> : public PNP_SUPPORT_NAMESPACE::generic_c_string_pointer_printer<Pointer>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_c_string_pointer_printer<Pointer>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

// Every other pointer: as an address.
template <typename Pointer>
    requires std::is_pointer_v<Pointer>
&& (not PNP_SUPPORT_NAMESPACE::enable_default_c_string_pnp<std::remove_cv_t<std::remove_pointer_t<Pointer>>>)
struct default_printer<Pointer> : public PNP_SUPPORT_NAMESPACE::generic_pointer_printer<Pointer>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_pointer_printer<Pointer>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END