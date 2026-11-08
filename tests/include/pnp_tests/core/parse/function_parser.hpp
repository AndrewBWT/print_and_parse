#pragma once
#include "pnp/core/parse/function_parser.hpp"
#include "pnp/sources/testing/piece_source.hpp"
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <string>
#include <type_traits>
#include <utility>

namespace pnp_tests::function_parser_tests
{
// Reads a run of digits: the value is how many there were. A plain function,
// so it can be used as a function, a function pointer, or from a lambda.
inline pnp::basic_parser_return_value
    count_digits(
        pnp::parser_input& input_arg,
        int&               value_arg
    )
{
    const auto digits = input_arg.peek_while(
        [](const char8_t c)
        {
            return c >= u8'0' && c <= u8'9';
        }
    );
    if (digits.get().empty())
    {
        return input_arg.fail(u8"expected digits");
    }
    value_arg = static_cast<int>(digits.get().size());
    input_arg.advance(digits.get().size());
    return {};
}

inline constexpr auto count_digits_lambda
    = [](pnp::parser_input& input_arg, int& value_arg)
{
    return count_digits(input_arg, value_arg);
};

// A function object with state: adds offset to the count of digits.
struct adds_offset
{
    int offset = 0;

    pnp::basic_parser_return_value
        operator()(
            pnp::parser_input& input_arg,
            int&               value_arg
        ) const
    {
        int count = 0;
        if (auto result = count_digits(input_arg, count); result.failed())
        {
            return result;
        }
        value_arg = count + offset;
        return {};
    }
};

// Callables that aren't pnp bodies.
inline constexpr auto wrong_return = [](pnp::parser_input&, int&)
{
    return true;
};
inline constexpr auto wrong_arguments = [](pnp::parser_input&)
{
    return pnp::basic_parser_return_value{};
};
inline constexpr auto takes_the_key
    = [](pnp::parser_input&, int&, pnp::parser_run_key)
{
    return pnp::basic_parser_return_value{};
};
inline constexpr auto mutable_counter
    = [count = 0](pnp::parser_input&, int&) mutable
{
    ++count;
    return pnp::basic_parser_return_value{};
};

using lambda_parser = decltype(pnp::function_parser<int>(count_digits_lambda));
using function_pointer_parser
    = decltype(pnp::function_parser<int>(count_digits));
} // namespace pnp_tests::function_parser_tests

using namespace pnp_tests::function_parser_tests;

// --- Compile-time checks
// ------------------------------------------------------------------
// What function_parser makes is a pnp.
static_assert(pnp::parser_c<lambda_parser, int>);
static_assert(pnp::parser_c<function_pointer_parser, int>);
static_assert(pnp::parser_c<
              decltype(pnp::function_parser<int>(adds_offset{})),
              int>);
// A function becomes a function pointer.
static_assert(pnp::function_parser_callable_c<int, decltype(&count_digits)>);
// Callables that aren't accepted.
static_assert(not pnp::function_parser_callable_c<int, decltype(wrong_return)>);
static_assert(not pnp::
                  function_parser_callable_c<int, decltype(wrong_arguments)>);
static_assert(not pnp::
                  function_parser_callable_c<int, decltype(takes_the_key)>);
// run_parser is const, so a callable that changes itself can't be one.
static_assert(not pnp::
                  function_parser_callable_c<int, decltype(mutable_counter)>);
// The wrong value type.
static_assert(not pnp::function_parser_callable_c<
              double,
              decltype(count_digits_lambda)>);
// No accidental conversion from a callable to a parser.
static_assert(not std::is_convertible_v<
              decltype(count_digits_lambda),
              lambda_parser>);
// The constructor template doesn't take over copying.
static_assert(std::is_copy_constructible_v<lambda_parser>);

TEST_CASE(
    "function_parser: from a lambda, a function, or a function pointer",
    "[function_parser]"
)
{
    int value = 0;
    CHECK(succeeded(pnp::parse(
        u8"123", value, pnp::function_parser<int>(count_digits_lambda)
    )));
    CHECK(value == 3);

    CHECK(succeeded(
        pnp::parse(u8"1234", value, pnp::function_parser<int>(count_digits))
    ));
    CHECK(value == 4);

    CHECK(succeeded(
        pnp::parse(u8"12345", value, pnp::function_parser<int>(&count_digits))
    ));
    CHECK(value == 5);
}

TEST_CASE(
    "function_parser: a lambda with captures",
    "[function_parser]"
)
{
    const int  scale  = 10;
    const auto scaled = pnp::function_parser<int>(
        [scale](pnp::parser_input& input_arg, int& value_arg)
            -> pnp::basic_parser_return_value
        {
            int count = 0;
            if (auto result = count_digits(input_arg, count); result.failed())
            {
                return result;
            }
            value_arg = count * scale;
            return {};
        }
    );
    int value = 0;
    CHECK(succeeded(pnp::parse(u8"123", value, scaled)));
    CHECK(value == 30);
}

TEST_CASE(
    "function_parser: holds its own copy of the callable",
    "[function_parser]"
)
{
    // From an lvalue: copied, so changing the original afterwards changes
    // nothing.
    adds_offset add{5};
    const auto  from_lvalue = pnp::function_parser<int>(add);
    add.offset              = 100;
    int value               = 0;
    CHECK(succeeded(pnp::parse(u8"12", value, from_lvalue)));
    CHECK(value == 7);

    // From an rvalue: moved in.
    const auto from_rvalue = pnp::function_parser<int>(adds_offset{1});
    CHECK(succeeded(pnp::parse(u8"12", value, from_rvalue)));
    CHECK(value == 3);

    // Copying the parser copies the callable with it.
    const auto copy = from_lvalue;
    CHECK(succeeded(pnp::parse(u8"123", value, copy)));
    CHECK(value == 8);
}

TEST_CASE(
    "function_parser: a failure is reported, and the value left alone",
    "[function_parser][errors]"
)
{
    int        value  = 42;
    const auto result = pnp::parse(
        u8"abc", value, pnp::function_parser<int>(count_digits_lambda)
    );
    CHECK(not succeeded(result));
    CHECK(result.position == 0);
    CHECK(value == 42);
}

TEST_CASE(
    "function_parser: a parse_exception inside the callable becomes a failure",
    "[function_parser][errors]"
)
{
    const auto overreaching = pnp::function_parser<int>(
        [](pnp::parser_input& input_arg, int&) -> pnp::basic_parser_return_value
        {
            input_arg.advance(100);
            return {};
        }
    );
    int        value  = 0;
    const auto result = pnp::parse(u8"12", value, overreaching);
    CHECK(
        result.parse_outcome_state
        == pnp::parse_outcome::parse_outcome_state_enum::
            failure_parser_exception
    );
}

TEST_CASE(
    "function_parser: runs other pnps through the parser_input",
    "[function_parser]"
)
{
    // "[digits]": the digits are read by a pnp of their own, run through
    // run_parser as any pnp must be.
    const auto inner     = pnp::function_parser<int>(count_digits_lambda);
    const auto bracketed = pnp::function_parser<int>(
        [inner](pnp::parser_input& input_arg, int& value_arg)
            -> pnp::basic_parser_return_value
        {
            if (not input_arg.advance_if_starts_with(u8"["))
            {
                return input_arg.fail(u8"expected [");
            }
            int count = 0;
            if (auto result = input_arg.run_parser(inner, count);
                result.failed())
            {
                return result;
            }
            if (not input_arg.advance_if_starts_with(u8"]"))
            {
                return input_arg.fail(u8"expected ]");
            }
            value_arg = count;
            return {};
        }
    );

    int value = 0;
    CHECK(succeeded(pnp::parse(u8"[12]", value, bracketed)));
    CHECK(value == 2);

    // A failure inside: reported where it happened, and the value untouched.
    value                 = 42;
    const auto unfinished = pnp::parse(u8"[12", value, bracketed);
    CHECK(not succeeded(unfinished));
    CHECK(unfinished.position == 3);
    CHECK(value == 42);
}

TEST_CASE(
    "function_parser: from a source, a byte at a time",
    "[function_parser][io]"
)
{
    pnp::testing::piece_source source(std::u8string(u8"1234"), 1);
    int                        value = 0;
    CHECK(succeeded(pnp::parse(
        source, value, pnp::function_parser<int>(count_digits_lambda)
    )));
    CHECK(value == 4);
}