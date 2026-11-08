#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <functional>
#include <string>
#include <vector>

TEST_CASE("reference_wrapper: exactly as what it refers to", "[reference_wrapper]")
{
    int                         value = 5;
    std::reference_wrapper<int> reference = std::ref(value);
    CHECK(pnp::to_u8string(reference) == u8"5");

    // It follows the object it refers to.
    value = 7;
    CHECK(pnp::to_u8string(reference) == u8"7");

    // A reference to const, and references inside containers.
    const std::string text = "a,b";
    CHECK(pnp::to_u8string(std::cref(text)) == u8"\"a,b\"");
    int                                            a = 1;
    int                                            b = 2;
    const std::vector<std::reference_wrapper<int>> references{ a, b };
    CHECK(pnp::to_u8string(references) == u8"[1,2]");

    // A configured referent part.
    using reference_printer = pnp::default_printer<std::reference_wrapper<int>>;
    const reference_printer plus({ .referent = reference_printer::referent_part_type({.use_plus_for_positive_numbers = true}) });
    CHECK(pnp::to_u8string(plus, reference) == u8"+7");
}

static_assert(pnp::default_printable_c<std::reference_wrapper<int>>);
static_assert(pnp::default_printable_c<std::reference_wrapper<const std::string>>);
static_assert(not pnp::default_parseable_c<std::reference_wrapper<int>>, "print-only");

// A reference prints as its referent, but isn't one.
static_assert(not pnp::printer_c<pnp::default_printer<std::reference_wrapper<int>>, int>);