#pragma once

#include "pnp/support/internal/std/complex.hpp"

#include <complex>

PNP_NS_BEGIN

// std::complex<T>: (real,imaginary), as the standard's streams write it.
template <typename T>
    requires default_printable_c<T>
struct default_printer<std::complex<T>> : public PNP_SUPPORT_NAMESPACE::generic_complex_pnp<std::complex<T>, default_printer<T>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_complex_pnp<std::complex<T>, default_printer<T>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T>
    requires default_parseable_c<T>
struct default_parser<std::complex<T>> : public PNP_SUPPORT_NAMESPACE::generic_complex_pnp<std::complex<T>, default_parser<T>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_complex_pnp<std::complex<T>, default_parser<T>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END