#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace pnp_tests
{
// Compare objects bit-for-bit (needed for floats: NaN != NaN, and -0.0 == 0.0).
template <typename T>
bool
    same_bits(
        const T& a,
        const T& b
    )
{
    return std::bit_cast<std::array<unsigned char, sizeof(T)>>(a)
           == std::bit_cast<std::array<unsigned char, sizeof(T)>>(b);
}

template <
    typename T,
    typename Vals = pnp::support::fixed_width_pnp_unsigned_hex>
using fw_t = pnp::support::fixed_width_pnp<T, Vals>;

// Print with the given pnp, check the text, parse it back, check the bits.
template <typename T, typename Vals>
void
    check_round_trip(
        const fw_t<T, Vals>& pnp_obj,
        const T&             value,
        std::u8string_view   expected
    )
{
    const auto text = pnp::to_u8string(pnp_obj, value);
    CHECK(text == expected);
    const auto parsed = pnp::try_parse<T>(text, pnp_obj);
    if (not (succeeded(parsed)))
    {
        int x = 4;
    }
    REQUIRE(succeeded(parsed));
    CHECK(same_bits(parsed.value, value));
}

template <typename T, typename Vals>
fw_t<T, Vals>
    make_fw(
        bool fixed_size = false,
        bool upper      = false
    )
{
    return fw_t<T, Vals>({.upper = upper, .fixed_size_output = fixed_size});
}

// A user policy for an unusual base.
struct base3_policy : pnp::support::fixed_width_pnp_default_base
{
    static constexpr int                base   = 3;
    static constexpr std::u8string_view prefix = u8"t";
};

// Rejected policies.
struct base1_policy : pnp::support::fixed_width_pnp_default_base
{
    static constexpr int                base   = 1;
    static constexpr std::u8string_view prefix = u8"";
};

struct base37_policy : pnp::support::fixed_width_pnp_default_base
{
    static constexpr int                base   = 37;
    static constexpr std::u8string_view prefix = u8"";
};

struct missing_prefix_policy : pnp::support::fixed_width_pnp_default_base
{
    static constexpr int base = 16;
};

struct leading_space_prefix_policy : pnp::support::fixed_width_pnp_default_base
{
    static constexpr int                base   = 16;
    static constexpr std::u8string_view prefix = u8" 0x";
};

struct trailing_space_prefix_policy : pnp::support::fixed_width_pnp_default_base
{
    static constexpr int                base   = 16;
    static constexpr std::u8string_view prefix = u8"0x ";
};

struct invalid_utf8_prefix_policy : pnp::support::fixed_width_pnp_default_base
{
    static constexpr int     base             = 16;
    static constexpr char8_t lone_lead_byte[] = {static_cast<char8_t>(0xC3)};
    static constexpr std::u8string_view prefix{lone_lead_byte, 1};
};

enum class scoped_u16 : std::uint16_t
{
    a = 1,
    b = 500
};

enum unscoped_fixed : int
{
    minus_one = -1,
    zero      = 0
};

// A user type with restricted bit patterns: only 0, 1 and 2 are valid.
enum class small_enum : unsigned char
{
    zero,
    one,
    two
};

// An enum with bool as its underlying type: inherits bool's restriction.
enum class bool_enum : bool
{
    no  = false,
    yes = true
};
} // namespace pnp_tests

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <>
struct bit_pattern_validity<pnp_tests::small_enum>
{
    static constexpr std::u8string_view type_name         = u8"small_enum";
    static constexpr unsigned char      max_valid_pattern = 2;

    static constexpr bool
        is_valid(
            const unsigned char bits_arg
        ) noexcept
    {
        return bits_arg <= max_valid_pattern;
    }
};

PNP_SUPPORT_NS_END
PNP_NS_END

using namespace pnp_tests;
using bin_v = pnp::support::fixed_width_pnp_binary;
using oct_v = pnp::support::fixed_width_pnp_octal;
using dec_v = pnp::support::fixed_width_pnp_unsigned_int;
using hex_v = pnp::support::fixed_width_pnp_unsigned_hex;

TEST_CASE(
    "fixed_width: unsigned integers, every base",
    "[fixed_width]"
)
{
    check_round_trip(
        make_fw<std::uint8_t, dec_v>(), std::uint8_t{255}, u8"255"
    );
    check_round_trip(
        make_fw<std::uint8_t, hex_v>(), std::uint8_t{255}, u8"0xff"
    );
    check_round_trip(
        make_fw<std::uint8_t, bin_v>(), std::uint8_t{255}, u8"0b11111111"
    );
    check_round_trip(
        make_fw<std::uint8_t, oct_v>(), std::uint8_t{255}, u8"o377"
    );

    check_round_trip(
        make_fw<std::uint16_t, hex_v>(), std::uint16_t{0xBEEF}, u8"0xbeef"
    );
    check_round_trip(
        make_fw<std::uint32_t, dec_v>(),
        std::uint32_t{4'000'000'000u},
        u8"4000000000"
    );
    check_round_trip(
        make_fw<std::uint64_t, hex_v>(),
        std::numeric_limits<std::uint64_t>::max(),
        u8"0xffffffffffffffff"
    );
    check_round_trip(make_fw<std::uint64_t, dec_v>(), std::uint64_t{0}, u8"0");

    // Hex is the default policy.
    const fw_t<std::uint8_t> default_policy;
    check_round_trip(default_policy, std::uint8_t{0x2A}, u8"0x2a");
}

TEST_CASE(
    "fixed_width: formatting options",
    "[fixed_width]"
)
{
    // Upper-case hex digits.
    check_round_trip(
        make_fw<std::uint8_t, hex_v>(false, true), std::uint8_t{0xAB}, u8"0xAB"
    );

    // Padding to the full width of each base.
    check_round_trip(
        make_fw<std::uint16_t, hex_v>(true), std::uint16_t{10}, u8"0x000a"
    );
    check_round_trip(
        make_fw<std::uint16_t, dec_v>(true), std::uint16_t{10}, u8"00010"
    );
    check_round_trip(
        make_fw<std::uint8_t, bin_v>(true), std::uint8_t{5}, u8"0b00000101"
    );
    check_round_trip(
        make_fw<std::uint8_t, oct_v>(true), std::uint8_t{5}, u8"o005"
    );

    // A custom prefix, and an empty one.
    check_round_trip(
        fw_t<std::uint8_t, hex_v>({.prefix = u8"#"}),
        std::uint8_t{0x2A},
        u8"#2a"
    );
    check_round_trip(
        fw_t<std::uint8_t, hex_v>({.prefix = u8""}), std::uint8_t{0x2A}, u8"2a"
    );

    // Multi-character, non-ASCII and spaced-inside prefixes.
    check_round_trip(
        fw_t<std::uint8_t, hex_v>({.prefix = u8"hex:"}),
        std::uint8_t{0x2A},
        u8"hex:2a"
    );
    check_round_trip(
        fw_t<std::uint8_t, hex_v>({.prefix = u8"§"}),
        std::uint8_t{0x2A},
        u8"§2a"
    );
    check_round_trip(
        fw_t<std::uint8_t, hex_v>({.prefix = u8"h x"}),
        std::uint8_t{0x2A},
        u8"h x2a"
    );

    // All three options together.
    const fw_t<std::uint16_t, hex_v> all(
        {.prefix = u8"$", .upper = true, .fixed_size_output = true}
    );
    check_round_trip(all, std::uint16_t{0xAB}, u8"$00AB");

    // The options read back as set, with unnamed fields at their policy values.
    const fw_t<std::uint8_t, hex_v> partial({.upper = true});
    CHECK(partial.options().base == 16);
    CHECK(partial.options().prefix == u8"0x");
    CHECK(partial.options().upper);
    CHECK(not partial.options().fixed_size_output);

    // A modified copy of an existing configuration.
    auto modified   = partial.options();
    modified.prefix = u8"#";
    const fw_t<std::uint8_t, hex_v> copy(modified);
    check_round_trip(copy, std::uint8_t{0xAB}, u8"#AB");
}

TEST_CASE(
    "in pieces: fixed-width numbers",
    "[io][pieces][fixed_width]"
)
{
    check_round_trip_in_pieces(std::byte{0x00});
    check_round_trip_in_pieces(std::byte{0xAF});
    check_round_trip_in_pieces(
        std::vector<std::byte>{std::byte{1}, std::byte{0xFF}}
    );
}

TEST_CASE(
    "in pieces: variants",
    "[io][pieces][variants]"
)
{
    using int_or_text = std::variant<int, std::u8string>;
    check_round_trip_in_pieces(int_or_text{12'345});
    check_round_trip_in_pieces(int_or_text{std::u8string(u8"text")});
    check_round_trip_in_pieces(
        std::vector<int_or_text>{1, std::u8string(u8"two"), 3}
    );

    // A multi-digit index, split across pieces, and failures.
    using many = std::
        variant<int, int, int, int, int, int, int, int, int, int, int, int>;
    check_round_trip_in_pieces(many{std::in_place_index<11>, 7});
    check_parses_in_pieces<int_or_text>(u8"2:5"); // no such alternative
    check_parses_in_pieces<int_or_text>(u8"0;5"); // no separator
}

TEST_CASE(
    "fixed_width: any base from 2 to 36",
    "[fixed_width]"
)
{
    // A user policy for base 3.
    check_round_trip(
        fw_t<std::uint8_t, base3_policy>(), std::uint8_t{255}, u8"t100110"
    );

    // Base 36 uses letters beyond 'f', and upper case covers them all.
    const fw_t<std::uint16_t, hex_v> base36({.base = 36, .prefix = u8""});
    check_round_trip(base36, std::uint16_t{35}, u8"z");
    check_round_trip(base36, std::uint16_t{36}, u8"10");

    const fw_t<std::uint16_t, hex_v> base36_upper(
        {.base = 36, .prefix = u8"", .upper = true}
    );
    check_round_trip(base36_upper, std::uint16_t{35}, u8"Z");

    // Changing the base keeps the policy's prefix. This round-trips, but reads
    // oddly, so set both when you mean a different format.
    const fw_t<std::uint8_t, hex_v> hex_policy_base2({.base = 2});
    check_round_trip(hex_policy_base2, std::uint8_t{5}, u8"0x101");
    const fw_t<std::uint8_t, hex_v> both({.base = 2, .prefix = u8"0b"});
    check_round_trip(both, std::uint8_t{5}, u8"0b101");
}

TEST_CASE(
    "fixed_width: padding follows the configured base",
    "[fixed_width]"
)
{
    // The padded width is computed when the pnp is constructed, from the
    // options' base, not the policy's. These would pad to the policy's width if
    // it weren't.

    // 65535 is "1ekf" in base 36: four digits, not the hex policy's four by
    // chance.
    const fw_t<std::uint16_t, hex_v> base36_padded(
        {.base = 36, .prefix = u8"", .fixed_size_output = true}
    );
    check_round_trip(base36_padded, std::uint16_t{35}, u8"000z");

    // A uint8_t in binary pads to 8 digits, where the hex policy alone would
    // pad to 2.
    const fw_t<std::uint8_t, hex_v> base2_padded(
        {.base = 2, .prefix = u8"0b", .fixed_size_output = true}
    );
    check_round_trip(base2_padded, std::uint8_t{5}, u8"0b00000101");

    // A uint32_t in base 3 pads to 21 digits (4294967295 has 21 base-3 digits).
    const fw_t<std::uint32_t, hex_v> base3_padded(
        {.base = 3, .prefix = u8"", .fixed_size_output = true}
    );
    check_round_trip(base3_padded, std::uint32_t{2}, u8"000000000000000000002");

    // A copied pnp keeps its computed width.
    const fw_t<std::uint8_t, hex_v> copied = base2_padded;
    check_round_trip(copied, std::uint8_t{5}, u8"0b00000101");

    // The defaults use the same constructors, so they compute the width too.
    const pnp::default_printer<std::byte> byte_upper({.upper = true});
    CHECK(pnp::to_u8string(byte_upper, std::byte{0xab}) == u8"0xAB");
    const pnp::default_printer<std::byte> byte_binary(
        {.base = 2, .prefix = u8"0b"}
    );
    CHECK(pnp::to_u8string(byte_binary, std::byte{0x05}) == u8"0b00000101");
}

TEST_CASE(
    "fixed_width: invalid options throw on construction",
    "[fixed_width]"
)
{
    // The base must be between 2 and 36.
    CHECK_THROWS_AS(fw_t<std::uint8_t>({.base = 1}), std::invalid_argument);
    CHECK_THROWS_AS(fw_t<std::uint8_t>({.base = 0}), std::invalid_argument);
    CHECK_THROWS_AS(fw_t<std::uint8_t>({.base = -16}), std::invalid_argument);
    CHECK_THROWS_AS(fw_t<std::uint8_t>({.base = 37}), std::invalid_argument);
    CHECK_NOTHROW(fw_t<std::uint8_t>({.base = 2}));
    CHECK_NOTHROW(fw_t<std::uint8_t>({.base = 36}));

    // The prefix may be empty or start with a digit, because the parser strips
    // exactly what the printer wrote...
    CHECK_NOTHROW(fw_t<std::uint8_t>({.prefix = u8""}));
    CHECK_NOTHROW(fw_t<std::uint8_t>({.prefix = u8"1"}));

    // ...but it must be valid UTF-8, with no whitespace at either end.
    CHECK_THROWS_AS(
        fw_t<std::uint8_t>({.prefix = u8" 0x"}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        fw_t<std::uint8_t>({.prefix = u8"0x "}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        fw_t<std::uint8_t>({.prefix = u8"\t"}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        fw_t<std::uint8_t>({.prefix = half_a_character}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        fw_t<std::uint8_t>({.prefix = u8"0x" + half_a_character}),
        std::invalid_argument
    );
}

TEST_CASE(
    "fixed_width: construction errors name the rule that was broken",
    "[fixed_width]"
)
{
    CHECK(
        invalid_argument_message(
            []
            {
                fw_t<std::uint8_t> p({.base = 37});
            }
        )
        == "fixed_width_pnp_options base must be between 2 and 36"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                fw_t<std::uint8_t> p({.prefix = half_a_character});
            }
        )
        == "fixed_width_pnp_options prefix must be valid UTF-8"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                fw_t<std::uint8_t> p({.prefix = u8"0x "});
            }
        )
        == "fixed_width_pnp_options prefix must not start or end with "
           "whitespace"
    );

    // The base is checked first.
    CHECK(
        invalid_argument_message(
            []
            {
                fw_t<std::uint8_t> p({.base = 1, .prefix = u8" "});
            }
        )
        == "fixed_width_pnp_options base must be between 2 and 36"
    );
}

TEST_CASE(
    "fixed_width: signed integers print their bit pattern",
    "[fixed_width]"
)
{
    check_round_trip(make_fw<std::int8_t, hex_v>(), std::int8_t{-1}, u8"0xff");
    check_round_trip(
        make_fw<std::int16_t, hex_v>(), std::int16_t{-2}, u8"0xfffe"
    );
    check_round_trip(
        make_fw<std::int32_t, hex_v>(), std::int32_t{-1}, u8"0xffffffff"
    );
    check_round_trip(
        make_fw<std::int64_t, dec_v>(),
        std::numeric_limits<std::int64_t>::min(),
        u8"9223372036854775808"
    );
}

TEST_CASE(
    "fixed_width: floating point is exact, including special values",
    "[fixed_width]"
)
{
    check_round_trip(make_fw<float, hex_v>(), 1.0f, u8"0x3f800000");
    check_round_trip(make_fw<double, hex_v>(), 1.0, u8"0x3ff0000000000000");
    check_round_trip(make_fw<double, hex_v>(), -0.0, u8"0x8000000000000000");
    check_round_trip(
        make_fw<double, hex_v>(),
        std::numeric_limits<double>::infinity(),
        u8"0x7ff0000000000000"
    );

    // NaN payloads survive bit-for-bit, which the generic float pnp
    // deliberately doesn't do.
    const double nan_with_payload
        = std::bit_cast<double>(std::uint64_t{0x7ff8'0000'0000'0123});
    check_round_trip(
        make_fw<double, hex_v>(), nan_with_payload, u8"0x7ff8000000000123"
    );
}

TEST_CASE(
    "fixed_width: characters, bytes and pointers",
    "[fixed_width]"
)
{
    check_round_trip(make_fw<char, hex_v>(), 'A', u8"0x41");
    check_round_trip(make_fw<char32_t, hex_v>(), U'\U0001F600', u8"0x1f600");
    check_round_trip(
        make_fw<std::byte, hex_v>(true), std::byte{0x2a}, u8"0x2a"
    );

    // The std::byte default pads to two hex digits.
    CHECK(pnp::to_u8string(std::byte{0x2a}) == u8"0x2a");
    CHECK(pnp::to_u8string(std::byte{0x05}) == u8"0x05");
    const auto parsed_byte = pnp::try_parse<std::byte>(u8"0x05");
    REQUIRE(succeeded(parsed_byte));
    CHECK(std::to_integer<int>(parsed_byte.value) == 0x05);

    int        target = 0;
    const auto p      = make_fw<int*, hex_v>();
    const auto text   = pnp::to_u8string(p, &target);
    const auto parsed = pnp::try_parse<int*>(text, p);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == &target); // same address, not just the same bits

    check_round_trip(
        make_fw<int*, hex_v>(), static_cast<int*>(nullptr), u8"0x0"
    );
}

TEST_CASE(
    "fixed_width: enums",
    "[fixed_width]"
)
{
    // Scoped enum with a fixed underlying type: every value is valid,
    // including values that aren't enumerators.
    check_round_trip(make_fw<scoped_u16, dec_v>(), scoped_u16::b, u8"500");
    check_round_trip(
        make_fw<scoped_u16, dec_v>(), static_cast<scoped_u16>(600), u8"600"
    );

    // Unscoped enum with a fixed underlying type.
    check_round_trip(
        make_fw<unscoped_fixed, hex_v>(),
        unscoped_fixed::minus_one,
        u8"0xffffffff"
    );

    // A user-restricted enum: values within the trait's rule parse; others
    // fail.
    const auto small = make_fw<small_enum, dec_v>();
    check_round_trip(small, small_enum::two, u8"2");
    const auto bad_small = pnp::try_parse<small_enum>(u8"3", small);
    CHECK(not succeeded(bad_small));
    CHECK(
        first_message(bad_small)
        == u8"The value \"3\" is not a valid small_enum."
    );

    // An enum with bool as its underlying type: only included where the
    // platform considers bool to have unique object representations.
    if constexpr (pnp::support::bit_castable_to_unsigned_c<bool_enum>)
    {
        check_round_trip(make_fw<bool_enum, dec_v>(), bool_enum::yes, u8"1");
        check_round_trip(
            make_fw<bool_enum, bin_v>(true), bool_enum::no, u8"0b0"
        ); // one digit, not eight

        const auto bool_enum_dec = make_fw<bool_enum, dec_v>();
        const auto invalid = pnp::try_parse<bool_enum>(u8"2", bool_enum_dec);
        CHECK(not succeeded(invalid));
        CHECK(
            first_message(invalid) == u8"The value \"2\" is not a valid bool."
        );
    }
}

TEST_CASE(
    "fixed_width: bool and its invalid bit patterns",
    "[fixed_width]"
)
{
    check_round_trip(make_fw<bool, dec_v>(), true, u8"1");
    check_round_trip(make_fw<bool, dec_v>(), false, u8"0");
    check_round_trip(make_fw<bool, hex_v>(true), true, u8"0x1");

    const auto dec     = make_fw<bool, dec_v>();
    const auto bad_dec = pnp::try_parse<bool>(u8"2", dec);
    CHECK(not succeeded(bad_dec));
    CHECK(first_message(bad_dec) == u8"The value \"2\" is not a valid bool.");

    const auto hex     = make_fw<bool, hex_v>();
    const auto bad_hex = pnp::try_parse<bool>(u8"0x2", hex);
    CHECK(not succeeded(bad_hex));
    CHECK(first_message(bad_hex) == u8"The value \"0x2\" is not a valid bool.");

    // Only one digit is read: "0" is false, and the "2" is left over.
    CHECK(not succeeded(pnp::try_parse<bool>(u8"0x02", hex)));
}

TEST_CASE(
    "fixed_width: parse errors and leniency",
    "[fixed_width]"
)
{
    const auto hex8      = make_fw<std::uint8_t, hex_v>();
    const auto dec8      = make_fw<std::uint8_t, dec_v>();

    // Missing prefix.
    const auto no_prefix = pnp::try_parse<std::uint8_t>(u8"ff", hex8);
    CHECK(not succeeded(no_prefix));
    CHECK(first_message(no_prefix) == u8"Unable to detect prefix \"0x\".");

    // No digits, or a bad digit: the message names the base and the prefix.
    for (const std::u8string_view input : {u8"0x", u8"0xg"})
    {
        INFO(std::string(input.begin(), input.end()));
        const auto parsed = pnp::try_parse<std::uint8_t>(input, hex8);
        CHECK(not succeeded(parsed));
        CHECK(
            first_message(parsed)
            == u8"Expected a valid hexadecimal integer (with prefix \"0x\"), "
               u8"but no valid digits were found."
        );
    }
    const auto no_digits_dec = pnp::try_parse<std::uint8_t>(u8"x", dec8);
    CHECK(
        first_message(no_digits_dec)
        == u8"Expected a valid decimal integer, but no valid digits were found."
    );

    // An unusual base is named by number.
    const fw_t<std::uint8_t, base3_policy> base3;
    const auto bad_base3 = pnp::try_parse<std::uint8_t>(u8"t3", base3);
    CHECK(
        first_message(bad_base3)
        == u8"Expected a valid base-3 integer (with prefix \"t\"), but no "
           u8"valid digits were found."
    );

    // Too large, with the offending text quoted as written.
    const auto too_big = pnp::try_parse<std::uint8_t>(u8"256", dec8);
    CHECK(not succeeded(too_big));
    CHECK(
        first_message(too_big)
        == u8"The value \"256\" is too large for a target type of 1 byte."
    );
    // In hex, only two digits are read, so "0x100" reads "10" and leaves "0"
    // over.
    CHECK(not succeeded(pnp::try_parse<std::uint8_t>(u8"0x100", hex8)));

    // Negative numbers are rejected: the digits are an unsigned bit pattern.
    CHECK(not succeeded(pnp::try_parse<std::uint8_t>(u8"-1", hex8)));

    // Lenient on width: fewer digits than the padded form is fine.
    const auto hex16      = make_fw<std::uint16_t, hex_v>(true);
    const auto short_form = pnp::try_parse<std::uint16_t>(u8"0x1", hex16);
    REQUIRE(succeeded(short_form));
    CHECK(short_form.value == 1);

    // Trailing text means the whole input wasn't consumed.
    CHECK(not succeeded(pnp::try_parse<std::uint8_t>(u8"0x1fz", hex8)));
}

TEST_CASE(
    "fixed_width: the parser never reads more digits than a valid value needs",
    "[fixed_width]"
)
{
    const auto hex8 = make_fw<std::uint8_t, hex_v>();
    const auto dec8 = make_fw<std::uint8_t, dec_v>();

    // Extra leading zeros beyond the width are left over, so these are
    // incomplete parses.
    CHECK(not succeeded(pnp::try_parse<std::uint8_t>(u8"0x005", hex8)));
    CHECK(not succeeded(pnp::try_parse<std::uint8_t>(u8"0255", hex8)));

    // Within the width, leading zeros are still fine.
    const auto padded = pnp::try_parse<std::uint8_t>(u8"007", dec8);
    REQUIRE(succeeded(padded));
    CHECK(padded.value == 7);

    // A bool needs one digit in any base, so a padded bool is one digit too.
    check_round_trip(make_fw<bool, bin_v>(true), true, u8"0b1");
    check_round_trip(make_fw<bool, dec_v>(true), false, u8"0");
}

TEST_CASE(
    "fixed_width: a declared maximum sets the width",
    "[fixed_width]"
)
{
    // small_enum declares 2 as its largest valid pattern: two binary digits,
    // not eight.
    check_round_trip(
        make_fw<small_enum, bin_v>(true), small_enum::one, u8"0b01"
    );
    check_round_trip(
        make_fw<small_enum, bin_v>(true), small_enum::two, u8"0b10"
    );

    // In decimal it's one digit, so "02" leaves the "2" over.
    const auto small = make_fw<small_enum, dec_v>();
    CHECK(not succeeded(pnp::try_parse<small_enum>(u8"02", small)));
}

TEST_CASE(
    "fixed_width: a fixed output width when padded",
    "[fixed_width]"
)
{
    // Unpadded output varies in width: 0xa and 0xaf.
    CHECK(not make_fw<std::uint8_t, hex_v>().fixed_output_width().has_value());

    // Padded: the prefix plus every digit the largest valid value needs.
    CHECK(
        make_fw<std::uint8_t, hex_v>(true).fixed_output_width() == 4
    ); // 0x + 2
    CHECK(
        make_fw<std::uint16_t, dec_v>(true).fixed_output_width() == 5
    ); // 65535
    CHECK(
        make_fw<std::uint8_t, bin_v>(true).fixed_output_width() == 10
    ); // 0b + 8
    CHECK(
        make_fw<std::uint64_t, hex_v>(true).fixed_output_width() == 18
    ); // 0x + 16

    // A declared maximum narrows it: bool and small_enum need one decimal
    // digit.
    CHECK(make_fw<bool, bin_v>(true).fixed_output_width() == 3); // 0b + 1
    CHECK(make_fw<small_enum, dec_v>(true).fixed_output_width() == 1);

    // The prefix counts in code units: "§" is two.
    CHECK(
        fw_t<std::uint8_t, hex_v>({.prefix = u8"§", .fixed_size_output = true})
            .fixed_output_width()
        == 4
    );

    // It follows the configured base and prefix, not the policy's.
    CHECK(
        fw_t<std::uint16_t, hex_v>(
            {.base = 36, .prefix = u8"", .fixed_size_output = true}
        )
            .fixed_output_width()
        == 4
    );

    // Copies keep it, and the std::byte default is padded.
    const auto padded = make_fw<std::uint8_t, hex_v>(true);
    const auto copy   = padded;
    CHECK(copy.fixed_output_width() == 4);
    CHECK(pnp::default_printer<std::byte>{}.fixed_output_width() == 4);

    // Every value printed really has that width, and parses back consuming
    // exactly that much.
    for (int value = 0; value <= 0xff; ++value)
    {
        INFO(value);
        const auto text
            = pnp::to_u8string(padded, static_cast<std::uint8_t>(value));
        CHECK(text.size() == 4);
        const auto parsed = pnp::try_parse<std::uint8_t>(text, padded);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value);
    }
}

TEST_CASE(
    "fixed_width: a moved-from pnp keeps a width matching its options",
    "[fixed_width]"
)
{
    // A binary pnp, moved from: the new one prints in binary, padded to 8
    // digits.
    fw_t<std::uint8_t, hex_v> source(
        {.base = 2, .prefix = u8"0b", .fixed_size_output = true}
    );
    const fw_t<std::uint8_t, hex_v> moved = std::move(source);
    CHECK(pnp::to_u8string(moved, std::uint8_t{5}) == u8"0b00000101");

    // The source is back to the policy's defaults (hex, unpadded), with a width
    // to match.
    CHECK(source.options().base == 16);
    CHECK(pnp::to_u8string(source, std::uint8_t{5}) == u8"0x5");
    const fw_t<std::uint8_t, hex_v> padded_source = [&]
    {
        auto options              = source.options();
        options.fixed_size_output = true;
        return fw_t<std::uint8_t, hex_v>(options);
    }();
    CHECK(pnp::to_u8string(padded_source, std::uint8_t{5}) == u8"0x05");

    // Move assignment, including onto itself.
    fw_t<std::uint8_t, hex_v> target;
    target = fw_t<std::uint8_t, hex_v>(
        {.base = 2, .prefix = u8"0b", .fixed_size_output = true}
    );
    CHECK(pnp::to_u8string(target, std::uint8_t{5}) == u8"0b00000101");
    target = std::move(target);
    CHECK(pnp::to_u8string(target, std::uint8_t{5}) == u8"0b00000101");
}

// --- Policy validation at compile time ---------------------------------------
static_assert(pnp::support::valid_fixed_width_pnp_c<bin_v>);
static_assert(pnp::support::valid_fixed_width_pnp_c<oct_v>);
static_assert(pnp::support::valid_fixed_width_pnp_c<dec_v>);
static_assert(pnp::support::valid_fixed_width_pnp_c<hex_v>);
static_assert(pnp::support::valid_fixed_width_pnp_c<base3_policy>);
static_assert(not pnp::support::valid_fixed_width_pnp_c<base1_policy>);
static_assert(not pnp::support::valid_fixed_width_pnp_c<base37_policy>);
static_assert(not pnp::support::valid_fixed_width_pnp_c<missing_prefix_policy>);
static_assert(not pnp::support::valid_fixed_width_pnp_c<
              leading_space_prefix_policy>);
static_assert(not pnp::support::valid_fixed_width_pnp_c<
              trailing_space_prefix_policy>);
static_assert(not pnp::support::valid_fixed_width_pnp_c<
              invalid_utf8_prefix_policy>);

// Validation stays wired up.
static_assert(pnp::support::checkable_options_c<
              pnp::support::fixed_width_pnp_options<hex_v>>);

// Options for the same policy are the same type whatever T is.
static_assert(std::same_as<
              fw_t<std::uint8_t, hex_v>::options_type,
              fw_t<double, hex_v>::options_type>);
static_assert(not std::same_as<
              fw_t<std::uint8_t, hex_v>::options_type,
              fw_t<std::uint8_t, dec_v>::options_type>);

// --- What the concept accepts and rejects ------------------------------------
static_assert(pnp::support::bit_castable_to_unsigned_c<std::uint64_t>);
static_assert(pnp::support::bit_castable_to_unsigned_c<double>);
static_assert(pnp::support::bit_castable_to_unsigned_c<bool>);
static_assert(pnp::support::bit_castable_to_unsigned_c<std::byte>);
static_assert(pnp::support::bit_castable_to_unsigned_c<scoped_u16>);
static_assert(pnp::support::bit_castable_to_unsigned_c<int*>);
static_assert(
    not pnp::support::bit_castable_to_unsigned_c<std::string>,
    "not trivially copyable"
);
static_assert(
    not pnp::support::bit_castable_to_unsigned_c<std::array<char, 3>>,
    "no 3-byte unsigned type"
);

struct padded
{
    char c;
    int  i;
};

static_assert(
    not pnp::support::bit_castable_to_unsigned_c<padded>,
    "padding bits vary between equal values"
);

// --- The exact-type guard
// ------------------------------------------------------
static_assert(pnp::printer_c<fw_t<std::uint32_t, hex_v>, std::uint32_t>);
static_assert(not pnp::printer_c<fw_t<std::uint32_t, hex_v>, std::uint16_t>);
static_assert(not pnp::printer_c<fw_t<bool, dec_v>, int>);

// --- Enums over bool
// ------------------------------------------------------------ An enum over
// bool has exactly bool's valid patterns and maximum.
static_assert(pnp::support::bit_pattern_validity<bool_enum>::is_valid(0));
static_assert(pnp::support::bit_pattern_validity<bool_enum>::is_valid(1));
static_assert(not pnp::support::bit_pattern_validity<bool_enum>::is_valid(2));
static_assert(pnp::support::internal::max_valid_pattern<bool_enum>() == 1);

// Other enums are unaffected.
static_assert(pnp::support::bit_pattern_validity<scoped_u16>::is_valid(600));
static_assert(pnp::support::fixed_output_width_c<fw_t<std::uint8_t, hex_v>>);
static_assert(pnp::support::fixed_output_width_c<
              pnp::default_printer<std::byte>>);