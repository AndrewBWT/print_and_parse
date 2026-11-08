#pragma once

#include "pnp/support/internal/std/expected.hpp"

#include <type_traits>
#include <version>

#if defined(__cpp_lib_expected)
#include <expected>

PNP_NS_BEGIN

PNP_SUPPORT_NS_BEGIN
PNP_INTERNAL_NS_BEGIN
// The value part a default uses: the default for the value type, or the placeholder for void.
template <typename T, template <typename> typename Default>
struct default_expected_value_part
{
    using type = Default<std::remove_cv_t<T>>;
};

template <template <typename> typename Default>
struct default_expected_value_part<void, Default>
{
    using type = no_value_pnp;
};

template <typename T>
concept default_expected_value_printable_c = std::is_void_v<T> || default_printable_c<std::remove_cv_t<T>>;

template <typename T>
concept default_expected_value_parseable_c = std::is_void_v<T> || default_parseable_c<std::remove_cv_t<T>>;
PNP_INTERNAL_NS_END
PNP_SUPPORT_NS_END
// std::unexpected<E>: unexpected("e"), as an expected's error prints.
template <typename E>
    requires default_printable_c<E>
struct default_printer<std::unexpected<E>>
    : public PNP_SUPPORT_NAMESPACE::generic_unexpected_pnp<std::unexpected<E>, default_printer<E>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_unexpected_pnp<std::unexpected<E>, default_printer<E>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename E>
    requires default_parseable_c<E>
struct default_parser<std::unexpected<E>>
    : public PNP_SUPPORT_NAMESPACE::generic_unexpected_pnp<std::unexpected<E>, default_parser<E>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_unexpected_pnp<std::unexpected<E>, default_parser<E>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};
// std::expected<T, E>: expected(1), or unexpected("e").
template <typename T, typename E>
    requires PNP_SUPPORT_NAMESPACE::PNP_INTERNAL_NAMESPACE::default_expected_value_printable_c<T>
&& default_printable_c<std::remove_cv_t<E>>
struct default_printer<std::expected<T, E>>
    : public PNP_SUPPORT_NAMESPACE::generic_expected_pnp<
    std::expected<T, E>,
    typename PNP_SUPPORT_NAMESPACE::PNP_INTERNAL_NAMESPACE::default_expected_value_part<T, default_printer>::type,
    default_printer<std::remove_cv_t<E>>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_expected_pnp<
        std::expected<T, E>,
        typename PNP_SUPPORT_NAMESPACE::PNP_INTERNAL_NAMESPACE::default_expected_value_part<T, default_printer>::type,
        default_printer<std::remove_cv_t<E>>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T, typename E>
    requires PNP_SUPPORT_NAMESPACE::PNP_INTERNAL_NAMESPACE::default_expected_value_parseable_c<T>
&& default_parseable_c<std::remove_cv_t<E>>
struct default_parser<std::expected<T, E>>
    : public PNP_SUPPORT_NAMESPACE::generic_expected_pnp<
    std::expected<T, E>,
    typename PNP_SUPPORT_NAMESPACE::PNP_INTERNAL_NAMESPACE::default_expected_value_part<T, default_parser>::type,
    default_parser<std::remove_cv_t<E>>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_expected_pnp<
        std::expected<T, E>,
        typename PNP_SUPPORT_NAMESPACE::PNP_INTERNAL_NAMESPACE::default_expected_value_part<T, default_parser>::type,
        default_parser<std::remove_cv_t<E>>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END

#endif