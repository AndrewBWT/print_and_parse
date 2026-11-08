#pragma once
#include "pnp/support/internal/core_language/chars.hpp"

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <> inline constexpr bool enable_default_char_pnp<char> = true;
template <> inline constexpr bool enable_default_char_pnp<char8_t> = true;
template <> inline constexpr bool enable_default_char_pnp<char16_t> = true;
template <> inline constexpr bool enable_default_char_pnp<char32_t> = true;
template <> inline constexpr bool enable_default_char_pnp<wchar_t> = true;
PNP_SUPPORT_NS_END
PNP_NS_END