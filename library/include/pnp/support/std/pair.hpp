#pragma once
#include "pnp/support/internal/tuple_like.hpp"

PNP_NS_BEGIN
template <typename T1, typename T2>
    requires default_printable_c<std::remove_cv_t<T1>>&& default_printable_c<T2>
struct default_printer<std::pair<T1, T2>>
    : public PNP_SUPPORT_NAMESPACE::generic_tuple_pnp<
    std::pair<T1, T2>, std::tuple<default_printer<std::remove_cv_t<T1>>, default_printer<T2>>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_tuple_pnp<
        std::pair<T1, T2>, std::tuple<default_printer<std::remove_cv_t<T1>>, default_printer<T2>>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T1, typename T2>
    requires default_parseable_c<T1>&& default_parseable_c<T2>
struct default_parser<std::pair<T1, T2>>
    : public PNP_SUPPORT_NAMESPACE::generic_tuple_pnp<std::pair<T1, T2>, std::tuple<default_parser<T1>, default_parser<T2>>>
{
    using base_t
        = PNP_SUPPORT_NAMESPACE::generic_tuple_pnp<std::pair<T1, T2>, std::tuple<default_parser<T1>, default_parser<T2>>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};
PNP_NS_END