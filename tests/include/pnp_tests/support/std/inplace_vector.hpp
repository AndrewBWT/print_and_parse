#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <version>

#if defined(__cpp_lib_inplace_vector)
#include <inplace_vector>

TEST_CASE("inplace_vector: a sequence of at most N elements", "[inplace_vector]")
{
    const std::inplace_vector<int, 4> values{ 1, 2, 3 };
    CHECK(pnp::to_u8string(values) == u8"[1,2,3]");

    const auto parsed = pnp::try_parse<std::inplace_vector<int, 4>>(u8"[1,2,3]");
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == values);

    // Up to the capacity, but not past it.
    CHECK(succeeded(pnp::try_parse<std::inplace_vector<int, 4>>(u8"[1,2,3,4]")));
    const auto full = pnp::try_parse<std::inplace_vector<int, 4>>(u8"[1,2,3,4,5]");
    CHECK(first_message(full)
        == u8"Could not add element 4 to the sequence: Too many elements; an inplace_vector of capacity 4 is full.");
}

static_assert(pnp::default_parseable_c<std::inplace_vector<int, 4>>);
static_assert(not pnp::support::sequence_fixed_count_v<std::inplace_vector<int, 4>>.has_value(), "its size varies");

#endif