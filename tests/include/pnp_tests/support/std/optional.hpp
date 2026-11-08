#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pnp_tests
{
    using optional_int_pnp = pnp::default_printer<std::optional<int>>;
    using optional_int_parser = pnp::default_parser<std::optional<int>>;

    using nested_optional = std::optional<std::optional<int>>;
    using nested_optional_parser = pnp::default_parser<nested_optional>;

    template <typename O>
    void
        check_optional_round_trip(const O& value_arg, std::u8string_view expected_arg)
    {
        const auto text = pnp::to_u8string(value_arg);
        CHECK(text == expected_arg);
        const auto parsed = pnp::try_parse<O>(text);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value_arg);
    }

    // An empty token that starts with an opening brace.
    struct braced_nullopt
    {
        static constexpr std::u8string_view nullopt_id = u8"{none}";
    };

    // The empty token starts with the opening delimiter: the parser couldn't tell them apart.
    struct confusable_optional_policy
    {
        using empty_defaults = braced_nullopt;
        static constexpr std::u8string_view open_delimiter = u8"{";
        static constexpr std::u8string_view close_delimiter = u8"}";
    };
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("optional: values in braces, and nullopt", "[optional]")
{
    check_optional_round_trip(std::optional<int>{5}, u8"{5}");
    check_optional_round_trip(std::optional<int>{}, u8"nullopt");
    check_optional_round_trip(std::optional<int>{-3}, u8"{-3}");
    check_optional_round_trip(std::optional<std::string>{"a,b"}, u8"{\"a,b\"}");
    check_optional_round_trip(std::optional<std::vector<int>>{std::vector<int>{1, 2}}, u8"{[1,2]}");
    check_optional_round_trip(std::optional<bool>{false}, u8"{false}");
}

TEST_CASE("optional: nested optionals keep all three states apart", "[optional]")
{
    check_optional_round_trip(nested_optional{}, u8"nullopt");
    check_optional_round_trip(nested_optional{ std::optional<int>{} }, u8"{nullopt}");
    check_optional_round_trip(nested_optional{ std::optional<int>{5} }, u8"{{5}}");

    // The middle state really is an engaged outer optional holding an empty one.
    const auto parsed = pnp::try_parse<nested_optional>(u8"{nullopt}");
    REQUIRE(succeeded(parsed));
    REQUIRE(parsed.value.has_value());
    CHECK(not parsed.value->has_value());
}

TEST_CASE("optional: inside containers", "[optional]")
{
    const std::vector<std::optional<int>> values{ 1, std::nullopt, 3 };
    check_optional_round_trip(values, u8"[{1},nullopt,{3}]");
}

TEST_CASE("optional: parsing replaces what was there", "[optional]")
{
    // Parsing an empty optional into an engaged one empties it, and the reverse.
    optional_int_parser parser;
    std::optional<int>  target{ 7 };
    pnp::parser_input   empty_input(u8"nullopt");
    REQUIRE(empty_input.run_parser(parser, target).succeeded());
    CHECK(not target.has_value());

    pnp::parser_input value_input(u8"{9}");
    REQUIRE(value_input.run_parser(parser, target).succeeded());
    CHECK(target == 9);

    // A failed parse leaves it as it was.
    pnp::parser_input bad_input(u8"{x}");
    REQUIRE(bad_input.run_parser(parser, target).failed());
    CHECK(target == 9);
}

TEST_CASE("optional: configuration", "[optional]")
{
    // A custom empty token, delimiters, and value.
    const optional_int_pnp none({ .empty = optional_int_pnp::empty_part_type({.nullopt_id = u8"none"}) });
    CHECK(pnp::to_u8string(none, std::optional<int>{}) == u8"none");

    const optional_int_pnp angled({ .open_delimiter = u8"<", .close_delimiter = u8">" });
    CHECK(pnp::to_u8string(angled, std::optional<int>{5}) == u8"<5>");

    const optional_int_pnp plus({ .value = optional_int_pnp::value_part_type({.use_plus_for_positive_numbers = true}) });
    CHECK(pnp::to_u8string(plus, std::optional<int>{5}) == u8"{+5}");

    // Empty delimiters are allowed, for values that can't print the empty token.
    const optional_int_pnp bare({ .open_delimiter = u8"", .close_delimiter = u8"" });
    CHECK(pnp::to_u8string(bare, std::optional<int>{5}) == u8"5");
    const optional_int_parser bare_parser({ .open_delimiter = u8"", .close_delimiter = u8"" });
    const auto bare_parsed = pnp::try_parse<std::optional<int>>(u8"5", bare_parser);
    REQUIRE(succeeded(bare_parsed));
    CHECK(bare_parsed.value == 5);
}

TEST_CASE("optional: the opening delimiter and empty token must be distinguishable", "[optional]")
{
    CHECK(invalid_argument_message(
        [] { optional_int_pnp p({ .empty = optional_int_pnp::empty_part_type({.nullopt_id = u8"{none}"}) }); })
        == "optional_pnp_options open delimiter and empty token must not be prefixes of each other");
    CHECK_THROWS_AS(optional_int_pnp({ .open_delimiter = u8"nullopt!" }), std::invalid_argument);
    CHECK_THROWS_AS(optional_int_pnp({ .open_delimiter = u8"null" }), std::invalid_argument);

    // Fine when they differ from the first character, or the delimiters are empty.
    CHECK_NOTHROW(optional_int_pnp({ .empty = optional_int_pnp::empty_part_type({.nullopt_id = u8"<none>"}) }));
    CHECK_NOTHROW(optional_int_pnp({
        .empty = optional_int_pnp::empty_part_type({.nullopt_id = u8"{none}"}),
        .open_delimiter = u8"",
        .close_delimiter = u8"",
        }));

    // Invalid delimiters, and an invalid empty token.
    CHECK(invalid_argument_message([] { optional_int_pnp p({ .open_delimiter = u8" {" }); })
        == "optional_pnp_options delimiters must not start or end with whitespace");
    CHECK(invalid_argument_message([] { optional_int_pnp::empty_part_type p({ .nullopt_id = u8"" }); })
        == "nullopt_pnp_options nullopt id must be non-empty");
}

TEST_CASE("optional: parse errors", "[optional]")
{
    const auto neither = pnp::try_parse<std::optional<int>>(u8"5");
    CHECK(first_message(neither) == u8"Expected the empty token \"nullopt\" or the opening delimiter \"{\".");

    const auto bad = pnp::try_parse<std::optional<int>>(u8"{x}");
    CHECK(first_message(bad) == u8"Expected a valid integer, but no valid digits were found.");

    const auto no_close = pnp::try_parse<std::optional<int>>(u8"{5");
    CHECK(first_message(no_close) == u8"Expected the closing delimiter \"}\".");

    CHECK(not succeeded(pnp::try_parse<std::optional<int>>(u8"null")));
    CHECK(not succeeded(pnp::try_parse<std::optional<int>>(u8"nulloptx")));
    CHECK(not succeeded(pnp::try_parse<std::optional<int>>(u8"{5}x")));
    CHECK(not succeeded(pnp::try_parse<std::optional<int>>(u8"")));
}

TEST_CASE("optional: never fixed width", "[optional][fixed_width]")
{
    CHECK(not optional_int_pnp{}.fixed_output_width().has_value());
    using optional_bits = pnp::default_printer<std::vector<std::optional<int>>>;
    CHECK_THROWS_AS(optional_bits({ .separator = u8"" }), std::invalid_argument);
}

TEST_CASE("nullopt: a token of its own", "[optional][nullopt]")
{
    using nullopt_pnp = pnp::support::nullopt_pnp<>;

    CHECK(pnp::to_u8string(std::nullopt) == u8"nullopt");
    CHECK(nullopt_pnp{}.token() == u8"nullopt");
    CHECK(nullopt_pnp({ .nullopt_id = u8"∅" }).token() == u8"∅");

    // Always the token, so always its length.
    CHECK(nullopt_pnp{}.fixed_output_width() == 7);
    CHECK(nullopt_pnp({ .nullopt_id = u8"∅" }).fixed_output_width() == 3);

    // Parsing accepts exactly the token.
    std::nullopt_t    target = std::nullopt;
    pnp::parser_input good(u8"nullopt");
    CHECK(good.run_parser(nullopt_pnp{}, target).succeeded());
    pnp::parser_input bad(u8"null");
    CHECK(bad.run_parser(nullopt_pnp{}, target).failed());

    // Invalid tokens.
    CHECK_THROWS_AS(nullopt_pnp({ .nullopt_id = u8" nullopt" }), std::invalid_argument);
    CHECK_THROWS_AS(nullopt_pnp({ .nullopt_id = half_a_character }), std::invalid_argument);
}

// --- Policies ------------------------------------------------------------------------
static_assert(pnp::support::valid_optional_pnp_c<pnp::default_printer<int>, pnp::support::optional_pnp_default>);
static_assert(not pnp::support::valid_optional_pnp_c<pnp::default_printer<int>, confusable_optional_policy>);
static_assert(pnp::support::valid_nullopt_pnp_c<pnp::support::nullopt_pnp_default>);
static_assert(pnp::support::internal::token_pnp_c<pnp::support::nullopt_pnp<>>);

// --- Defaults ------------------------------------------------------------------------
static_assert(pnp::default_printable_c<std::optional<int>>);
static_assert(pnp::default_parseable_c<std::optional<int>>);
static_assert(pnp::default_parseable_c<nested_optional>);
static_assert(pnp::default_printable_c<std::nullopt_t>);

// --- The exact-type guard ------------------------------------------------------------
static_assert(not pnp::printer_c<optional_int_pnp, std::optional<long>>);
static_assert(not pnp::printer_c<optional_int_pnp, int>);
static_assert(not pnp::printer_c<optional_int_pnp, std::nullopt_t>);