#pragma once

#include "pnp/support/internal/std/maps.hpp"

#include <map>

PNP_NS_BEGIN

// std::map: {1:"a",2:"b"}. A repeated key is a parse error.
template <typename Key, typename Value, typename Compare, typename Alloc>
    requires default_printable_c<Key>&& default_printable_c<Value>
struct default_printer<std::map<Key, Value, Compare, Alloc>>
    : public PNP_SUPPORT_NAMESPACE::default_map_printer<std::map<Key, Value, Compare, Alloc>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::default_map_printer<std::map<Key, Value, Compare, Alloc>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename Key, typename Value, typename Compare, typename Alloc>
    requires default_parseable_c<Key>&& default_parseable_c<Value>
struct default_parser<std::map<Key, Value, Compare, Alloc>>
    : public PNP_SUPPORT_NAMESPACE::default_map_parser<std::map<Key, Value, Compare, Alloc>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::default_map_parser<std::map<Key, Value, Compare, Alloc>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END