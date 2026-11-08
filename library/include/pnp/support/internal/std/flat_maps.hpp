#pragma once

#include "pnp/support/internal/sequences.hpp"
#include "pnp/support/internal/tuple_like.hpp"

#include <tuple>
#include <version>

#if defined(__cpp_lib_flat_map)
#include <flat_map>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

PNP_INTERNAL_NS_BEGIN
// A flat map stores keys and values apart, so iterating it yields pairs of references rather
// than std::pair<const K, V>. Those are what its entries print as. (Parsing still uses
// std::pair<K, V>, through the map specialisation of sequence_parse_value.)
template <typename K, typename V, typename C, typename KC, typename VC>
struct sequence_value<std::flat_map<K, V, C, KC, VC>>
{
    using type = typename std::flat_map<K, V, C, KC, VC>::const_reference;
};

template <typename K, typename V, typename C, typename KC, typename VC>
struct sequence_value<std::flat_multimap<K, V, C, KC, VC>>
{
    using type = typename std::flat_multimap<K, V, C, KC, VC>::const_reference;
};
PNP_INTERNAL_NS_END

// A flat map's entry printer: key, a colon, value, from the pair of references iteration yields.
template <typename Flat_Map, typename Key_Pnp, typename Value_Pnp>
using flat_map_entry_printer
= generic_tuple_pnp<typename Flat_Map::const_reference, std::tuple<Key_Pnp, Value_Pnp>, map_entry_pnp_default>;

PNP_SUPPORT_NS_END
PNP_NS_END

#endif