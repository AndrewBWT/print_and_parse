#pragma once
#include "pnp/core/parse/parse_outcome.hpp"
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace pnp_tests::parse_outcome_tests
{
using state_t = pnp::parse_outcome::parse_outcome_state_enum;

// No default constructor; a placeholder of 7.
struct starts_at_seven
{
    constexpr explicit starts_at_seven(
        const int n_arg
    )
        : n(n_arg)
    {}

    int n;
};

// A default constructor (1) and a placeholder (2): the placeholder must win.
struct default_one
{
    int n = 1;
};

// Neither a default constructor nor a placeholder.
struct unmakeable
{
    explicit unmakeable(
        const int n_arg
    )
        : n(n_arg)
    {}

    int n;
};

// Whether parse_outcome_with_value<T> can be named at all: false when its
// constraint rejects T.
template <typename T>
concept can_hold_c = requires { typename pnp::parse_outcome_with_value<T>; };

// A parse_outcome in the given state.
inline pnp::parse_outcome
    outcome_in(
        const state_t state_arg
    )
{
    pnp::parse_outcome outcome;
    outcome.parse_outcome_state = state_arg;
    return outcome;
}

// The digit at index_arg of text_arg, or -1 if it isn't one.
inline int
    digit_at(
        const std::u8string_view text_arg,
        const std::size_t        index_arg
    )
{
    const char8_t c = text_arg[index_arg];
    return c >= u8'0' && c <= u8'9' ? static_cast<int>(c - u8'0') : -1;
}

// Our own pnps for C arrays, so these tests don't depend on pnp's C array
// support. Each parses into a local array, and only copies it into value_arg
// once everything has succeeded: a failure writes nothing.

// Exactly three digits into an int[3]: an array whose elements have no
// placeholder.
struct three_digits_pnp
{
    pnp::basic_parser_return_value
        run_parser(
            pnp::parser_input& input_arg,
            int (&value_arg)[3],
            pnp::parser_run_key
        ) const
    {
        const auto text = input_arg.peek_chars(3);
        if (not text.has_value() || text->get().size() != 3)
        {
            return input_arg.fail(u8"expected three digits");
        }
        int parsed[3]{};
        for (std::size_t index = 0; index < 3; ++index)
        {
            parsed[index] = digit_at(text->get(), index);
            if (parsed[index] < 0)
            {
                return input_arg.fail(
                    u8"expected a digit", input_arg.position() + index
                );
            }
        }
        input_arg.advance(3);
        for (std::size_t index = 0; index < 3; ++index)
        {
            value_arg[index] = parsed[index];
        }
        return {};
    }
};

// Exactly two digits into a starts_at_seven[2]: an array of a type with only a
// placeholder.
struct two_digits_pnp
{
    pnp::basic_parser_return_value
        run_parser(
            pnp::parser_input& input_arg,
            starts_at_seven (&value_arg)[2],
            pnp::parser_run_key
        ) const
    {
        const auto text = input_arg.peek_chars(2);
        if (not text.has_value() || text->get().size() != 2)
        {
            return input_arg.fail(u8"expected two digits");
        }
        const int first  = digit_at(text->get(), 0);
        const int second = digit_at(text->get(), 1);
        if (first < 0 || second < 0)
        {
            return input_arg.fail(u8"expected a digit");
        }
        input_arg.advance(2);
        value_arg[0] = starts_at_seven(first);
        value_arg[1] = starts_at_seven(second);
        return {};
    }
};
} // namespace pnp_tests::parse_outcome_tests

PNP_NS_BEGIN

template <>
struct placeholder_value<pnp_tests::parse_outcome_tests::starts_at_seven>
{
    static constexpr pnp_tests::parse_outcome_tests::starts_at_seven
        value()
    {
        return pnp_tests::parse_outcome_tests::starts_at_seven(7);
    }
};

template <>
struct placeholder_value<pnp_tests::parse_outcome_tests::default_one>
{
    static constexpr pnp_tests::parse_outcome_tests::default_one
        value()
    {
        return pnp_tests::parse_outcome_tests::default_one{2};
    }
};

PNP_NS_END

using namespace pnp_tests::parse_outcome_tests;

TEST_CASE(
    "parse_outcome: a default outcome is a failure",
    "[parse_outcome]"
)
{
    const pnp::parse_outcome outcome;
    CHECK(outcome.parse_outcome_state == state_t::failure_parser_error);
    CHECK(outcome.failed());
    CHECK(not outcome.parsed_value());
    CHECK(outcome.position == 0);
    CHECK(outcome.parser_error_messages.empty());
    CHECK(
        outcome.error.error_code
        == pnp::parse_result_error::error_enum::invalid_utf8
    );
}

TEST_CASE(
    "parse_outcome: each state answers each question",
    "[parse_outcome]"
)
{
    struct expected_answers
    {
        state_t state;
        bool    succeeded;
        bool    succeeded_with_input_remaining;
        bool    parsed_value;
        bool    failed;
    };

    const std::array<expected_answers, 4> table{
        {
         {state_t::success_and_all_input_consumed, true, false, true, false},
         {state_t::success_but_input_remains, false, true, true, false},
         {state_t::failure_parser_error, false, false, false, true},
         {state_t::failure_parser_exception, false, false, false, true},
         }
    };

    for (const expected_answers& row : table)
    {
        CAPTURE(static_cast<int>(row.state));
        const pnp::parse_outcome outcome = outcome_in(row.state);
        CHECK(outcome.succeeded() == row.succeeded);
        CHECK(
            outcome.succeeded_with_input_remaining()
            == row.succeeded_with_input_remaining
        );
        CHECK(outcome.parsed_value() == row.parsed_value);
        CHECK(outcome.failed() == row.failed);
        // Every state is exactly one of: all consumed, input remaining, failed.
        CHECK(
            int(outcome.succeeded())
                + int(outcome.succeeded_with_input_remaining())
                + int(outcome.failed())
            == 1
        );
        // A value was parsed exactly when the parse didn't fail.
        CHECK(outcome.parsed_value() == not outcome.failed());
    }
}

TEST_CASE(
    "parse_outcome_with_value: starts from make_placeholder",
    "[parse_outcome]"
)
{
    // A value-initialised T: zero, not whatever was in memory.
    const pnp::parse_outcome_with_value<int> with_int;
    CHECK(with_int.value == 0);
    CHECK(with_int.outcome.failed());

    // A placeholder, for a type with no default constructor.
    const pnp::parse_outcome_with_value<starts_at_seven> with_placeholder;
    CHECK(with_placeholder.value.n == 7);

    // The placeholder wins over a default constructor.
    const pnp::parse_outcome_with_value<default_one> with_both;
    CHECK(with_both.value.n == 2);
}

TEST_CASE(
    "parse_outcome_with_value: C arrays",
    "[parse_outcome]"
)
{
    // No placeholder: value-initialised, any number of dimensions.
    const pnp::parse_outcome_with_value<int[2][3]> ints;
    for (const auto& row : ints.value)
    {
        for (const int n : row)
        {
            CHECK(n == 0);
        }
    }

    // A placeholder-only element: each element its placeholder.
    const pnp::parse_outcome_with_value<starts_at_seven[3]> sevens;
    for (const starts_at_seven& element : sevens.value)
    {
        CHECK(element.n == 7);
    }

    // The same, in two dimensions: one flat list fills both.
    const pnp::parse_outcome_with_value<starts_at_seven[2][2]> grid;
    for (const auto& row : grid.value)
    {
        for (const starts_at_seven& element : row)
        {
            CHECK(element.n == 7);
        }
    }

    // The placeholder wins inside an array too, in any number of dimensions.
    const pnp::parse_outcome_with_value<default_one[3]> both;
    for (const default_one& element : both.value)
    {
        CHECK(element.n == 2);
    }
    const pnp::parse_outcome_with_value<default_one[2][2]> both_grid;
    for (const auto& row : both_grid.value)
    {
        for (const default_one& element : row)
        {
            CHECK(element.n == 2);
        }
    }
}

TEST_CASE(
    "parse_outcome_with_value: only for types pnp can make",
    "[parse_outcome]"
)
{
    STATIC_CHECK(can_hold_c<int>);
    STATIC_CHECK(can_hold_c<starts_at_seven>);
    STATIC_CHECK(can_hold_c<default_one>);
    STATIC_CHECK(not can_hold_c<unmakeable>);

    // C arrays: if their elements can be made.
    STATIC_CHECK(can_hold_c<int[3]>);
    STATIC_CHECK(can_hold_c<int[2][3]>);
    STATIC_CHECK(can_hold_c<starts_at_seven[2][2]>);
    STATIC_CHECK(not can_hold_c<unmakeable[3]>);
    // An array of unknown bound has no size to make.
    STATIC_CHECK(not can_hold_c<int[]>);
}

TEST_CASE(
    "parse_outcome_with_value: C arrays through try_parse and parse",
    "[parse_outcome][parse]"
)
{
    // Elements with no placeholder.
    {
        const auto parsed = pnp::try_parse<int[3], three_digits_pnp>(u8"123");
        REQUIRE(parsed.outcome.succeeded());
        CHECK(parsed.value[0] == 1);
        CHECK(parsed.value[1] == 2);
        CHECK(parsed.value[2] == 3);

        // A failure leaves the value as it started: value-initialised.
        const auto failed = pnp::try_parse<int[3], three_digits_pnp>(u8"1x3");
        CHECK(failed.outcome.failed());
        CHECK(failed.outcome.position == 1);
        for (const int n : failed.value)
        {
            CHECK(n == 0);
        }
    }
    // Elements with only a placeholder.
    {
        const auto parsed
            = pnp::try_parse<starts_at_seven[2], two_digits_pnp>(u8"45");
        REQUIRE(parsed.outcome.succeeded());
        CHECK(parsed.value[0].n == 4);
        CHECK(parsed.value[1].n == 5);

        // A failure leaves each element its placeholder.
        const auto failed
            = pnp::try_parse<starts_at_seven[2], two_digits_pnp>(u8"4");
        CHECK(failed.outcome.failed());
        CHECK(failed.value[0].n == 7);
        CHECK(failed.value[1].n == 7);
    }
    // Into an existing array: written on success, untouched on failure.
    {
        int values[3]{9, 9, 9};
        CHECK(pnp::parse<three_digits_pnp>(u8"x23", values).failed());
        CHECK(values[0] == 9);
        CHECK(values[1] == 9);
        CHECK(values[2] == 9);
        CHECK(pnp::parse<three_digits_pnp>(u8"456", values).succeeded());
        CHECK(values[0] == 4);
        CHECK(values[1] == 5);
        CHECK(values[2] == 6);
    }
}

TEST_CASE(
    "parse_outcome: real parses reach each state",
    "[parse_outcome][parse]"
)
{
    CHECK(
        pnp::try_parse<int>(u8"5").outcome.parse_outcome_state
        == state_t::success_and_all_input_consumed
    );
    CHECK(
        pnp::try_parse<int>(u8"5 x").outcome.parse_outcome_state
        == state_t::success_but_input_remains
    );

    // A failure leaves the value as its placeholder.
    const auto failed = pnp::try_parse<int>(u8"x");
    CHECK(failed.outcome.parse_outcome_state == state_t::failure_parser_error);
    CHECK(failed.value == 0);

    const auto invalid = pnp::try_parse<int>(u8"5" + code_unit(0x80));
    CHECK(
        invalid.outcome.parse_outcome_state == state_t::failure_parser_exception
    );
}

static_assert(not std::is_constructible_v<
              pnp::parse_outcome_with_value<starts_at_seven[3]>,
              std::index_sequence<0, 1, 2>>);