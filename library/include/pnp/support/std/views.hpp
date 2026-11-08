#pragma once

#include "pnp/support/internal/std/ranges.hpp"

#include <cstddef>
#include <ranges>
#include <version>

PNP_NS_BEGIN
// The standard views.
template <typename R> inline constexpr bool enable_default_range_printer<std::ranges::ref_view<R>> = true;
template <typename R> inline constexpr bool enable_default_range_printer<std::ranges::owning_view<R>> = true;
template <typename T> inline constexpr bool enable_default_range_printer<std::ranges::empty_view<T>> = true;
template <typename T> inline constexpr bool enable_default_range_printer<std::ranges::single_view<T>> = true;
template <typename W, typename B> inline constexpr bool enable_default_range_printer<std::ranges::iota_view<W, B>> = true;
template <typename I, typename S, std::ranges::subrange_kind K>
inline constexpr bool enable_default_range_printer<std::ranges::subrange<I, S, K>> = true;
template <typename V, typename P> inline constexpr bool enable_default_range_printer<std::ranges::filter_view<V, P>> = true;
template <typename V, typename F> inline constexpr bool enable_default_range_printer<std::ranges::transform_view<V, F>> = true;
template <typename V> inline constexpr bool enable_default_range_printer<std::ranges::take_view<V>> = true;
template <typename V, typename P> inline constexpr bool enable_default_range_printer<std::ranges::take_while_view<V, P>> = true;
template <typename V> inline constexpr bool enable_default_range_printer<std::ranges::drop_view<V>> = true;
template <typename V, typename P> inline constexpr bool enable_default_range_printer<std::ranges::drop_while_view<V, P>> = true;
template <typename V> inline constexpr bool enable_default_range_printer<std::ranges::join_view<V>> = true;
template <typename V> inline constexpr bool enable_default_range_printer<std::ranges::common_view<V>> = true;
template <typename V> inline constexpr bool enable_default_range_printer<std::ranges::reverse_view<V>> = true;
template <typename V, std::size_t N> inline constexpr bool enable_default_range_printer<std::ranges::elements_view<V, N>> = true;

#if defined(__cpp_lib_ranges_zip)
template <typename... Vs> inline constexpr bool enable_default_range_printer<std::ranges::zip_view<Vs...>> = true;
#endif
#if defined(__cpp_lib_ranges_chunk)
template <typename V> inline constexpr bool enable_default_range_printer<std::ranges::chunk_view<V>> = true;
#endif
#if defined(__cpp_lib_ranges_slide)
template <typename V> inline constexpr bool enable_default_range_printer<std::ranges::slide_view<V>> = true;
#endif
#if defined(__cpp_lib_ranges_stride)
template <typename V> inline constexpr bool enable_default_range_printer<std::ranges::stride_view<V>> = true;
#endif
#if defined(__cpp_lib_ranges_enumerate)
template <typename V> inline constexpr bool enable_default_range_printer<std::ranges::enumerate_view<V>> = true;
#endif
#if defined(__cpp_lib_ranges_as_const)
template <typename V> inline constexpr bool enable_default_range_printer<std::ranges::as_const_view<V>> = true;
#endif
PNP_NS_END