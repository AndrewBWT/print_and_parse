#pragma once

#include "pnp/support/internal/sequences.hpp"

#include <array>
#include <cstddef>

PNP_NS_BEGIN

// std::array: a sequence of exactly N elements, [1,2,3]. Parsing requires exactly N.
template <typename T, std::size_t N>
    requires default_printable_c<T>
struct default_printer<std::array<T, N>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::array<T, N>, default_printer<T>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::array<T, N>, default_printer<T>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T, std::size_t N>
    requires default_parseable_c<T>
struct default_parser<std::array<T, N>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::array<T, N>, default_parser<T>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::array<T, N>, default_parser<T>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END