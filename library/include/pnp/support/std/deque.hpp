#pragma once

#include "pnp/support/internal/sequences.hpp"

#include <deque>

PNP_NS_BEGIN

// std::deque: [1,2,3].
template <typename T, typename Alloc>
    requires default_printable_c<T>
struct default_printer<std::deque<T, Alloc>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::deque<T, Alloc>, default_printer<T>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::deque<T, Alloc>, default_printer<T>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T, typename Alloc>
    requires default_parseable_c<T>
struct default_parser<std::deque<T, Alloc>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::deque<T, Alloc>, default_parser<T>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::deque<T, Alloc>, default_parser<T>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END