#pragma once

#include "pnp/support/internal/sequences.hpp"

#include <stack>

PNP_NS_BEGIN

// std::stack: its underlying container, bottom to top: [1,2,3] has 3 on top.
template <typename T, typename Container>
    requires default_printable_c<Container>
struct default_printer<std::stack<T, Container>>
    : public PNP_SUPPORT_NAMESPACE::generic_adaptor_pnp<std::stack<T, Container>, default_printer<Container>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_adaptor_pnp<std::stack<T, Container>, default_printer<Container>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T, typename Container>
    requires default_parseable_c<Container>
struct default_parser<std::stack<T, Container>>
    : public PNP_SUPPORT_NAMESPACE::generic_adaptor_pnp<std::stack<T, Container>, default_parser<Container>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_adaptor_pnp<std::stack<T, Container>, default_parser<Container>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END