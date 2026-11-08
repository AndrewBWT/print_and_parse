#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pnp_tests
{
    // Prints with the default pnp, checks the text, parses it back, and checks the value.
    inline void
        check_byte_round_trip(const std::byte value, std::u8string_view expected)
    {
        const auto text = pnp::to_u8string(value);
        CHECK(text == expected);
        const auto parsed = pnp::try_parse<std::byte>(text);
        REQUIRE(succeeded(parsed));
        CHECK(std::to_integer<int>(parsed.value) == std::to_integer<int>(value));
    }

    // The same, with a given pnp.
    template <typename P>
    void
        check_byte_round_trip_with(const P& pnp_obj, const std::byte value, std::u8string_view expected)
    {
        const auto text = pnp::to_u8string(pnp_obj, value);
        CHECK(text == expected);
        const auto parsed = pnp::try_parse<std::byte>(text, pnp_obj);
        REQUIRE(succeeded(parsed));
        CHECK(std::to_integer<int>(parsed.value) == std::to_integer<int>(value));
    }

    using byte_pnp = pnp::default_printer<std::byte>;
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("byte: prints as two padded hex digits", "[byte]")
{
    check_byte_round_trip(std::byte{ 0x00 }, u8"0x00");
    check_byte_round_trip(std::byte{ 0x05 }, u8"0x05");
    check_byte_round_trip(std::byte{ 0x2a }, u8"0x2a");
    check_byte_round_trip(std::byte{ 0x80 }, u8"0x80");
    check_byte_round_trip(std::byte{ 0xff }, u8"0xff");
}

TEST_CASE("byte: every value round-trips", "[byte]")
{
    for (int value = 0; value <= 0xff; ++value)
    {
        INFO(value);
        const auto byte = static_cast<std::byte>(value);
        const auto parsed = pnp::try_parse<std::byte>(pnp::to_u8string(byte));
        REQUIRE(succeeded(parsed));
        CHECK(std::to_integer<int>(parsed.value) == value);
    }
}

TEST_CASE("byte: parsing is lenient on case, and on width up to two digits", "[byte]")
{
    // Fewer digits than the padded form.
    const auto short_form = pnp::try_parse<std::byte>(u8"0x5");
    REQUIRE(succeeded(short_form));
    CHECK(std::to_integer<int>(short_form.value) == 5);

    // A byte never needs more than two hex digits, so only two are read: extra leading
    // zeros leave a digit over, and the parse is incomplete.
    CHECK(not succeeded(pnp::try_parse<std::byte>(u8"0x005")));
    CHECK(not succeeded(pnp::try_parse<std::byte>(u8"0x0ff")));

    // Upper-case digits.
    const auto upper = pnp::try_parse<std::byte>(u8"0xAB");
    REQUIRE(succeeded(upper));
    CHECK(std::to_integer<int>(upper.value) == 0xab);
}

TEST_CASE("byte: parse errors", "[byte]")
{
    // A missing prefix.
    const auto no_prefix = pnp::try_parse<std::byte>(u8"2a");
    CHECK(not succeeded(no_prefix));
    CHECK(first_message(no_prefix) == u8"Unable to detect prefix \"0x\".");

    // No digits, or a bad digit.
    for (const std::u8string_view input : {u8"0x", u8"0xg"})
    {
        INFO(std::string(input.begin(), input.end()));
        const auto parsed = pnp::try_parse<std::byte>(input);
        CHECK(not succeeded(parsed));
        CHECK(first_message(parsed)
            == u8"Expected a valid hexadecimal integer (with prefix \"0x\"), but no valid digits were found.");
    }

    // Too large for a byte, with the input quoted as written.
    const auto too_big = pnp::try_parse<std::byte>(u8"0x100");
    // Too many digits: only two are read, so "0x100" leaves its last digit over.
    CHECK(not succeeded(pnp::try_parse<std::byte>(u8"0x100")));

    // Signs, whitespace, trailing input and empty input.
    CHECK(not succeeded(pnp::try_parse<std::byte>(u8"-0x01")));
    CHECK(succeeded(pnp::try_parse<std::byte>(u8" 0x01")));
    CHECK(not succeeded(pnp::try_parse<std::byte>(u8"0x01g")));
    CHECK(not succeeded(pnp::try_parse<std::byte>(u8"")));
}

TEST_CASE("byte: configuring the default through options", "[byte]")
{
    // Upper-case digits.
    check_byte_round_trip_with(byte_pnp({ .upper = true }), std::byte{ 0xab }, u8"0xAB");

    // No padding.
    check_byte_round_trip_with(byte_pnp({ .fixed_size_output = false }), std::byte{ 0x05 }, u8"0x5");

    // A custom prefix, and none at all.
    check_byte_round_trip_with(byte_pnp({ .prefix = u8"$" }), std::byte{ 0x2a }, u8"$2a");
    check_byte_round_trip_with(byte_pnp({ .prefix = u8"" }), std::byte{ 0x2a }, u8"2a");

    // Binary and decimal, padded to the width of a byte in each base.
    check_byte_round_trip_with(byte_pnp({ .base = 2, .prefix = u8"0b" }), std::byte{ 0x05 }, u8"0b00000101");
    check_byte_round_trip_with(byte_pnp({ .base = 10, .prefix = u8"" }), std::byte{ 0x05 }, u8"005");

    // The options read back, with unnamed fields at the byte policy's values.
    const byte_pnp upper({ .upper = true });
    CHECK(upper.options().base == 16);
    CHECK(upper.options().prefix == u8"0x");
    CHECK(upper.options().fixed_size_output);

    // Invalid options throw.
    CHECK_THROWS_AS(byte_pnp({ .base = 1 }), std::invalid_argument);
    CHECK_THROWS_AS(byte_pnp({ .prefix = u8" 0x" }), std::invalid_argument);
    // In decimal, three digits are read, and those can exceed 255.
    const byte_pnp decimal({ .base = 10, .prefix = u8"" });
    const auto     too_big = pnp::try_parse<std::byte>(u8"256", decimal);
    CHECK(not succeeded(too_big));
    CHECK(first_message(too_big) == u8"The value \"256\" is too large for a target type of 1 byte.");
}

TEST_CASE("byte: inside containers", "[byte]")
{
    // A vector of bytes.
    const std::vector<std::byte> bytes{ std::byte{0x01}, std::byte{0x2a}, std::byte{0xff} };
    const auto text = pnp::to_u8string(bytes);
    CHECK(text == u8"[0x01,0x2a,0xff]");
    const auto parsed = pnp::try_parse<std::vector<std::byte>>(text);
    REQUIRE(succeeded(parsed));
    CHECK((parsed.value == bytes));

    // A built-in array of bytes is a sequence, not text: std::byte isn't a character type.
    const std::byte array[3] = { std::byte{0x00}, std::byte{0x10}, std::byte{0x20} };
    CHECK(pnp::to_u8string(array) == u8"[0x00,0x10,0x20]");

    // An empty vector.
    CHECK(pnp::to_u8string(std::vector<std::byte>{}) == u8"[]");
}

// --- The default ---------------------------------------------------------------
static_assert(pnp::default_printable_c<std::byte>);
static_assert(pnp::default_parseable_c<std::byte>);
static_assert(std::is_base_of_v<pnp::support::fixed_width_pnp<std::byte, pnp::support::fixed_width_pnp_byte_hex>,
    pnp::default_printer<std::byte>>);
static_assert(pnp::support::valid_fixed_width_pnp_c<pnp::support::fixed_width_pnp_byte_hex>);

// --- The exact-type guard ----------------------------------------------------
static_assert(pnp::printer_c<byte_pnp, std::byte>);
static_assert(not pnp::printer_c<byte_pnp, unsigned char>);
static_assert(not pnp::printer_c<byte_pnp, int>);
static_assert(not pnp::printer_c<byte_pnp, char8_t>);
static_assert(not pnp::printer_c<pnp::default_printer<unsigned char>, std::byte>);