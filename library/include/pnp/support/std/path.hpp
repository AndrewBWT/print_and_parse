#pragma once

#include "pnp/support/internal/std/path.hpp"

#include <filesystem>

PNP_NS_BEGIN

// std::filesystem::path: a quoted string, with forward slashes: "docs/report.txt".
template <>
struct default_printer<std::filesystem::path> : public PNP_SUPPORT_NAMESPACE::generic_path_pnp<>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_path_pnp<>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <>
struct default_parser<std::filesystem::path> : public PNP_SUPPORT_NAMESPACE::generic_path_pnp<>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_path_pnp<>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END