#pragma once

#include "pnp/support/internal/sequences.hpp"

#include <cstddef>
#include <span>
#include <type_traits>
#include <optional>
#include "pnp/support/internal/std/span.hpp"

PNP_NS_BEGIN
// std::span: a sequence, [1,2,3]. Print-only: a span refers to storage elsewhere, and has no
// appender, so the sequence pnp's run_parser isn't available for it. std::span<char> is a
// sequence of characters, like std::array<char, N>.
template <typename T, std::size_t Extent>
    requires default_printable_c<std::remove_cv_t<T>>
struct default_printer<std::span<T, Extent>>
    : public PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::span<T, Extent>, default_printer<std::remove_cv_t<T>>>
{
    using base_t
        = PNP_SUPPORT_NAMESPACE::generic_sequence_pnp<std::span<T, Extent>, default_printer<std::remove_cv_t<T>>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END