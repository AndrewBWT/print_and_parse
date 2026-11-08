#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <version>

#if defined(__cpp_lib_expected)
#include <expected>

namespace pnp_tests
{
    using int_or_string = std::expected<int, std::string>;
    using int_or_string_printer = pnp::default_printer<int_or_string>;
    using int_or_string_parser = pnp::default_parser<int_or_string>;

    template <typename X>
    void
        check_expected_round_trip(const X& value_arg, std::u8string_view expected_text_arg)
    {
        const auto text = pnp::to_u8string(value_arg);
        CHECK(text == expected_text_arg);
        const auto parsed = pnp::try_parse<X>(text);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value_arg);
    }
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("expected: values and errors, each marked", "[expected]")
{
    check_expected_round_trip(int_or_string{ 1 }, u8"expected(1)");
    check_expected_round_trip(int_or_string{ std::unexpect, "hello" }, u8"unexpected(\"hello\")");
    check_expected_round_trip(std::expected<std::vector<int>, int>{std::vector<int>{1, 2}}, u8"expected([1,2])");
    check_expected_round_trip(std::expected<std::vector<int>, int>{std::unexpect, 404}, u8"unexpected(404)");
}

TEST_CASE("expected: void values", "[expected]")
{
    check_expected_round_trip(std::expected<void, int>{}, u8"expected()");
    check_expected_round_trip(std::expected<void, int>{std::unexpect, 7}, u8"unexpected(7)");
    CHECK(not succeeded(pnp::try_parse<std::expected<void, int>>(u8"expected(1)")));
}

TEST_CASE("expected: nested expecteds keep all three states apart", "[expected]")
{
    using nested = std::expected<std::expected<int, int>, int>;
    check_expected_round_trip(nested{ std::expected<int, int>{1} }, u8"expected(expected(1))");
    check_expected_round_trip(nested{ std::expected<int, int>{std::unexpect, 1} }, u8"expected(unexpected(1))");
    check_expected_round_trip(nested{ std::unexpect, 1 }, u8"unexpected(1)");
}

TEST_CASE("expected: inside containers", "[expected]")
{
    const std::vector<std::expected<int, int>> values{ 1, std::unexpected(2), 3 };
    check_expected_round_trip(values, u8"[expected(1),unexpected(2),expected(3)]");
}

TEST_CASE("expected: parsing replaces what was there", "[expected]")
{
    // An error parsed over a value, and the reverse.
    const int_or_string_parser parser;
    int_or_string              target{ 5 };
    pnp::parser_input          error_input(u8"unexpected(\"e\")");
    REQUIRE(error_input.run_parser(parser, target).succeeded());
    REQUIRE(not target.has_value());
    CHECK(target.error() == "e");

    pnp::parser_input value_input(u8"expected(9)");
    REQUIRE(value_input.run_parser(parser, target).succeeded());
    CHECK(target == 9);

    // A failed parse leaves it as it was.
    pnp::parser_input bad_input(u8"expected(x)");
    CHECK(bad_input.run_parser(parser, target).failed());
    CHECK(target == 9);
}

TEST_CASE("expected: configuration", "[expected]")
{
    // Different markers on each side.
    const int_or_string_printer result_style({
        .value_open_delimiter = u8"ok(",
        .error_open_delimiter = u8"err(",
        });
    CHECK(pnp::to_u8string(result_style, int_or_string{ 1 }) == u8"ok(1)");
    CHECK(pnp::to_u8string(result_style, int_or_string{ std::unexpect, "e" }) == u8"err(\"e\")");

    // A configured part.
    const int_or_string_printer plus({ .value = int_or_string_printer::value_part_type({.use_plus_for_positive_numbers = true}) });
    CHECK(pnp::to_u8string(plus, int_or_string{ 1 }) == u8"expected(+1)");

    // One side may be unmarked, read when the other's marker isn't there.
    const int_or_string_printer bare_value({ .value_open_delimiter = u8"", .value_close_delimiter = u8"" });
    CHECK(pnp::to_u8string(bare_value, int_or_string{ 1 }) == u8"1");
    const int_or_string_parser bare_parser({ .value_open_delimiter = u8"", .value_close_delimiter = u8"" });
    const auto bare = pnp::try_parse<int_or_string>(u8"1", bare_parser);
    REQUIRE(succeeded(bare));
    CHECK(bare.value == 1);
    const auto bare_error = pnp::try_parse<int_or_string>(u8"unexpected(\"e\")", bare_parser);
    REQUIRE(succeeded(bare_error));
    CHECK(bare_error.value.error() == "e");
}

TEST_CASE("expected: the two opening delimiters must be distinguishable", "[expected]")
{
    CHECK(invalid_argument_message([] { int_or_string_printer p({ .value_open_delimiter = u8"unexpected" }); })
        == "expected_pnp_options value and error opening delimiters must not be prefixes of each other, and at "
        "most one may be empty");
    CHECK_THROWS_AS(int_or_string_printer({ .value_open_delimiter = u8"(", .error_open_delimiter = u8"(!" }),
        std::invalid_argument);
    CHECK_THROWS_AS(int_or_string_printer({ .value_open_delimiter = u8"", .error_open_delimiter = u8"" }),
        std::invalid_argument);

    CHECK(invalid_argument_message([] { int_or_string_printer p({ .error_close_delimiter = u8") " }); })
        == "expected_pnp_options delimiters must not start or end with whitespace");
}

TEST_CASE("expected: parse errors", "[expected]")
{
    const auto neither = pnp::try_parse<int_or_string>(u8"1");
    CHECK(first_message(neither)
        == u8"Expected the value's opening delimiter \"expected(\" or the error's opening delimiter \"unexpected(\".");

    const auto bad_value = pnp::try_parse<int_or_string>(u8"expected(x)");
    CHECK(first_message(bad_value) == u8"Expected a valid integer, but no valid digits were found.");

    const auto no_close = pnp::try_parse<int_or_string>(u8"expected(1");
    CHECK(first_message(no_close) == u8"Expected the closing delimiter \")\".");

    CHECK(not succeeded(pnp::try_parse<int_or_string>(u8"unexpected(1)")));   // the error is a string
    CHECK(not succeeded(pnp::try_parse<int_or_string>(u8"expected(1)x")));
    CHECK(not succeeded(pnp::try_parse<int_or_string>(u8"")));
}

TEST_CASE("expected: never fixed width", "[expected][fixed_width]")
{
    CHECK(not int_or_string_printer{}.fixed_output_width().has_value());
}

// --- Policies and defaults -------------------------------------------------------------
static_assert(pnp::support::valid_expected_pnp_c<pnp::default_printer<int>, pnp::default_printer<std::string>,
    pnp::support::expected_pnp_default>);
static_assert(pnp::default_printable_c<int_or_string>);
static_assert(pnp::default_parseable_c<int_or_string>);
static_assert(pnp::default_printable_c<std::expected<void, int>>);
static_assert(pnp::default_parseable_c<std::expected<void, int>>);

// --- The exact-type guard ------------------------------------------------------------
static_assert(not pnp::printer_c<int_or_string_printer, std::expected<long, std::string>>);
static_assert(not pnp::printer_c<int_or_string_printer, int>);
TEST_CASE("unexpected: an error on its own", "[expected][unexpected]")
{
    check_expected_round_trip(std::unexpected<std::string>("hello"), u8"unexpected(\"hello\")");
    check_expected_round_trip(std::unexpected<int>(404), u8"unexpected(404)");

    // Exactly as the error side of an expected prints.
    CHECK(pnp::to_u8string(std::unexpected<std::string>("e")) == pnp::to_u8string(int_or_string{ std::unexpect, "e" }));

    // Configured, and parse errors.
    using unexpected_pnp = pnp::default_printer<std::unexpected<int>>;
    CHECK(pnp::to_u8string(unexpected_pnp({ .error_open_delimiter = u8"err(" }), std::unexpected<int>(1)) == u8"err(1)");
    CHECK(first_message(pnp::try_parse<std::unexpected<int>>(u8"1"))
        == u8"Expected the error's opening delimiter \"unexpected(\".");
    CHECK(not succeeded(pnp::try_parse<std::unexpected<int>>(u8"unexpected(1")));
}

static_assert(pnp::parse_constructible_c<std::unexpected<int>>);
static_assert(pnp::default_parseable_c<std::unexpected<std::string>>);
#endif