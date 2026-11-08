#pragma once

#include "pnp/support/internal/sequences.hpp"

#include <cstddef>
#include <optional>
#include <span>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN
// A span with a static extent always has that many elements.
template <typename T, std::size_t Extent>
requires (Extent != std::dynamic_extent)
inline constexpr std::optional<std::size_t>
    sequence_fixed_count_v<std::span<T, Extent>> = Extent;
PNP_SUPPORT_NS_END
PNP_NS_END