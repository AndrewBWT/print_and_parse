#pragma once

#include "pnp/support/internal/std/system_error.hpp"

#include <system_error>

PNP_NS_BEGIN

// std::error_code and std::error_condition: the category and value, system:2.
template <>
struct default_printer<std::error_code> : public PNP_SUPPORT_NAMESPACE::generic_error_code_printer<std::error_code>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_error_code_printer<std::error_code>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END