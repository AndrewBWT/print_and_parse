#pragma once
#include "pnp/support/internal/tuple_like.hpp"

PNP_NS_BEGIN
template <typename... Ts>
    requires (default_printable_c<Ts> && ...)
struct default_printer<std::tuple<Ts...>>
    : public PNP_SUPPORT_NAMESPACE::generic_tuple_pnp<std::tuple<Ts...>, std::tuple<default_printer<Ts>...>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_tuple_pnp<std::tuple<Ts...>, std::tuple<default_printer<Ts>...>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename... Ts>
    requires (default_parseable_c<Ts> && ...)
struct default_parser<std::tuple<Ts...>>
    : public PNP_SUPPORT_NAMESPACE::generic_tuple_pnp<std::tuple<Ts...>, std::tuple<default_parser<Ts>...>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_tuple_pnp<std::tuple<Ts...>, std::tuple<default_parser<Ts>...>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};
PNP_NS_END