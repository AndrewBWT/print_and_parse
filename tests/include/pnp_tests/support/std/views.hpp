#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <map>
#include <ranges>
#include <string>
#include <vector>
#include <version>

using namespace pnp_tests;

TEST_CASE("views: the standard views print as sequences", "[views]")
{
    const std::vector<int> values{ 1, 2, 3, 4, 5, 6 };

    CHECK(pnp::to_u8string(std::views::iota(1, 4)) == u8"[1,2,3]");
    CHECK(pnp::to_u8string(std::views::all(values)) == u8"[1,2,3,4,5,6]");                             // ref_view
    CHECK(pnp::to_u8string(values | std::views::take(2)) == u8"[1,2]");
    CHECK(pnp::to_u8string(values | std::views::drop(4)) == u8"[5,6]");
    CHECK(pnp::to_u8string(values | std::views::reverse) == u8"[6,5,4,3,2,1]");
    CHECK(pnp::to_u8string(std::views::empty<int>) == u8"[]");
    CHECK(pnp::to_u8string(std::views::single(42)) == u8"[42]");

    // A finite part of an infinite view.
    CHECK(pnp::to_u8string(std::views::iota(1) | std::views::take(3)) == u8"[1,2,3]");
}

TEST_CASE("views: views with a non-const begin print through a copy", "[views]")
{
    const std::vector<int> values{ 1, 2, 3, 4, 5, 6 };
    const auto             is_even = [](const int value_arg) { return value_arg % 2 == 0; };

    CHECK(pnp::to_u8string(values | std::views::filter(is_even)) == u8"[2,4,6]");
    CHECK(pnp::to_u8string(values | std::views::drop_while([](const int v) { return v < 3; })) == u8"[3,4,5,6]");
}

TEST_CASE("views: elements are converted to the value type", "[views]")
{
    const std::vector<int> values{ 1, 2, 3 };

    // transform_view yields prvalues.
    CHECK(pnp::to_u8string(values | std::views::transform([](const int v) { return v * 2; })) == u8"[2,4,6]");

    // keys and values of a map, through elements_view.
    const std::map<int, std::string> map{ {1, "a"}, {2, "b"} };
    CHECK(pnp::to_u8string(map | std::views::keys) == u8"[1,2]");
    CHECK(pnp::to_u8string(map | std::views::values) == u8"[\"a\",\"b\"]");

    // join flattens nested containers.
    const std::vector<std::vector<int>> nested{ {1, 2}, {}, {3} };
    CHECK(pnp::to_u8string(nested | std::views::join) == u8"[1,2,3]");

#if defined(__cpp_lib_ranges_zip)
    // zip_view yields tuples of references, converted to tuples of values.
    const std::vector<std::string> names{ "a", "b" };
    CHECK(pnp::to_u8string(std::views::zip(values, names)) == u8"[(1,\"a\"),(2,\"b\")]");
#endif
}

TEST_CASE("views: configured like sequences", "[views]")
{
    using iota_view = decltype(std::views::iota(1, 4));
    using view_printer = pnp::default_printer<iota_view>;
    CHECK(pnp::to_u8string(view_printer({ .separator_spacing = u8" " }), std::views::iota(1, 4)) == u8"[1, 2, 3]");
    CHECK(pnp::to_u8string(view_printer({ .open_delimiter = u8"<", .close_delimiter = u8">" }), std::views::iota(1, 4))
        == u8"<1,2,3>");

    // A view prints exactly as the equivalent vector.
    const std::vector<int> values{ 1, 2, 3 };
    CHECK(pnp::to_u8string(std::views::all(values)) == pnp::to_u8string(values));
}

// --- Defaults ----------------------------------------------------------------------
using iota_view_type = decltype(std::views::iota(1, 4));
static_assert(pnp::default_printable_c<iota_view_type>);
static_assert(not pnp::default_parseable_c<iota_view_type>, "views are print-only");

// An infinite view can't be printed.
static_assert(not pnp::support::printable_range_c<decltype(std::views::iota(1))>);
static_assert(not pnp::default_printable_c<decltype(std::views::iota(1))>);

// Views aren't confused with each other, or with containers.
static_assert(not pnp::printer_c<pnp::default_printer<iota_view_type>, std::vector<int>>);