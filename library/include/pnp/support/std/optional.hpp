#pragma once
#include "pnp/support/internal/std/optional.hpp"

#include <optional>
#include <type_traits>

PNP_NS_BEGIN

// std::nullopt: nullopt.
template <>
struct default_printer<std::nullopt_t> : public PNP_SUPPORT_NAMESPACE::nullopt_pnp<>
{
    using base_t = PNP_SUPPORT_NAMESPACE::nullopt_pnp<>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <>
struct default_parser<std::nullopt_t> : public PNP_SUPPORT_NAMESPACE::nullopt_pnp<>
{
    using base_t = PNP_SUPPORT_NAMESPACE::nullopt_pnp<>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

// std::optional<T>: nullopt, or its value in braces, {5}.
template <typename T>
    requires default_printable_c<std::remove_cv_t<T>>
struct default_printer<std::optional<T>>
    : public PNP_SUPPORT_NAMESPACE::generic_optional_pnp<std::optional<T>, default_printer<std::remove_cv_t<T>>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_optional_pnp<std::optional<T>, default_printer<std::remove_cv_t<T>>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T>
    requires default_parseable_c<std::remove_cv_t<T>>
struct default_parser<std::optional<T>>
    : public PNP_SUPPORT_NAMESPACE::generic_optional_pnp<std::optional<T>, default_parser<std::remove_cv_t<T>>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_optional_pnp<std::optional<T>, default_parser<std::remove_cv_t<T>>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END