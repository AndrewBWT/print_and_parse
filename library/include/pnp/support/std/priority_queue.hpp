#pragma once

#include "pnp/support/internal/sequences.hpp"

#include <queue>

PNP_NS_BEGIN

// std::priority_queue: its underlying container, in the queue's internal heap order (not
// sorted). Parsing rebuilds the heap, so the same elements come out in the same priority
// order, though re-printing may order them differently.
template <typename T, typename Container, typename Compare>
    requires default_printable_c<Container>
struct default_printer<std::priority_queue<T, Container, Compare>>
    : public PNP_SUPPORT_NAMESPACE::generic_adaptor_pnp<
    std::priority_queue<T, Container, Compare>, default_printer<Container>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_adaptor_pnp<
        std::priority_queue<T, Container, Compare>, default_printer<Container>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T, typename Container, typename Compare>
    requires default_parseable_c<Container>&& std::default_initializable<Compare>
struct default_parser<std::priority_queue<T, Container, Compare>>
    : public PNP_SUPPORT_NAMESPACE::generic_adaptor_pnp<
    std::priority_queue<T, Container, Compare>, default_parser<Container>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_adaptor_pnp<
        std::priority_queue<T, Container, Compare>, default_parser<Container>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END