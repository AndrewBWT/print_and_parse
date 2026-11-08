#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace pnp_tests
{
    using int_or_text = std::variant<int, std::string>;
    using int_or_text_printer = pnp::default_printer<int_or_text>;
    using int_or_text_parser = pnp::default_parser<int_or_text>;

    template <typename V>
    void
        check_variant_round_trip(const V& value_arg, std::u8string_view expected_arg)
    {
        const auto text = pnp::to_u8string(value_arg);
        CHECK(text == expected_arg);
        const auto parsed = pnp::try_parse<V>(text);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value_arg);

        // For a variant, == alone doesn't show the right alternative was chosen when two share a
        // type, so compare the indices too.
        if constexpr (requires { value_arg.index(); })
        {
            CHECK(parsed.value.index() == value_arg.index());
        }
    }

    // A type whose copy throws, for making a variant valueless.
    struct throws_on_copy
    {
        throws_on_copy() = default;
        throws_on_copy(const throws_on_copy&)
        {
            throw std::runtime_error("copy");
        }
    };

    // Prints a throws_on_copy, so a variant holding one can be printed.
    struct throws_on_copy_printer
    {
        void
            run_printer(const std::same_as<throws_on_copy> auto&, pnp::text_sink& sink_arg) const
        {
            sink_arg.write(std::u8string_view(u8"thrower"));
        }
    };
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("variant: the index, then the alternative", "[variant]")
{
    check_variant_round_trip(int_or_text{ 1 }, u8"0:1");
    check_variant_round_trip(int_or_text{ "a,b" }, u8"1:\"a,b\"");

    // Alternatives with their own delimiters.
    using with_optional = std::variant<int, std::optional<int>>;
    check_variant_round_trip(with_optional{ std::in_place_index<0>, 1 }, u8"0:1");
    check_variant_round_trip(with_optional{ std::in_place_index<1>, 5 }, u8"1:{5}");
    check_variant_round_trip(with_optional{ std::in_place_index<1>, std::nullopt }, u8"1:nullopt");
}

TEST_CASE("variant: repeated types keep their index", "[variant]")
{
    using two_ints = std::variant<int, int>;
    check_variant_round_trip(two_ints{ std::in_place_index<0>, 5 }, u8"0:5");
    check_variant_round_trip(two_ints{ std::in_place_index<1>, 5 }, u8"1:5");
}

TEST_CASE("variant: monostate, and nested variants", "[variant]")
{
    using maybe_int = std::variant<std::monostate, int>;
    check_variant_round_trip(maybe_int{}, u8"0:monostate");
    check_variant_round_trip(maybe_int{ 5 }, u8"1:5");

    using nested = std::variant<int, std::variant<int, std::string>>;
    check_variant_round_trip(nested{ std::in_place_index<1>, std::variant<int, std::string>{5} }, u8"1:0:5");

    // Ten or more alternatives: two-digit indices.
    using eleven = std::variant<int, int, int, int, int, int, int, int, int, int, int>;
    check_variant_round_trip(eleven{ std::in_place_index<10>, 7 }, u8"10:7");
}

TEST_CASE("variant: inside containers", "[variant]")
{
    const std::vector<int_or_text> values{ 1, "a", 3 };
    check_variant_round_trip(values, u8"[0:1,1:\"a\",0:3]");
}

TEST_CASE("variant: a valueless variant prints, but can't be parsed", "[variant]")
{
    using thrower_variant = std::variant<int, throws_on_copy>;
    using thrower_printer = pnp::support::generic_variant_pnp<
        thrower_variant,
        std::tuple<pnp::default_printer<int>, throws_on_copy_printer>>;

    thrower_variant      value = 5;
    const throws_on_copy source;
    CHECK_THROWS(value.emplace<1>(source));
    REQUIRE(value.valueless_by_exception());
    CHECK(pnp::to_u8string(thrower_printer{}, value) == u8"valueless");

    const auto parsed = pnp::try_parse<int_or_text>(u8"valueless");
    CHECK(first_message(parsed)
        == u8"A valueless variant (\"valueless\") can't be parsed: it only arises from an exception.");
}

TEST_CASE("variant: configuration", "[variant]")
{
    const int_or_text_printer equals({ .index_separator = u8"=" });
    CHECK(pnp::to_u8string(equals, int_or_text{ 1 }) == u8"0=1");

    const int_or_text_printer plus({
        .alternatives = {int_or_text_printer::part_type<0>({.use_plus_for_positive_numbers = true}),
                         int_or_text_printer::part_type<1>{}},
        });
    CHECK(pnp::to_u8string(plus, int_or_text{ 1 }) == u8"0:+1");

    const int_or_text_parser equals_parser({ .index_separator = u8"=" });
    const auto parsed = pnp::try_parse<int_or_text>(u8"1=\"x\"", equals_parser);
    REQUIRE(succeeded(parsed));
    CHECK(std::get<1>(parsed.value) == "x");
}

TEST_CASE("variant: invalid options", "[variant]")
{
    CHECK(invalid_argument_message([] { int_or_text_printer p({ .index_separator = u8"" }); })
        == "variant_pnp_options index separator must be non-empty");
    CHECK(invalid_argument_message([] { int_or_text_printer p({ .index_separator = u8"1" }); })
        == "variant_pnp_options index separator must not start with a digit");
    CHECK(invalid_argument_message([] { int_or_text_printer p({ .valueless_id = u8"0valueless" }); })
        == "variant_pnp_options valueless id must not start with a digit");
    CHECK_THROWS_AS(int_or_text_printer({ .index_separator = u8" :" }), std::invalid_argument);
    CHECK_THROWS_AS(int_or_text_printer({ .valueless_id = half_a_character }), std::invalid_argument);
}

TEST_CASE("variant: parse errors", "[variant]")
{
    CHECK(first_message(pnp::try_parse<int_or_text>(u8"x")) == u8"Expected the index of a variant's alternative.");
    CHECK(first_message(pnp::try_parse<int_or_text>(u8"2:1"))
        == u8"The index 2 is out of range for a variant of 2 alternatives.");
    CHECK(first_message(pnp::try_parse<int_or_text>(u8"0"))
        == u8"Expected the index separator \":\" after the alternative's index.");
    CHECK(first_message(pnp::try_parse<int_or_text>(u8"0:x"))
        == u8"Expected a valid integer, but no valid digits were found.");
    CHECK(not succeeded(pnp::try_parse<int_or_text>(u8"1:5")));   // the second alternative is a string
    CHECK(not succeeded(pnp::try_parse<int_or_text>(u8"0:1x")));
    CHECK(not succeeded(pnp::try_parse<int_or_text>(u8"")));
}

TEST_CASE("variant: monostate on its own", "[variant][monostate]")
{
    using monostate_pnp = pnp::support::monostate_pnp<>;
    CHECK(pnp::to_u8string(std::monostate{}) == u8"monostate");
    CHECK(monostate_pnp({ .monostate_id = u8"∅" }).token() == u8"∅");
    CHECK(monostate_pnp{}.fixed_output_width() == 9);
    check_variant_round_trip(std::monostate{}, u8"monostate");
    CHECK_THROWS_AS(monostate_pnp({ .monostate_id = u8"" }), std::invalid_argument);
}

TEST_CASE("read ahead: a variant stops where its value does", "[io][read_ahead][variants]")
{
    using int_or_text = std::variant<int, std::u8string>;
    {
        INFO("an int alternative: the value needs the character after it");
        const auto outcome = parse_from_pieces<int_or_text>({ u8"0:5", u8"\n", u8"1:\"x\"", u8"\n" });
        CHECK(outcome.succeeded);
        CHECK(outcome.value == int_or_text{ 5 });
        CHECK(outcome.pieces_read == 2);
    }
    {
        INFO("a string alternative: complete at its closing quote");
        const auto outcome = parse_from_pieces<int_or_text>({ u8"1:\"x\"", u8"\n", u8"0:5", u8"\n" });
        CHECK(outcome.succeeded);
        CHECK(outcome.value == int_or_text{ std::u8string(u8"x") });
        CHECK(outcome.pieces_read == 1);
    }
    {
        INFO("a two-digit index, split across pieces");
        using many = std::variant<int, int, int, int, int, int, int, int, int, int, int, int>;
        const auto outcome = parse_from_pieces<many>({ u8"1", u8"1", u8":", u8"7", u8"\n", u8"0:1", u8"\n" });
        CHECK(outcome.succeeded);
        CHECK(outcome.value.index() == 11);
        CHECK(outcome.pieces_read == 5);
    }
}

// --- Parts, defaults, and the exact-type guard ---------------------------------------
using int_string_parts = std::tuple<pnp::default_printer<int>, pnp::default_printer<std::string>>;
static_assert(pnp::support::variant_parts_print_c<int_or_text, int_string_parts>);
static_assert(not pnp::support::variant_parts_c<int_or_text, std::tuple<pnp::default_printer<int>>>,
    "one part per alternative");
static_assert(not pnp::support::variant_parts_print_c<std::variant<std::string, int>, int_string_parts>,
    "parts are matched to alternatives in order");

static_assert(pnp::default_printable_c<int_or_text>);
static_assert(pnp::default_parseable_c<int_or_text>);
static_assert(pnp::default_parseable_c<std::variant<std::monostate, int>>);
static_assert(pnp::default_parseable_c<std::monostate>);
static_assert(not pnp::printer_c<int_or_text_printer, std::variant<int, long>>);
static_assert(not pnp::printer_c<int_or_text_printer, int>);