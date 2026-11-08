#pragma once

#include "pnp/support/internal/std/maps.hpp"

#include <unordered_map>

PNP_NS_BEGIN

// std::unordered_map: {1:"a",2:"b"}, in the map's iteration order, which is unspecified.
// A repeated key is a parse error.
template <typename Key, typename Value, typename Hash, typename Key_Equal, typename Alloc>
    requires default_printable_c<Key>&& default_printable_c<Value>
struct default_printer<std::unordered_map<Key, Value, Hash, Key_Equal, Alloc>>
    : public PNP_SUPPORT_NAMESPACE::default_map_printer<std::unordered_map<Key, Value, Hash, Key_Equal, Alloc>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::default_map_printer<std::unordered_map<Key, Value, Hash, Key_Equal, Alloc>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename Key, typename Value, typename Hash, typename Key_Equal, typename Alloc>
    requires default_parseable_c<Key>&& default_parseable_c<Value>
struct default_parser<std::unordered_map<Key, Value, Hash, Key_Equal, Alloc>>
    : public PNP_SUPPORT_NAMESPACE::default_map_parser<std::unordered_map<Key, Value, Hash, Key_Equal, Alloc>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::default_map_parser<std::unordered_map<Key, Value, Hash, Key_Equal, Alloc>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};
PNP_NS_END