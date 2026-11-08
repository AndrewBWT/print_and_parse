#pragma once

#include "pnp/support/internal/std/variant.hpp"

#include <tuple>
#include <type_traits>
#include <variant>

PNP_NS_BEGIN

// std::monostate: monostate.
template <>
struct default_printer<std::monostate> : public PNP_SUPPORT_NAMESPACE::monostate_pnp<>
{
    using base_t = PNP_SUPPORT_NAMESPACE::monostate_pnp<>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <>
struct default_parser<std::monostate> : public PNP_SUPPORT_NAMESPACE::monostate_pnp<>
{
    using base_t = PNP_SUPPORT_NAMESPACE::monostate_pnp<>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

// std::variant<Ts...>: the alternative's index, a colon, and its text, 0:1.
template <typename... Ts>
    requires (default_printable_c<std::remove_cv_t<Ts>> && ...)
struct default_printer<std::variant<Ts...>>
    : public PNP_SUPPORT_NAMESPACE::generic_variant_pnp<std::variant<Ts...>,
    std::tuple<default_printer<std::remove_cv_t<Ts>>...>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_variant_pnp<std::variant<Ts...>,
        std::tuple<default_printer<std::remove_cv_t<Ts>>...>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename... Ts>
    requires (default_parseable_c<std::remove_cv_t<Ts>> && ...)
struct default_parser<std::variant<Ts...>>
    : public PNP_SUPPORT_NAMESPACE::generic_variant_pnp<std::variant<Ts...>,
    std::tuple<default_parser<std::remove_cv_t<Ts>>...>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_variant_pnp<std::variant<Ts...>,
        std::tuple<default_parser<std::remove_cv_t<Ts>>...>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END