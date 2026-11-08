#pragma once

#include "pnp/support/internal/sequences.hpp"

#include <set>

PNP_NS_BEGIN

// std::set: {1,2,3}, in the set's order. A repeated key is a parse error.
template <typename Key, typename Compare, typename Alloc>
    requires default_printable_c<Key>
struct default_printer<std::set<Key, Compare, Alloc>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<
    std::set<Key, Compare, Alloc>, default_printer<Key>, PNP_SUPPORT_NAMESPACE::set_pnp_default>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<
        std::set<Key, Compare, Alloc>, default_printer<Key>, PNP_SUPPORT_NAMESPACE::set_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename Key, typename Compare, typename Alloc>
    requires default_parseable_c<Key>
struct default_parser<std::set<Key, Compare, Alloc>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<
    std::set<Key, Compare, Alloc>, default_parser<Key>, PNP_SUPPORT_NAMESPACE::set_pnp_default>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<
        std::set<Key, Compare, Alloc>, default_parser<Key>, PNP_SUPPORT_NAMESPACE::set_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END