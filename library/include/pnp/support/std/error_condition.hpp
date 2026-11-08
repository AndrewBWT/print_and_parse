#pragma once

#include "pnp/support/internal/std/system_error.hpp"

#include <system_error>

PNP_NS_BEGIN

template <>
struct default_printer<std::error_condition> : public PNP_SUPPORT_NAMESPACE::generic_error_code_printer<std::error_condition>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_error_code_printer<std::error_condition>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END