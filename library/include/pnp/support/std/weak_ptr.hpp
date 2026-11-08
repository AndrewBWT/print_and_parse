#pragma once

#include "pnp/support/internal/std/smart_pointers.hpp"

#include <memory>
#include <type_traits>

PNP_NS_BEGIN

// std::weak_ptr: what it points to while alive, nullptr once expired.
// Print-only.
template <typename T>
    requires default_printable_c<std::remove_cv_t<T>>
struct default_printer<std::weak_ptr<T>>
    : public PNP_SUPPORT_NAMESPACE::generic_weak_pointer_printer<
    std::weak_ptr<T>,
    default_printer<std::remove_cv_t<T>>,
    PNP_SUPPORT_NAMESPACE::weak_ptr_pnp_default>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_weak_pointer_printer<
        std::weak_ptr<T>,
        default_printer<std::remove_cv_t<T>>,
        PNP_SUPPORT_NAMESPACE::weak_ptr_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END