#pragma once
#include "pnp/support/internal/core_language/bool.hpp"

PNP_NS_BEGIN

template <typename T>
requires std::same_as<T, bool>
struct default_printer<T>
    : public PNP_SUPPORT_NAMESPACE::bool_pnp<
          PNP_SUPPORT_NAMESPACE::bool_pnp_vals_words_lc>
{
    using base_t = bool_pnp<PNP_SUPPORT_NAMESPACE::bool_pnp_vals_words_lc>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T>
requires std::same_as<T, bool>
struct default_parser<T>
    : public PNP_SUPPORT_NAMESPACE::bool_pnp<
          PNP_SUPPORT_NAMESPACE::bool_pnp_vals_words_lc>
{
    using base_t = bool_pnp<PNP_SUPPORT_NAMESPACE::bool_pnp_vals_words_lc>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END