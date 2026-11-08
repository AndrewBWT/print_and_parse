#pragma once

#include "pnp/support/internal/std/inplace_vector.hpp"

#include <cstddef>
#include <version>

#if defined(__cpp_lib_inplace_vector)
#include <inplace_vector>

PNP_NS_BEGIN

// std::inplace_vector<T, N>: a sequence, [1,2,3], of at most N elements.
template <typename T, std::size_t N>
    requires default_printable_c<T>
struct default_printer<std::inplace_vector<T, N>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::inplace_vector<T, N>, default_printer<T>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::inplace_vector<T, N>, default_printer<T>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T, std::size_t N>
    requires default_parseable_c<T>
struct default_parser<std::inplace_vector<T, N>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::inplace_vector<T, N>, default_parser<T>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::inplace_vector<T, N>, default_parser<T>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END

#endif