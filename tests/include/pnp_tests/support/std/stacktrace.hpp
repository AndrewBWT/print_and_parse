#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <string>
#include <version>

#if defined(__cpp_lib_stacktrace)
#include <stacktrace>

TEST_CASE("stacktrace: entries on one line, in brackets", "[stacktrace]")
{
    // The exact text depends on the platform and its symbols, so only the shape is checked.
    const std::stacktrace trace = std::stacktrace::current();
    const auto            text = pnp::to_u8string(trace);
    CHECK(text.starts_with(u8"["));
    CHECK(text.ends_with(u8"]"));
    CHECK(text.find(u8'\n') == std::u8string::npos);

    // Each entry prints as on its own.
    if (not trace.empty())
    {
        CHECK(text.find(pnp::to_u8string(trace[0])) != std::u8string::npos);
    }

    // An empty trace is just the brackets.
    CHECK(pnp::to_u8string(std::stacktrace{}) == u8"[]");
}
static_assert(pnp::default_printable_c<std::stacktrace>);
static_assert(pnp::default_printable_c<std::stacktrace_entry>);
static_assert(not pnp::default_parseable_c<std::stacktrace>, "print-only");

#endif