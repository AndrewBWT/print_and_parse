#pragma once

#include "pnp/support/internal/sequences.hpp"

#include <queue>

PNP_NS_BEGIN

// std::queue: its underlying container, front to back: [1,2,3] has 1 at the front.
template <typename T, typename Container>
    requires default_printable_c<Container>
struct default_printer<std::queue<T, Container>>
    : public PNP_SUPPORT_NAMESPACE::generic_adaptor_pnp<std::queue<T, Container>, default_printer<Container>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_adaptor_pnp<std::queue<T, Container>, default_printer<Container>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T, typename Container>
    requires default_parseable_c<Container>
struct default_parser<std::queue<T, Container>>
    : public PNP_SUPPORT_NAMESPACE::generic_adaptor_pnp<std::queue<T, Container>, default_parser<Container>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_adaptor_pnp<std::queue<T, Container>, default_parser<Container>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END