#pragma once

#include "pnp/support/internal/std/reference_wrapper.hpp"

#include <functional>
#include <type_traits>

PNP_NS_BEGIN

// std::reference_wrapper<T>: exactly as the object it refers to.
template <typename T>
    requires default_printable_c<std::remove_cv_t<T>>
struct default_printer<std::reference_wrapper<T>>
    : public PNP_SUPPORT_NAMESPACE::generic_reference_wrapper_printer<std::reference_wrapper<T>,
    default_printer<std::remove_cv_t<T>>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_reference_wrapper_printer<std::reference_wrapper<T>,
        default_printer<std::remove_cv_t<T>>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END