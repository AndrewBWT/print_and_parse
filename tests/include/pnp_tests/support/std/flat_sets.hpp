#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <string_view>
#include <version>

#if defined(__cpp_lib_flat_set)
#include <flat_set>

namespace pnp_tests
{
    template <typename T>
    void
        check_flat_set_round_trip(const T& value_arg, std::u8string_view expected_arg)
    {
        CHECK(pnp::to_u8string(value_arg) == expected_arg);
        const auto parsed = pnp::try_parse<T>(expected_arg);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value_arg);
    }
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("flat_set: as std::set prints, in order", "[flat_set]")
{
    check_flat_set_round_trip(std::flat_set<int>{3, 1, 2}, u8"{1,2,3}");
    check_flat_set_round_trip(std::flat_set<int>{}, u8"{}");
    CHECK(not succeeded(pnp::try_parse<std::flat_set<int>>(u8"{1,1}")));   // repeated element
}

TEST_CASE("flat_multiset: repeated elements are kept", "[flat_set]")
{
    check_flat_set_round_trip(std::flat_multiset<int>{2, 1, 2}, u8"{1,2,2}");
}

static_assert(pnp::default_parseable_c<std::flat_set<int>>);
static_assert(pnp::default_parseable_c<std::flat_multiset<int>>);

#endif