#pragma once

#include "pnp/support/internal/std/smart_pointers.hpp"

#include <memory>
#include <type_traits>

PNP_NS_BEGIN

// std::shared_ptr: its pointee, or nullptr. Parsing creates a new, unshared pointee.
template <typename T>
    requires (not std::is_array_v<T>) && default_printable_c<std::remove_cv_t<T>>
struct default_printer<std::shared_ptr<T>>
    : public PNP_SUPPORT_NAMESPACE::generic_smart_pointer_pnp<
    std::shared_ptr<T>,
    default_printer<std::remove_cv_t<T>>,
    PNP_SUPPORT_NAMESPACE::shared_ptr_pnp_default>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_smart_pointer_pnp<
        std::shared_ptr<T>,
        default_printer<std::remove_cv_t<T>>,
        PNP_SUPPORT_NAMESPACE::shared_ptr_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T>
    requires (not std::is_array_v<T>) && default_parseable_c<std::remove_cv_t<T>>
struct default_parser<std::shared_ptr<T>>
    : public PNP_SUPPORT_NAMESPACE::generic_smart_pointer_pnp<
    std::shared_ptr<T>,
    default_parser<std::remove_cv_t<T>>,
    PNP_SUPPORT_NAMESPACE::shared_ptr_pnp_default>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_smart_pointer_pnp<
        std::shared_ptr<T>,
        default_parser<std::remove_cv_t<T>>,
        PNP_SUPPORT_NAMESPACE::shared_ptr_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};


PNP_NS_END