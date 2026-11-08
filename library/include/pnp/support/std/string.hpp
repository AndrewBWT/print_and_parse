#pragma once
#include "pnp/support/internal/std/string.hpp"
#include <string>
#include <unicode_bridge.hpp>
PNP_NS_BEGIN

template <typename CharT, typename Traits, typename Alloc>
requires unicode_bridge::is_char_type_c<CharT>
struct default_printer<std::basic_string<CharT, Traits, Alloc>>
    : public PNP_SUPPORT_NAMESPACE::generic_string_pnp<
          std::basic_string<CharT, Traits, Alloc>,
          PNP_SUPPORT_NAMESPACE::string_pnp_default>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_string_pnp<
        std::basic_string<CharT, Traits, Alloc>,
        PNP_SUPPORT_NAMESPACE::string_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename CharT, typename Traits, typename Alloc>
requires unicode_bridge::is_char_type_c<CharT>
struct default_parser<std::basic_string<CharT, Traits, Alloc>>
    : public PNP_SUPPORT_NAMESPACE::generic_string_pnp<
          std::basic_string<CharT, Traits, Alloc>,
          PNP_SUPPORT_NAMESPACE::string_pnp_default>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_string_pnp<
        std::basic_string<CharT, Traits, Alloc>,
        PNP_SUPPORT_NAMESPACE::string_pnp_default>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END