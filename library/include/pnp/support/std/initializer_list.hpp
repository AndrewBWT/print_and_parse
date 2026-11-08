#pragma once

#include "pnp/support/internal/sequences.hpp"

#include <initializer_list>

PNP_NS_BEGIN

// std::initializer_list<T>: a sequence, [1,2,3]. Print-only: it can't be built from parsed
// elements, so it has no appender.
template <typename T>
    requires default_printable_c<T>
struct default_printer<std::initializer_list<T>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::initializer_list<T>, default_printer<T>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::initializer_list<T>, default_printer<T>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END