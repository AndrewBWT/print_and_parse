#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <cstdint>
#include <limits>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pnp_tests
{
    template <typename T>
    void
        check_integer_round_trip(const T value, std::u8string_view expected)
    {
        const auto text = pnp::to_u8string(value);
        CHECK(text == expected);
        const auto parsed = pnp::try_parse<T>(text);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value);
    }

    // Prints with the given pnp, checks the text, parses it back, and checks the value.
    template <typename P, typename T>
    void
        check_integer_round_trip(const P& pnp_obj, const T value, std::u8string_view expected)
    {
        const auto text = pnp::to_u8string(pnp_obj, value);
        INFO(unicode_bridge::cast_string_view<char>(text));
        INFO(unicode_bridge::cast_string_view<char>(expected));
        CHECK(text == expected);
        const auto parsed = pnp::try_parse<T, P>(text, pnp_obj);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value);
    }

    // Round-trips min, max, zero (and -1 for signed types) for every integer type.
    template <typename T>
    void
        check_integer_limits()
    {
        const auto check_value = [](const T value)
            {
                const auto parsed = pnp::try_parse<T>(pnp::to_u8string(value));
                REQUIRE(succeeded(parsed));
                CHECK(parsed.value == value);
            };
        check_value(std::numeric_limits<T>::min());
        check_value(std::numeric_limits<T>::max());
        check_value(T{ 0 });
        if constexpr (std::is_signed_v<T>)
        {
            check_value(T{ -1 });
        }
    }

    // A user policy that signs positive numbers by default.
    struct always_signed
    {
        static constexpr bool use_plus_for_positive_numbers = true;
    };

    // Rejected: missing its member.
    struct empty_integer_policy
    {
    };

    template <typename T>
    concept integer_pnp_formable = requires { typename pnp::support::integer_pnp<T>; };
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("integers: every type round-trips at its limits", "[integers]")
{
    check_integer_limits<signed char>();
    check_integer_limits<short>();
    check_integer_limits<int>();
    check_integer_limits<long>();
    check_integer_limits<long long>();
    check_integer_limits<unsigned char>();
    check_integer_limits<unsigned short>();
    check_integer_limits<unsigned int>();
    check_integer_limits<unsigned long>();
    check_integer_limits<unsigned long long>();
}

TEST_CASE("integers: printed text", "[integers]")
{
    check_integer_round_trip(42, u8"42");
    check_integer_round_trip(-42, u8"-42");
    check_integer_round_trip(0u, u8"0");
    check_integer_round_trip(std::numeric_limits<int>::min(), u8"-2147483648");   // needs the sign slot
    check_integer_round_trip(std::numeric_limits<unsigned long long>::max(), u8"18446744073709551615");

    // signed char and unsigned char are integers, not characters.
    check_integer_round_trip(static_cast<signed char>(65), u8"65");
    check_integer_round_trip(static_cast<unsigned char>(65), u8"65");
    check_integer_round_trip(std::uint8_t{ 200 }, u8"200");   // the <cstdint> typedefs go through the same path

    // integer_pnp can be used explicitly on a character type, to print it as a number.
    check_integer_round_trip(pnp::support::integer_pnp<char>{}, 'A', u8"65");
}

TEST_CASE("integers: sign handling", "[integers]")
{
    // A leading '+' is accepted for both signed and unsigned.
    const auto plus_signed = pnp::try_parse<int>(u8"+5");
    REQUIRE(succeeded(plus_signed));
    CHECK(plus_signed.value == 5);

    const auto plus_unsigned = pnp::try_parse<unsigned int>(u8"+5");
    REQUIRE(succeeded(plus_unsigned));
    CHECK(plus_unsigned.value == 5u);

    // '-' is rejected for unsigned types, with a clear message.
    const auto negative_unsigned = pnp::try_parse<unsigned int>(u8"-5");
    CHECK(not succeeded(negative_unsigned));
    CHECK(first_message(negative_unsigned) == u8"Negative values cannot be stored in an unsigned type.");

    // "+-" is rejected for both.
    const auto plus_minus = pnp::try_parse<int>(u8"+-5");
    CHECK(not succeeded(plus_minus));
    CHECK(first_message(plus_minus) == u8"Expected a number, but found \"+-\".");
    CHECK(not succeeded(pnp::try_parse<unsigned int>(u8"+-5")));

    // A lone sign has no digits.
    const auto lone_minus = pnp::try_parse<int>(u8"-");
    CHECK(not succeeded(lone_minus));
    CHECK(first_message(lone_minus) == u8"Expected a valid integer, but no valid digits were found.");
    CHECK(not succeeded(pnp::try_parse<int>(u8"+")));
}

TEST_CASE("integers: printing a plus for positive numbers", "[integers]")
{
    using signed_pnp = pnp::support::integer_pnp<int>;
    using unsigned_pnp = pnp::support::integer_pnp<unsigned int>;

    const signed_pnp signed_plus({ .use_plus_for_positive_numbers = true });
    check_integer_round_trip(signed_plus, 5, u8"+5");
    check_integer_round_trip(signed_plus, 0, u8"+0");    // matches the float pnp
    check_integer_round_trip(signed_plus, -5, u8"-5");   // negatives are unaffected
    check_integer_round_trip(signed_plus, std::numeric_limits<int>::max(), u8"+2147483647");
    check_integer_round_trip(signed_plus, std::numeric_limits<int>::min(), u8"-2147483648");
    CHECK(signed_plus.options().use_plus_for_positive_numbers);

    const unsigned_pnp unsigned_plus({ .use_plus_for_positive_numbers = true });
    check_integer_round_trip(unsigned_plus, 5u, u8"+5");
    check_integer_round_trip(unsigned_plus, 0u, u8"+0");
    check_integer_round_trip(unsigned_plus, std::numeric_limits<unsigned int>::max(), u8"+4294967295");

    // Off by default.
    const signed_pnp plain;
    CHECK(not plain.options().use_plus_for_positive_numbers);
    check_integer_round_trip(plain, 5, u8"5");

    // A user policy can turn it on by default; options can still turn it off.
    const pnp::support::integer_pnp<int, always_signed> policy_plus;
    check_integer_round_trip(policy_plus, 5, u8"+5");
    const pnp::support::integer_pnp<int, always_signed> policy_off({ .use_plus_for_positive_numbers = false });
    check_integer_round_trip(policy_off, 5, u8"5");

    // The defaults inherit the options constructor.
    const pnp::default_printer<long long> default_plus({ .use_plus_for_positive_numbers = true });
    CHECK(pnp::to_u8string(default_plus, 7LL) == u8"+7");

    // Signed output composes inside containers.
    const pnp::support::generic_sequence_pnp<std::vector<int>, signed_pnp> sequence({ .element = signed_plus });
    CHECK(pnp::to_u8string(sequence, std::vector<int>{-1, 0, 1}) == u8"[-1,+0,+1]");
}

TEST_CASE("integers: out of range quotes the input", "[integers]")
{
    const auto too_big_signed = pnp::try_parse<signed char>(u8"128");
    CHECK(not succeeded(too_big_signed));
    CHECK(first_message(too_big_signed) == u8"The value \"128\" is out of range for a signed type of 1 byte.");

    const auto too_small_signed = pnp::try_parse<signed char>(u8"-129");
    CHECK(not succeeded(too_small_signed));
    CHECK(first_message(too_small_signed) == u8"The value \"-129\" is out of range for a signed type of 1 byte.");

    const auto too_big_unsigned = pnp::try_parse<unsigned short>(u8"+70000");
    CHECK(not succeeded(too_big_unsigned));
    CHECK(first_message(too_big_unsigned)
        == u8"The value \"+70000\" is out of range for an unsigned type of 2 bytes.");
}

TEST_CASE("integers: malformed input", "[integers]")
{
    CHECK(not succeeded(pnp::try_parse<int>(u8"")));
    CHECK(not succeeded(pnp::try_parse<int>(u8"abc")));
    CHECK(succeeded(pnp::try_parse<int>(u8" 5")));     // no whitespace skipping
    CHECK(not succeeded(pnp::try_parse<int>(u8"5x")));     // trailing input: not a full parse
    CHECK(not succeeded(pnp::try_parse<int>(u8"0x10")));   // hex is fixed_width's job

    // Leading zeros are accepted.
    const auto leading_zeros = pnp::try_parse<int>(u8"007");
    REQUIRE(succeeded(leading_zeros));
    CHECK(leading_zeros.value == 7);
}

TEST_CASE("integers: inside containers", "[integers]")
{
    const std::vector<int> values{ -1, 0, 1, std::numeric_limits<int>::max() };
    const auto text = pnp::to_u8string(values);
    CHECK(text == u8"[-1,0,1,2147483647]");
    const auto parsed = pnp::try_parse<std::vector<int>>(text);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == values);

    // An out-of-range element reports the element and the cause.
    CHECK(not succeeded(pnp::try_parse<std::vector<unsigned char>>(u8"[1,256,3]")));
}

TEST_CASE("read ahead: an integer stops at the first character after it", "[io][read_ahead]")
{
    // The digits, then one more piece to see that the number has ended.
    const auto outcome = parse_from_pieces<int>({ u8"5", u8"\n", u8"6", u8"\n" });
    CHECK(outcome.succeeded);
    CHECK(outcome.value == 5);
    CHECK(outcome.pieces_read == 2);
}

// --- Policies and options ----------------------------------------------------
static_assert(pnp::support::valid_integer_pnp_c<pnp::support::integer_pnp_default>);
static_assert(pnp::support::valid_integer_pnp_c<always_signed>);
static_assert(not pnp::support::valid_integer_pnp_c<empty_integer_policy>);

// Integer options have an empty rule set, so just checks that it fires off.
static_assert(pnp::support::checkable_options_c<pnp::support::integer_pnp_options<pnp::support::integer_pnp_default>>);

// Options for the same policy are shared across integer types.
static_assert(std::same_as<pnp::support::integer_pnp<int>::options_type,
    pnp::support::integer_pnp<unsigned long long>::options_type>);

// --- Which types integer_pnp accepts -----------------------------------------
static_assert(integer_pnp_formable<int>);
static_assert(integer_pnp_formable<unsigned char>);
static_assert(integer_pnp_formable<char>, "allowed explicitly, though not a default");
static_assert(not integer_pnp_formable<bool>, "to_chars has no bool overload");
static_assert(not integer_pnp_formable<double>);

// --- The exact-type guard ----------------------------------------------------
static_assert(pnp::printer_c<pnp::default_printer<int>, int>);
static_assert(not pnp::printer_c<pnp::default_printer<int>, short>);
static_assert(not pnp::printer_c<pnp::default_printer<int>, long long>);
static_assert(not pnp::printer_c<pnp::default_printer<int>, unsigned int>);
static_assert(not pnp::printer_c<pnp::default_printer<int>, bool>);
static_assert(not pnp::printer_c<pnp::default_printer<unsigned int>, int>);

// --- Which types the integer defaults claim ----------------------------------
static_assert(pnp::default_printable_c<signed char>);
static_assert(pnp::default_printable_c<unsigned long long>);
static_assert(not pnp::printer_c<pnp::default_printer<char>, int>,
    "an int must not implicitly convert into the char printer");