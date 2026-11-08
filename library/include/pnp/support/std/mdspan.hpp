#pragma once

#include "pnp/support/internal/std/mdspan.hpp"

#include <version>

#if defined(__cpp_lib_mdspan)
#include <mdspan>

PNP_NS_BEGIN

// std::mdspan: nested sequences, one level per dimension, [[1,2,3],[4,5,6]].
template <typename T, typename Extents, typename Layout, typename Accessor>
    requires default_printable_c<typename std::mdspan<T, Extents, Layout, Accessor>::value_type>
struct default_printer<std::mdspan<T, Extents, Layout, Accessor>>
    : public PNP_SUPPORT_NAMESPACE::generic_mdspan_printer<
    std::mdspan<T, Extents, Layout, Accessor>,
    default_printer<typename std::mdspan<T, Extents, Layout, Accessor>::value_type>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_mdspan_printer<
        std::mdspan<T, Extents, Layout, Accessor>,
        default_printer<typename std::mdspan<T, Extents, Layout, Accessor>::value_type>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END

#endif