#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <string>
#include <string_view>
#include <version>

#if defined(__cpp_lib_flat_map)
#include <flat_map>

namespace pnp_tests
{
    template <typename T>
    void
        check_flat_map_round_trip(const T& value_arg, std::u8string_view expected_arg)
    {
        CHECK(pnp::to_u8string(value_arg) == expected_arg);
        const auto parsed = pnp::try_parse<T>(expected_arg);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value_arg);
    }
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("flat_map: as std::map prints, in key order", "[flat_map]")
{
    check_flat_map_round_trip(std::flat_map<int, std::string>{{2, "b"}, { 1, "a" }}, u8"{1:\"a\",2:\"b\"}");
    check_flat_map_round_trip(std::flat_map<int, std::string>{}, u8"{}");

    // A repeated key is an error, as for std::map.
    CHECK(not succeeded(pnp::try_parse<std::flat_map<int, int>>(u8"{1:1,1:2}")));
}

TEST_CASE("flat_multimap: repeated keys are kept", "[flat_map]")
{
    const auto parsed = pnp::try_parse<std::flat_multimap<int, int>>(u8"{1:10,1:11,2:20}");
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value.count(1) == 2);
    CHECK(pnp::to_u8string(parsed.value) == u8"{1:10,1:11,2:20}");
}

static_assert(pnp::default_printable_c<std::flat_map<int, std::string>>);
static_assert(pnp::default_parseable_c<std::flat_map<int, std::string>>);
static_assert(pnp::default_parseable_c<std::flat_multimap<int, int>>);

#endif