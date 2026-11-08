#pragma once

#include "pnp/support/internal/std/smart_pointers.hpp"

#include <memory>
#include <type_traits>

PNP_NS_BEGIN

// std::unique_ptr: its pointee, or nullptr. Printable with any deleter;
// parseable only with the default deleter, which smart_pointer_factory knows
// how to create. Arrays (unique_ptr<T[]>) have no known size, so they aren't
// printable.
template <typename T, typename D>
    requires (not std::is_array_v<T>) && default_printable_c<std::remove_cv_t<T>>
struct default_printer<std::unique_ptr<T, D>>
    : public PNP_SUPPORT_NAMESPACE::generic_smart_pointer_pnp<
    std::unique_ptr<T, D>,
    default_printer<std::remove_cv_t<T>>,
    PNP_SUPPORT_NAMESPACE::unique_ptr_pnp_default>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_smart_pointer_pnp<
        std::unique_ptr<T, D>,
        default_printer<std::remove_cv_t<T>>,
        PNP_SUPPORT_NAMESPACE::unique_ptr_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T, typename D>
    requires (not std::is_array_v<T>) && default_parseable_c<std::remove_cv_t<T>>
struct default_parser<std::unique_ptr<T, D>>
    : public PNP_SUPPORT_NAMESPACE::generic_smart_pointer_pnp<
    std::unique_ptr<T, D>,
    default_parser<std::remove_cv_t<T>>,
    PNP_SUPPORT_NAMESPACE::unique_ptr_pnp_default>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_smart_pointer_pnp<
        std::unique_ptr<T, D>,
        default_parser<std::remove_cv_t<T>>,
        PNP_SUPPORT_NAMESPACE::unique_ptr_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END