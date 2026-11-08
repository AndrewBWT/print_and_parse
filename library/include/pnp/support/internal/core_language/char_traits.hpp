#pragma once
#include "pnp/core.hpp"

#include <type_traits>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN
// Character types whose pointers print as C strings rather than addresses. Both pointer
// defaults below depend on this, so the list and the defaults live together: including
// this file gives a consistent answer for every pointer, and leaving it out gives none.
template <typename T>
inline constexpr bool enable_default_c_string_pnp = false;

template <> inline constexpr bool enable_default_c_string_pnp<char> = true;
template <> inline constexpr bool enable_default_c_string_pnp<wchar_t> = true;
template <> inline constexpr bool enable_default_c_string_pnp<char8_t> = true;
template <> inline constexpr bool enable_default_c_string_pnp<char16_t> = true;
template <> inline constexpr bool enable_default_c_string_pnp<char32_t> = true;
PNP_SUPPORT_NS_END
PNP_NS_END