#pragma once

#include "pnp/support/internal/std/stacktrace.hpp"

#include <version>

#if defined(__cpp_lib_stacktrace)
#include <stacktrace>

PNP_NS_BEGIN

// std::stacktrace_entry: description at file:line.
template <>
struct default_printer<std::stacktrace_entry> : public PNP_SUPPORT_NAMESPACE::stacktrace_entry_printer
{
    using base_t = PNP_SUPPORT_NAMESPACE::stacktrace_entry_printer;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

// std::basic_stacktrace: one entry per line, in brackets.
template <typename Allocator>
struct default_printer<std::basic_stacktrace<Allocator>>
    : public PNP_SUPPORT_NAMESPACE::generic_stacktrace_printer<std::basic_stacktrace<Allocator>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_stacktrace_printer<std::basic_stacktrace<Allocator>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END

#endif