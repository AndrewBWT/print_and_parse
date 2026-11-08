#pragma once

#include "pnp/support/internal/std/maps.hpp"

#include <map>

PNP_NS_BEGIN
// std::multimap: the same format; repeated keys are kept.
template <typename Key, typename Value, typename Compare, typename Alloc>
    requires default_printable_c<Key>&& default_printable_c<Value>
struct default_printer<std::multimap<Key, Value, Compare, Alloc>>
    : public PNP_SUPPORT_NAMESPACE::default_map_printer<std::multimap<Key, Value, Compare, Alloc>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::default_map_printer<std::multimap<Key, Value, Compare, Alloc>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename Key, typename Value, typename Compare, typename Alloc>
    requires default_parseable_c<Key>&& default_parseable_c<Value>
struct default_parser<std::multimap<Key, Value, Compare, Alloc>>
    : public PNP_SUPPORT_NAMESPACE::default_map_parser<std::multimap<Key, Value, Compare, Alloc>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::default_map_parser<std::multimap<Key, Value, Compare, Alloc>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END