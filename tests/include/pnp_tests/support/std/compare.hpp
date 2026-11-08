#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <array>
#include <compare>
#include <stdexcept>
#include <string_view>

namespace pnp_tests
{
using strong_pnp  = pnp::support::ordering_pnp<std::strong_ordering>;
using partial_pnp = pnp::support::ordering_pnp<std::partial_ordering>;

template <typename O>
void
    check_ordering_round_trip(
        const O            value_arg,
        std::u8string_view expected_arg
    )
{
    CHECK(pnp::to_u8string(value_arg) == expected_arg);
    const auto parsed = pnp::try_parse<O>(expected_arg);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value_arg);
}
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "orderings: the standard's own names",
    "[compare]"
)
{
    check_ordering_round_trip(std::strong_ordering::less, u8"less");
    check_ordering_round_trip(std::strong_ordering::equal, u8"equal");
    check_ordering_round_trip(std::strong_ordering::greater, u8"greater");

    check_ordering_round_trip(std::weak_ordering::less, u8"less");
    check_ordering_round_trip(std::weak_ordering::equivalent, u8"equivalent");
    check_ordering_round_trip(std::weak_ordering::greater, u8"greater");

    check_ordering_round_trip(std::partial_ordering::less, u8"less");
    check_ordering_round_trip(
        std::partial_ordering::equivalent, u8"equivalent"
    );
    check_ordering_round_trip(std::partial_ordering::greater, u8"greater");
    check_ordering_round_trip(std::partial_ordering::unordered, u8"unordered");

    // From real comparisons.
    CHECK(pnp::to_u8string(1 <=> 2) == u8"less");
    CHECK(pnp::to_u8string(1.0 <=> 0.0 / 0.0) == u8"unordered"); // NaN
}

TEST_CASE(
    "orderings: symbols, and their fixed width",
    "[compare][fixed_width]"
)
{
    using symbols_pnp = pnp::support::
        ordering_pnp<std::partial_ordering, pnp::support::ordering_pnp_symbols>;
    CHECK(
        pnp::to_u8string(symbols_pnp{}, std::partial_ordering::unordered)
        == u8"?"
    );
    CHECK(symbols_pnp{}.fixed_output_width() == 1);

    // Written back to back in a flat array: <=>.
    using strong_symbols = pnp::support::
        ordering_pnp<std::strong_ordering, pnp::support::ordering_pnp_symbols>;
    using flat_pnp = pnp::support::generic_sequence_pnp<
        std::array<std::strong_ordering, 3>,
        strong_symbols>;
    const flat_pnp flat(
        {.open_delimiter = u8"", .separator = u8"", .close_delimiter = u8""}
    );
    const std::array<std::strong_ordering, 3> values{
        std::strong_ordering::less,
        std::strong_ordering::equal,
        std::strong_ordering::greater
    };
    CHECK(pnp::to_u8string(flat, values) == u8"<=>");
    const auto parsed
        = pnp::try_parse<std::array<std::strong_ordering, 3>>(u8"<=>", flat);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == values);

    // The default words differ in length, so they aren't fixed width.
    CHECK(not strong_pnp{}.fixed_output_width().has_value());
}

TEST_CASE(
    "orderings: configuring the words",
    "[compare]"
)
{
    const strong_pnp lt(
        {.less_word = u8"lt", .equal_word = u8"eq", .greater_word = u8"gt"}
    );
    CHECK(pnp::to_u8string(lt, std::strong_ordering::greater) == u8"gt");
    CHECK(lt.fixed_output_width() == 2);

    const partial_pnp nan({.unordered_word = u8"nan"});
    CHECK(pnp::to_u8string(nan, std::partial_ordering::unordered) == u8"nan");
    const auto parsed = pnp::try_parse<std::partial_ordering>(u8"nan", nan);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == std::partial_ordering::unordered);
}

TEST_CASE(
    "orderings: invalid words",
    "[compare]"
)
{
    CHECK(
        invalid_argument_message(
            []
            {
                strong_pnp p({.less_word = u8""});
            }
        )
        == "ordering_pnp_options words must be non-empty"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                strong_pnp p({.less_word = u8"great"});
            }
        )
        == "ordering_pnp_options words must not be prefixes of each other"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                strong_pnp p({.equal_word = u8"less"});
            }
        )
        == "ordering_pnp_options words must not be prefixes of each other"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                partial_pnp p({.unordered_word = u8" unordered"});
            }
        )
        == "ordering_pnp_options words must not start or end with whitespace"
    );
    CHECK_THROWS_AS(
        strong_pnp({.greater_word = half_a_character}), std::invalid_argument
    );
}

TEST_CASE(
    "orderings: parse errors",
    "[compare]"
)
{
    CHECK(
        first_message(pnp::try_parse<std::strong_ordering>(u8"bigger"))
        == u8"Expected the word for less \"less\", the word for equal "
           u8"\"equal\" or the word for greater \"greater\"."
    );
    CHECK(not succeeded(pnp::try_parse<std::strong_ordering>(u8"equivalent"))
    ); // not a strong ordering word
    CHECK(not succeeded(pnp::try_parse<std::weak_ordering>(u8"unordered"))
    ); // only partial has it
    CHECK(not succeeded(pnp::try_parse<std::strong_ordering>(u8"lessx")));
    CHECK(not succeeded(pnp::try_parse<std::strong_ordering>(u8"")));
}

// --- Defaults and the exact-type guard
// -------------------------------------------------
static_assert(pnp::default_printable_c<std::strong_ordering>);
static_assert(pnp::default_parseable_c<std::weak_ordering>);
static_assert(pnp::default_parseable_c<std::partial_ordering>);
static_assert(not pnp::printer_c<strong_pnp, std::weak_ordering>);
static_assert(not pnp::printer_c<strong_pnp, int>);
static_assert(pnp::support::valid_ordering_pnp_c<
              std::partial_ordering,
              pnp::support::ordering_pnp_symbols>);