#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <source_location>
#include <string>

TEST_CASE("source_location: file, line and column", "[source_location]")
{
    const std::source_location here = std::source_location::current();
    using location_printer = pnp::default_printer<std::source_location>;

    // The position, without the directories: exact text depends on the compiler's file name.
    const auto short_text = pnp::to_u8string(location_printer({ .show_column = false, .file_name_only = true }), here);
    CHECK(short_text == u8"source_location.hpp:" + std::u8string(pnp::to_u8string(static_cast<unsigned long long>(here.line()))));

    // The default has the full file name and the column.
    const auto full_text = pnp::to_u8string(here);
    CHECK(full_text.ends_with(u8":" + pnp::to_u8string(static_cast<unsigned long long>(here.line())) + u8":"
        + pnp::to_u8string(static_cast<unsigned long long>(here.column()))));

    // With the function's name, whatever the compiler calls it.
    const auto with_function = pnp::to_u8string(location_printer({ .show_function = true }), here);
    CHECK(with_function.find(u8" in ") != std::u8string::npos);

    CHECK_THROWS_AS(location_printer({ .separator = u8"" }), std::invalid_argument);
}

static_assert(pnp::default_printable_c<std::source_location>);
static_assert(not pnp::default_parseable_c<std::source_location>, "print-only");