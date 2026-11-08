#pragma once
#include "pnp/support/internal/core_language/integers.hpp"

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN
template <>
inline constexpr bool enable_default_integer_pnp<unsigned char> = true;
template <>
inline constexpr bool enable_default_integer_pnp<unsigned short> = true;
template <>
inline constexpr bool enable_default_integer_pnp<unsigned int> = true;
template <>
inline constexpr bool enable_default_integer_pnp<unsigned long> = true;
template <>
inline constexpr bool enable_default_integer_pnp<unsigned long long> = true;

template <>
inline constexpr bool enable_default_integer_pnp<signed char> = true;
template <>
inline constexpr bool enable_default_integer_pnp<signed short> = true;
template <>
inline constexpr bool enable_default_integer_pnp<signed int> = true;
template <>
inline constexpr bool enable_default_integer_pnp<signed long> = true;
template <>
inline constexpr bool enable_default_integer_pnp<signed long long> = true;
PNP_SUPPORT_NS_END
PNP_NS_END