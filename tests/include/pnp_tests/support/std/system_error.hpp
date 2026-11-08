#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <string>
#include <system_error>

TEST_CASE("error codes: the category and the value", "[system_error]")
{
    CHECK(pnp::to_u8string(std::make_error_code(std::errc::no_such_file_or_directory)) == u8"generic:2");
    CHECK(pnp::to_u8string(std::error_code{}) == u8"system:0");
    CHECK(pnp::to_u8string(std::make_error_condition(std::errc::invalid_argument)) == u8"generic:22");

    // A different separator.
    using code_printer = pnp::default_printer<std::error_code>;
    CHECK(pnp::to_u8string(code_printer({ .separator = u8"/" }), std::make_error_code(std::errc::invalid_argument))
        == u8"generic/22");

    // With the message: whatever the category says, after the code.
    const std::error_code code = std::make_error_code(std::errc::invalid_argument);
    const auto            text = pnp::to_u8string(code_printer({ .show_message = true }), code);
    const std::string     message = code.message();
    CHECK(text == u8"generic:22 " + std::u8string(message.begin(), message.end()));

    CHECK_THROWS_AS(code_printer({ .separator = u8" :" }), std::invalid_argument);
}

static_assert(pnp::default_printable_c<std::error_code>);
static_assert(pnp::default_printable_c<std::error_condition>);
static_assert(not pnp::default_parseable_c<std::error_code>, "print-only");
static_assert(not pnp::printer_c<pnp::default_printer<std::error_code>, std::error_condition>);