#pragma once

#include "pnp/support/internal/std/compare.hpp"

#include <compare>

PNP_NS_BEGIN

// std::strong_ordering, std::weak_ordering and std::partial_ordering: less, equal (or
// equivalent), greater, and unordered.
template <typename O>
    requires PNP_SUPPORT_NAMESPACE::std_ordering_c<O>
struct default_printer<O> : public PNP_SUPPORT_NAMESPACE::ordering_pnp<O>
{
    using base_t = PNP_SUPPORT_NAMESPACE::ordering_pnp<O>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename O>
    requires PNP_SUPPORT_NAMESPACE::std_ordering_c<O>
struct default_parser<O> : public PNP_SUPPORT_NAMESPACE::ordering_pnp<O>
{
    using base_t = PNP_SUPPORT_NAMESPACE::ordering_pnp<O>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END