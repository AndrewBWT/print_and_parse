#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace pnp_tests
{
    // Prints, checks the text if an expectation is given, parses back, and checks the value.
    template <typename Set>
    void
        check_set_round_trip(const Set& value, std::u8string_view expected = {})
    {
        const auto text = pnp::to_u8string(value);
        if (not expected.empty())
        {
            CHECK(text == expected);
        }
        const auto parsed = pnp::try_parse<Set>(text);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value);
    }

    struct set_test_no_defaults
    {
    };
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("sets: std::set round-trips in key order", "[sets]")
{
    check_set_round_trip(std::set<int>{1, 2, 3}, u8"{1,2,3}");
    check_set_round_trip(std::set<int>{3, 1, 2}, u8"{1,2,3}");   // sorted
    check_set_round_trip(std::set<int>{42}, u8"{42}");
    check_set_round_trip(std::set<int>{}, u8"{}");
    check_set_round_trip(std::set<std::string>{"b,c", "a", "{d}"});
    check_set_round_trip(std::set<std::vector<int>>{{1, 2}, {}}, u8"{[],[1,2]}");

    // A custom comparator decides the order.
    check_set_round_trip(std::set<int, std::greater<int>>{1, 2, 3}, u8"{3,2,1}");
}

TEST_CASE("sets: std::multiset keeps repeated keys", "[sets]")
{
    check_set_round_trip(std::multiset<int>{2, 1, 2, 1}, u8"{1,1,2,2}");
    check_set_round_trip(std::multiset<int>{}, u8"{}");

    const auto parsed = pnp::try_parse<std::multiset<int>>(u8"{5,5,5}");
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value.count(5) == 3);
}

TEST_CASE("sets: the unordered sets round-trip", "[sets]")
{
    // With one element or none, the text is predictable; otherwise only the round trip.
    check_set_round_trip(std::unordered_set<int>{42}, u8"{42}");
    check_set_round_trip(std::unordered_set<int>{}, u8"{}");
    check_set_round_trip(std::unordered_set<int>{1, 2, 3, 4, 5});
    check_set_round_trip(std::unordered_set<std::string>{"x", "y,z"});

    check_set_round_trip(std::unordered_multiset<int>{7}, u8"{7}");
    check_set_round_trip(std::unordered_multiset<int>{1, 1, 2});

    const auto parsed = pnp::try_parse<std::unordered_multiset<int>>(u8"{5,5}");
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value.count(5) == 2);
}

TEST_CASE("sets: repeated keys are an error for unique sets", "[sets]")
{
    const auto ordered = pnp::try_parse<std::set<int>>(u8"{1,2,1}");
    CHECK(not succeeded(ordered));
    CHECK(first_message(ordered)
        == u8"Could not add element 2 to the sequence: An element with the same key is already in the container.");

    const auto unordered = pnp::try_parse<std::unordered_set<int>>(u8"{1,1}");
    CHECK(not succeeded(unordered));
    CHECK(first_message(unordered)
        == u8"Could not add element 1 to the sequence: An element with the same key is already in the container.");
}

TEST_CASE("sets: parse errors and configuration", "[sets]")
{
    const auto square = pnp::try_parse<std::set<int>>(u8"[1,2]");
    CHECK(not succeeded(square));
    CHECK(first_message(square) == u8"Expected opening delimiter \"{\".");

    CHECK(not succeeded(pnp::try_parse<std::set<int>>(u8"{1,x}")));
    CHECK(not succeeded(pnp::try_parse<std::set<int>>(u8"{1,2")));
    CHECK(not succeeded(pnp::try_parse<std::set<int>>(u8"{}x")));

    using set_printer = pnp::default_printer<std::set<int>>;
    using set_parser = pnp::default_parser<std::set<int>>;
    CHECK(pnp::to_u8string(set_printer({ .separator_spacing = u8" " }), std::set<int>{1, 2}) == u8"{1, 2}");

    const set_parser lenient({ .skip_whitespace = true });
    const auto       spaced = pnp::try_parse<std::set<int>>(u8"{ 2 ,1 }", lenient);
    REQUIRE(succeeded(spaced));
    CHECK(spaced.value == std::set<int>{1, 2});
}

// --- Defaults and appenders ------------------------------------------------------
static_assert(pnp::default_printable_c<std::set<int>>);
static_assert(pnp::default_parseable_c<std::set<int>>);
static_assert(pnp::default_printable_c<std::multiset<int>>);
static_assert(pnp::default_parseable_c<std::multiset<int>>);
static_assert(pnp::default_printable_c<std::unordered_set<int>>);
static_assert(pnp::default_parseable_c<std::unordered_set<int>>);
static_assert(pnp::default_printable_c<std::unordered_multiset<int>>);
static_assert(pnp::default_parseable_c<std::unordered_multiset<int>>);
static_assert(not pnp::default_printable_c<std::set<set_test_no_defaults>>);

static_assert(std::is_base_of_v<pnp::support::insert_appender<std::set<int>>, pnp::support::default_appender<std::set<int>>>);
static_assert(std::is_base_of_v<pnp::support::insert_appender<std::unordered_multiset<int>>,
    pnp::support::default_appender<std::unordered_multiset<int>>>);

// --- The exact-type guard ----------------------------------------------------
static_assert(not pnp::printer_c<pnp::default_printer<std::set<int>>, std::multiset<int>>);
static_assert(not pnp::printer_c<pnp::default_printer<std::set<int>>, std::unordered_set<int>>);
static_assert(not pnp::printer_c<pnp::default_printer<std::set<int>>, std::vector<int>>);