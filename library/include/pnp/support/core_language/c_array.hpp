#pragma once

#include "pnp/support/internal/core_language/char_traits.hpp"   // shared with the pointer defaults
#include "pnp/support/internal/core_language/c_array.hpp"

#include <cstddef>
#include <type_traits>

PNP_NS_BEGIN

// Character arrays: as strings.
template <typename E, std::size_t N>
    requires PNP_SUPPORT_NAMESPACE::c_string_array_c<E[N]>
&& PNP_SUPPORT_NAMESPACE::enable_default_c_string_pnp<std::remove_cv_t<E>>
struct default_printer<E[N]> : public PNP_SUPPORT_NAMESPACE::generic_c_string_array_pnp<E[N]>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_c_string_array_pnp<E[N]>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename E, std::size_t N>
    requires PNP_SUPPORT_NAMESPACE::c_string_array_c<E[N]>
&& PNP_SUPPORT_NAMESPACE::enable_default_c_string_pnp<std::remove_cv_t<E>>
struct default_parser<E[N]> : public PNP_SUPPORT_NAMESPACE::generic_c_string_array_pnp<E[N]>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_c_string_array_pnp<E[N]>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

// Every other array: as a sequence of its elements.
template <typename E, std::size_t N>
    requires (not (PNP_SUPPORT_NAMESPACE::c_string_array_c<E[N]>
&& PNP_SUPPORT_NAMESPACE::enable_default_c_string_pnp<std::remove_cv_t<E>>))
&& default_printable_c<E>
struct default_printer<E[N]> : public PNP_SUPPORT_NAMESPACE::generic_c_array_pnp<E[N], default_printer<E>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_c_array_pnp<E[N], default_printer<E>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename E, std::size_t N>
    requires (not (PNP_SUPPORT_NAMESPACE::c_string_array_c<E[N]>
&& PNP_SUPPORT_NAMESPACE::enable_default_c_string_pnp<std::remove_cv_t<E>>))
&& default_parseable_c<E>
struct default_parser<E[N]> : public PNP_SUPPORT_NAMESPACE::generic_c_array_pnp<E[N], default_parser<E>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_c_array_pnp<E[N], default_parser<E>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END