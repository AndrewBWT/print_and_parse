#pragma once
#include "pnp/support/internal/core_language/floats.hpp"
PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN
template <>
inline constexpr bool enable_default_float_pnp<float> = true;
template <>
inline constexpr bool enable_default_float_pnp<double> = true;
template <>
inline constexpr bool enable_default_float_pnp<long double> = true;
PNP_SUPPORT_NS_END
PNP_NS_END