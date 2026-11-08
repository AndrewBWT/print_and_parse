#pragma once

#include "pnp/support/internal/std/maps.hpp"
#include "pnp/support/internal/std/flat_maps.hpp"

#include <version>

#if defined(__cpp_lib_flat_map)
#include <flat_map>

PNP_NS_BEGIN

// std::flat_map and std::flat_multimap: as std::map does, {1:"a",2:"b"}.
template <typename K, typename V, typename C, typename KC, typename VC>
    requires default_printable_c<K>&& default_printable_c<V>
struct default_printer<std::flat_map<K, V, C, KC, VC>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<
    std::flat_map<K, V, C, KC, VC>,
    PNP_SUPPORT_NAMESPACE::flat_map_entry_printer<std::flat_map<K, V, C, KC, VC>, default_printer<K>, default_printer<V>>,
    PNP_SUPPORT_NAMESPACE::map_pnp_default>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<
        std::flat_map<K, V, C, KC, VC>,
        PNP_SUPPORT_NAMESPACE::flat_map_entry_printer<std::flat_map<K, V, C, KC, VC>, default_printer<K>, default_printer<V>>,
        PNP_SUPPORT_NAMESPACE::map_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename K, typename V, typename C, typename KC, typename VC>
    requires default_parseable_c<K>&& default_parseable_c<V>
struct default_parser<std::flat_map<K, V, C, KC, VC>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<
    std::flat_map<K, V, C, KC, VC>,
    PNP_SUPPORT_NAMESPACE::default_map_entry_parser<K, V>,
    PNP_SUPPORT_NAMESPACE::map_pnp_default>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<
        std::flat_map<K, V, C, KC, VC>,
        PNP_SUPPORT_NAMESPACE::default_map_entry_parser<K, V>,
        PNP_SUPPORT_NAMESPACE::map_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END

#endif