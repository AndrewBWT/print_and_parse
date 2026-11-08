#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <cstddef>
#include <valarray>
#include <vector>

namespace pnp_tests
{
// valarray's == compares element by element, giving a valarray of bools.
template <typename T>
bool
    valarrays_equal(
        const std::valarray<T>& left_arg,
        const std::valarray<T>& right_arg
    )
{
    if (left_arg.size() != right_arg.size())
    {
        return false;
    }
    for (std::size_t idx = 0; idx < left_arg.size(); ++idx)
    {
        if (not (left_arg[idx] == right_arg[idx]))
        {
            return false;
        }
    }
    return true;
}
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "valarray: a sequence",
    "[valarray]"
)
{
    const std::valarray<int> values{1, 2, 3};
    CHECK(pnp::to_u8string(values) == u8"[1,2,3]");
    CHECK(pnp::to_u8string(std::valarray<int>{}) == u8"[]");

    const auto parsed = pnp::try_parse<std::valarray<int>>(u8"[1,2,3]");
    REQUIRE(succeeded(parsed));
    CHECK(valarrays_equal(parsed.value, values));

    const auto empty = pnp::try_parse<std::valarray<int>>(u8"[]");
    REQUIRE(succeeded(empty));
    CHECK(empty.value.size() == 0);

    // Arithmetic results print too.
    CHECK(
        pnp::to_u8string(std::valarray<double>(
            std::valarray<double>(0.5, values.size()) * 2.0
        ))
        == u8"[1,1,1]"
    );
}

TEST_CASE(
    "valarray: configured like any sequence, and its errors",
    "[valarray]"
)
{
    using valarray_pnp = pnp::default_printer<std::valarray<int>>;
    CHECK(
        pnp::to_u8string(
            valarray_pnp({.separator_spacing = u8" "}), std::valarray<int>{1, 2}
        )
        == u8"[1, 2]"
    );

    CHECK(not succeeded(pnp::try_parse<std::valarray<int>>(u8"[1,x]")));
    CHECK(not succeeded(pnp::try_parse<std::valarray<int>>(u8"[1,2")));
}

static_assert(pnp::default_printable_c<std::valarray<int>>);
static_assert(pnp::default_parseable_c<std::valarray<int>>);
static_assert(not pnp::printer_c<
              pnp::default_printer<std::valarray<int>>,
              std::vector<int>>);