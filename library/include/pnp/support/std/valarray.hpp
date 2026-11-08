#pragma once

#include "pnp/support/internal/std/valarray.hpp"

#include <valarray>

PNP_NS_BEGIN

// std::valarray<T>: a sequence, [1,2,3].
template <typename T>
    requires default_printable_c<T>
struct default_printer<std::valarray<T>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::valarray<T>, default_printer<T>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::valarray<T>, default_printer<T>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T>
    requires default_parseable_c<T>
struct default_parser<std::valarray<T>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::valarray<T>, default_parser<T>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::valarray<T>, default_parser<T>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END