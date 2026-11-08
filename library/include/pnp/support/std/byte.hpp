#pragma once

#include "pnp/support/internal/fixed_width_pnp.hpp"

#include <concepts>
#include <cstddef>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN
// Bytes print as two hex digits: "0x2a".
struct fixed_width_pnp_byte_hex : fixed_width_pnp_unsigned_hex
{
    static constexpr bool fixed_size_output = true;
};
PNP_SUPPORT_NS_END

template <typename T>
    requires std::same_as<T, std::byte>
struct default_printer<T>
    : public PNP_SUPPORT_NAMESPACE::fixed_width_pnp<T, PNP_SUPPORT_NAMESPACE::fixed_width_pnp_byte_hex>
{
    using base_t = PNP_SUPPORT_NAMESPACE::fixed_width_pnp<T, PNP_SUPPORT_NAMESPACE::fixed_width_pnp_byte_hex>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T>
    requires std::same_as<T, std::byte>
struct default_parser<T>
    : public PNP_SUPPORT_NAMESPACE::fixed_width_pnp<T, PNP_SUPPORT_NAMESPACE::fixed_width_pnp_byte_hex>
{
    using base_t = PNP_SUPPORT_NAMESPACE::fixed_width_pnp<T, PNP_SUPPORT_NAMESPACE::fixed_width_pnp_byte_hex>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END