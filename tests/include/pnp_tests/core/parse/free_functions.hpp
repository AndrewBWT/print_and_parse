#pragma once
#include "pnp/sources/istream_text_source.hpp"
#include "pnp/sources/string_source.hpp"
#include "pnp/sources/testing/piece_source.hpp"
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace pnp_tests;

// ============================================================ the parse
// functions

namespace pnp_tests
{
// Reads on, then reports a failure at an earlier position.
struct fails_earlier
{
    pnp::basic_parser_return_value
        run_parser(
            pnp::parser_input& input_arg,
            int&,
            pnp::parser_run_key
        ) const
    {
        input_arg.advance(4);
        return input_arg.fail(u8"failed back at 1", 1);
    }
};

struct not_parseable
{};
} // namespace pnp_tests

TEST_CASE(
    "parse functions: every kind of input",
    "[parse]"
)
{
    // A literal, a view, a string, a narrow string: all viewed.
    CHECK(succeeded(pnp::try_parse<bool>(u8"true")));
    CHECK(succeeded(pnp::try_parse<bool>(std::u8string_view(u8"true"))));
    CHECK(succeeded(pnp::try_parse<bool>(std::u8string(u8"true"))));
    CHECK(succeeded(pnp::try_parse<bool>(std::string("true"))));

    // A source, split everywhere.
    pnp::testing::piece_source source(std::u8string(u8"false"), 1);
    const auto                 from_source = pnp::try_parse<bool>(source);
    REQUIRE(succeeded(from_source));
    CHECK(not from_source.value);

    // Into an existing object, and with a default-constructed parser.
    bool value = false;
    CHECK(
        pnp::parse(u8"true", value).parse_outcome_state
        == pnp::parse_outcome::parse_outcome_state_enum::
            success_and_all_input_consumed
    );
    CHECK(value);
    CHECK(succeeded(pnp::try_parse<bool, pnp::default_parser<bool>>(u8"true")));
}

TEST_CASE(
    "parse functions: each way of naming the parser",
    "[parse]"
)
{
    // digits_parser's value is how many digits it read.
    int value = 0;
    CHECK(succeeded(pnp::parse(u8"12", value, digits_parser{})));
    CHECK(value == 2);
    CHECK(succeeded(pnp::parse<digits_parser>(u8"123", value)));
    CHECK(value == 3);

    const auto tried = pnp::try_parse<int, digits_parser>(u8"1234");
    REQUIRE(succeeded(tried));
    CHECK(tried.value == 4);
    const auto tried_with = pnp::try_parse<int, digits_parser>(u8"12345");
    REQUIRE(succeeded(tried_with));
    CHECK(tried_with.value == 5);
}

TEST_CASE(
    "parse functions: a failure leaves the value alone",
    "[parse][errors]"
)
{
    // Into an existing object: untouched.
    int value = 42;
    CHECK(not succeeded(pnp::parse<digits_parser>(u8"abc", value)));
    CHECK(value == 42);

    // Into a new one: default constructed.
    const auto tried = pnp::try_parse<int, digits_parser>(u8"abc");
    CHECK(not succeeded(tried));
    CHECK(tried.value == 0);
}

TEST_CASE(
    "parse functions: nothing to parse",
    "[parse][errors]"
)
{
    CHECK(not succeeded(pnp::try_parse<bool>(u8"")));
    CHECK(not succeeded(pnp::try_parse<bool>(u8"   ")));
}

TEST_CASE(
    "parse functions: whitespace either side is skipped",
    "[parse]"
)
{
    CHECK(succeeded(pnp::try_parse<int>(u8"5\n")));
    CHECK(succeeded(pnp::try_parse<int>(u8"5 \t\r\n")));
    CHECK(succeeded(pnp::try_parse<int>(u8" 5")));
    CHECK(succeeded(pnp::try_parse<int>(u8"\n 5 \n")));
    // Anything else still remains, reported where it starts.
    int        value     = 0;
    const auto remaining = pnp::parse(u8" 5 x", value);
    CHECK(
        remaining.parse_outcome_state
        == pnp::parse_outcome::parse_outcome_state_enum::
            success_but_input_remains
    );
    CHECK(remaining.position == 3);
}

TEST_CASE(
    "parse functions: thrown input errors become failures",
    "[parse][errors]"
)
{
    using code_t      = pnp::parse_outcome::parse_outcome_state_enum;

    // A parse_exception is caught, and reported with its code and position.
    int        value  = 0;
    const auto result = pnp::parse<overreaching_parser>(u8"12", value);
    CHECK(result.parse_outcome_state == code_t::failure_parser_exception);
    CHECK(
        result.error.error_code == error_code::invalid_parser_advanced_past_end
    );
    CHECK(result.position == 0);

    // Invalid UTF-8 inside the value: reported where it is.
    const auto invalid = pnp::try_parse<bool>(u8"tr" + code_unit(0x80));
    CHECK(
        invalid.outcome.parse_outcome_state == code_t::failure_parser_exception
    );
    CHECK(invalid.outcome.error.error_code == error_code::invalid_utf8);
    CHECK(invalid.outcome.position == 2);
    CHECK(invalid.outcome.parsers_error_context.location.line == 1);
    CHECK(invalid.outcome.parsers_error_context.location.column == 3);

    // An ordinary failure is still reported as one.
    const auto failed = pnp::try_parse<bool>(u8"maybe");
    CHECK(failed.outcome.parse_outcome_state == code_t::failure_parser_error);
    CHECK(not failed.outcome.parser_error_messages.empty());

    // Input left over is reported, with where it starts.
    bool       flag      = false;
    const auto remaining = pnp::parse(u8"truex", flag);
    CHECK(remaining.parse_outcome_state == code_t::success_but_input_remains);
    CHECK(remaining.position == 4);
}

TEST_CASE(
    "parse functions: a failure's context is where it failed",
    "[parse][errors]"
)
{
    int        value = 0;
    const auto result
        = pnp::parse<fails_earlier>(std::u8string_view(u8"abcdef"), value);
    CHECK(not succeeded(result));
    CHECK(result.position == 1);
    // At the failure, not where the parser stopped (position 4).
    CHECK(result.parsers_error_context.current_pos_text == u8"b");
    CHECK(result.parsers_error_context.location.line == 1);
    CHECK(result.parsers_error_context.location.column == 2);
}

TEST_CASE(
    "parse functions: other exceptions pass through",
    "[parse][errors]"
)
{
    int value = 0;
    CHECK_THROWS_AS(
        pnp::parse<broken_parser>(u8"1", value), std::runtime_error
    );

    // Misuse isn't an input error: a source already being read.
    pnp::string_source source(u8"true");
    pnp::parser_input  reader(source);
    bool               flag = false;
    CHECK_THROWS_AS(pnp::parse(source, flag), std::logic_error);
}

TEST_CASE(
    "parse functions: a source moves on past what was parsed",
    "[parse][io]"
)
{
    using code_t = pnp::parse_outcome::parse_outcome_state_enum;

    // Everything parsed: the source is left empty, and free.
    {
        pnp::string_source source(u8"true ");
        bool               flag = false;
        CHECK(
            pnp::parse(source, flag).parse_outcome_state
            == code_t::success_and_all_input_consumed
        );
        CHECK(source.offset() == 5);
        CHECK(source.buffered().empty());
        CHECK_FALSE(source.in_use());
    }
    // Two values, one after the other.
    {
        pnp::string_source source(u8"true false");
        bool               flag  = false;
        const auto         first = pnp::parse(source, flag);
        CHECK(first.parse_outcome_state == code_t::success_but_input_remains);
        CHECK(flag);
        CHECK(source.offset() == 5); // the value and the space after it
        const auto second = pnp::parse(source, flag);
        CHECK(
            second.parse_outcome_state == code_t::success_and_all_input_consumed
        );
        CHECK(not flag);
        CHECK(source.offset() == 10);
    }
    // The same, arriving a byte at a time.
    {
        pnp::testing::piece_source source(std::u8string(u8"true false"), 1);
        bool                       flag = false;
        CHECK(succeeded_or_remains(pnp::parse(source, flag)));
        CHECK(flag);
        CHECK(succeeded(pnp::parse(source, flag)));
        CHECK(not flag);
    }
    // A failure discards nothing more, and leaves the source free.
    {
        pnp::string_source source(u8"maybe");
        bool               flag = false;
        CHECK(not succeeded(pnp::parse(source, flag)));
        CHECK(source.offset() == 0);
        CHECK_FALSE(source.in_use());
    }
}

// --- Compile-time checks
// ------------------------------------------------------------------
static_assert(pnp::parse_input_c<std::u8string_view>);
static_assert(pnp::parse_input_c<std::string_view>);
static_assert(pnp::parse_input_c<const char8_t (&)[5]>);
static_assert(pnp::parse_input_c<std::u8string>);
static_assert(pnp::parse_input_c<pnp::testing::piece_source>);
static_assert(pnp::parse_input_c<pnp::text_source>);
static_assert(not pnp::parse_input_c<int>);
// default_parser's purpose: a type with no specialisation isn't parseable.
static_assert(not pnp::default_parseable_c<pnp_tests::not_parseable>);