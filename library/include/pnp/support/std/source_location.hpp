#pragma once

#include "pnp/support/internal/std/source_location.hpp"

#include <source_location>

PNP_NS_BEGIN

// std::source_location: file:line:column.
template <>
struct default_printer<std::source_location> : public PNP_SUPPORT_NAMESPACE::source_location_printer<>
{
    using base_t = PNP_SUPPORT_NAMESPACE::source_location_printer<>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END