#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <initializer_list>
#include <string>
#include <vector>

TEST_CASE("initializer_list: printed as a sequence", "[initializer_list]")
{
    CHECK(pnp::to_u8string(std::initializer_list<int>{1, 2, 3}) == u8"[1,2,3]");
    CHECK(pnp::to_u8string(std::initializer_list<std::string>{"a", "b,c"}) == u8"[\"a\",\"b,c\"]");
    CHECK(pnp::to_u8string(std::initializer_list<int>{}) == u8"[]");
}

static_assert(pnp::default_printable_c<std::initializer_list<int>>);
static_assert(not pnp::default_parseable_c<std::initializer_list<int>>, "print-only");
static_assert(not pnp::printer_c<pnp::default_printer<std::initializer_list<int>>, std::vector<int>>);