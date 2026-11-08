#pragma once

#include "pnp/support/internal/sequences.hpp"

#include <version>

#if defined(__cpp_lib_flat_set)
#include <flat_set>

PNP_NS_BEGIN

template <typename K, typename C, typename KC>
    requires default_printable_c<K>
struct default_printer<std::flat_multiset<K, C, KC>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::flat_multiset<K, C, KC>, default_printer<K>, PNP_SUPPORT_NAMESPACE::set_pnp_default>
{
    using base_t
        = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::flat_multiset<K, C, KC>, default_printer<K>, PNP_SUPPORT_NAMESPACE::set_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename K, typename C, typename KC>
    requires default_parseable_c<K>
struct default_parser<std::flat_multiset<K, C, KC>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::flat_multiset<K, C, KC>, default_parser<K>, PNP_SUPPORT_NAMESPACE::set_pnp_default>
{
    using base_t
        = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::flat_multiset<K, C, KC>, default_parser<K>, PNP_SUPPORT_NAMESPACE::set_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END

#endif