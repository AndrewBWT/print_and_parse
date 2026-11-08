

#pragma once
#include "pnp/support/internal/core_language/nullptr.hpp"

PNP_NS_BEGIN
template <typename T>
    requires std::same_as<T, std::nullptr_t>
struct default_printer<T> : public PNP_SUPPORT_NAMESPACE::nullptr_pnp<PNP_SUPPORT_NAMESPACE::null_id_default_base>
{
    using base_t = PNP_SUPPORT_NAMESPACE::nullptr_pnp<PNP_SUPPORT_NAMESPACE::null_id_default_base>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T>
    requires std::same_as<T, std::nullptr_t>
struct default_parser<T> : public PNP_SUPPORT_NAMESPACE::nullptr_pnp<PNP_SUPPORT_NAMESPACE::null_id_default_base>
{
    using base_t = PNP_SUPPORT_NAMESPACE::nullptr_pnp<PNP_SUPPORT_NAMESPACE::null_id_default_base>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};
PNP_NS_END