#pragma once

#include "pnp/support/internal/std/bitset.hpp"

#include <bitset>
#include <cstddef>

PNP_NS_BEGIN

// std::bitset<N>: N binary digits, most significant first, 00000101.
template <std::size_t N>
struct default_printer<std::bitset<N>> : public PNP_SUPPORT_NAMESPACE::generic_bitset_pnp<N>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_bitset_pnp<N>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <std::size_t N>
struct default_parser<std::bitset<N>> : public PNP_SUPPORT_NAMESPACE::generic_bitset_pnp<N>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_bitset_pnp<N>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END