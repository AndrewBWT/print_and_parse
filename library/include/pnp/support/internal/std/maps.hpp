#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/sequences.hpp"
#include "pnp/support/internal/tuple_like.hpp"

#include <tuple>
#include <utility>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN
// A map entry: no delimiters, a colon between key and value: 1:"a".
struct map_entry_pnp_default : tuple_pnp_default_base
{
    static constexpr std::u8string_view open_delimiter = u8"";
    static constexpr std::u8string_view close_delimiter = u8"";
    static constexpr std::u8string_view separator = u8":";
};
// The default pnp for one map entry when printing: the map's own std::pair<const K, V>,
// as key:value.
template <typename Key, typename Value>
using default_map_entry_printer = generic_tuple_pnp<
    std::pair<const Key, Value>,
    std::tuple<default_printer<Key>, default_printer<Value>>,
    map_entry_pnp_default>;

// The default pnp for one map entry when parsing: an assignable std::pair<K, V>, which the
// insert appender then adds to the map.
template <typename Key, typename Value>
using default_map_entry_parser
= generic_tuple_pnp<std::pair<Key, Value>, std::tuple<default_parser<Key>, default_parser<Value>>, map_entry_pnp_default>;

// The default pnps for a whole map: its entries in braces, {1:"a",2:"b"}.
template <typename Map>
using default_map_printer = generic_sequence_pnp<
    Map,
    default_map_entry_printer<typename Map::key_type, typename Map::mapped_type>,
    map_pnp_default>;

template <typename Map>
using default_map_parser = generic_sequence_pnp<
    Map,
    default_map_entry_parser<typename Map::key_type, typename Map::mapped_type>,
    map_pnp_default>;

PNP_SUPPORT_NS_END
PNP_NS_END