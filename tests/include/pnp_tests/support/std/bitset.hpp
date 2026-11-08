#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <bitset>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pnp_tests
{
using bitset8_pnp = pnp::support::generic_bitset_pnp<8>;

template <typename P, std::size_t N>
void
    check_bitset_round_trip_with(
        const P&              pnp_arg,
        const std::bitset<N>& value_arg,
        std::u8string_view    expected_arg
    )
{
    const auto text = pnp::to_u8string(pnp_arg, value_arg);
    CHECK(text == expected_arg);
    const auto parsed = pnp::try_parse<std::bitset<N>>(text, pnp_arg);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value_arg);
}

// The delimited form, where fewer bits may be parsed.
inline bitset8_pnp
    bracketed_bitset8()
{
    return bitset8_pnp({.open_delimiter = u8"[", .close_delimiter = u8"]"});
}
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "bitset: flat, most significant bit first",
    "[bitset]"
)
{
    CHECK(pnp::to_u8string(std::bitset<8>(5)) == u8"00000101");
    CHECK(pnp::to_u8string(std::bitset<1>(1)) == u8"1");
    CHECK(pnp::to_u8string(std::bitset<0>{}) == u8"");

    // Exactly as std::bitset writes itself.
    const std::bitset<12> value(0xA5C);
    const std::string     standard = value.to_string();
    CHECK(
        pnp::to_u8string(value)
        == std::u8string(standard.begin(), standard.end())
    );

    for (const unsigned long bits : {0ul, 1ul, 5ul, 128ul, 255ul})
    {
        check_bitset_round_trip_with(
            bitset8_pnp{},
            std::bitset<8>(bits),
            pnp::to_u8string(std::bitset<8>(bits))
        );
    }
    check_bitset_round_trip_with(
        pnp::support::generic_bitset_pnp<0>{}, std::bitset<0>{}, u8""
    );
}

TEST_CASE(
    "bitset: flat parsing needs exactly N bits",
    "[bitset]"
)
{
    CHECK(not succeeded(pnp::try_parse<std::bitset<8>>(u8"0000101"))); // seven
    CHECK(not succeeded(pnp::try_parse<std::bitset<8>>(u8"000001011"))); // nine
    CHECK(not succeeded(pnp::try_parse<std::bitset<8>>(u8"00000201"))
    ); // not a bit
    CHECK(not succeeded(pnp::try_parse<std::bitset<8>>(u8"")));
}

TEST_CASE(
    "in pieces: fixed-count sequences, parsed by count",
    "[io][pieces][sequences]"
)
{
    // Flat bitsets: no delimiters, so the count says where they end.
    check_round_trip_in_pieces(std::bitset<8>(0b1011'0001));
    check_round_trip_in_pieces(std::bitset<1>(1));
    check_parses_in_pieces<std::bitset<8>>(u8"1011");     // too few
    check_parses_in_pieces<std::bitset<8>>(u8"10110002"); // not a bit

    // Arrays.
    check_round_trip_in_pieces(std::array<int, 3>{7, 8, 9});
    check_parses_in_pieces<std::array<int, 3>>(u8"[7,8]"); // too few
}

TEST_CASE(
    "bitset: delimited, fewer bits fill the low end",
    "[bitset]"
)
{
    check_bitset_round_trip_with(
        bracketed_bitset8(), std::bitset<8>(5), u8"[00000101]"
    );

    const auto short_bits
        = pnp::try_parse<std::bitset<8>>(u8"[101]", bracketed_bitset8());
    REQUIRE(succeeded(short_bits));
    CHECK(short_bits.value == std::bitset<8>(0b101));

    const auto none
        = pnp::try_parse<std::bitset<8>>(u8"[]", bracketed_bitset8());
    REQUIRE(succeeded(none));
    CHECK(none.value.none());

    const auto too_many
        = pnp::try_parse<std::bitset<8>>(u8"[111111111]", bracketed_bitset8());
    CHECK(
        first_message(too_many)
        == u8"Could not add element 8 to the sequence: Too many bits; a bitset "
           u8"of 8 holds at most 8."
    );
}

TEST_CASE(
    "bitset: configured like any sequence",
    "[bitset]"
)
{
    const std::bitset<4> value(0b1010);

    // A separator, and spacing.
    using bitset4_pnp = pnp::support::generic_bitset_pnp<4>;
    check_bitset_round_trip_with(
        bitset4_pnp({.separator = u8","}), value, u8"1,0,1,0"
    );
    check_bitset_round_trip_with(
        bitset4_pnp({.separator = u8",", .separator_spacing = u8" "}),
        value,
        u8"1, 0, 1, 0"
    );

    // Letters as bits, through a bool pnp with fixed-width words.
    using letters_pnp = pnp::support::generic_bitset_pnp<
        4,
        pnp::support::bool_pnp<pnp::support::bool_pnp_vals_letter_uc>>;
    check_bitset_round_trip_with(letters_pnp{}, value, u8"TFTF");

    // Lenient whitespace between bits.
    const bitset4_pnp lenient({.skip_whitespace = true});
    const auto spaced = pnp::try_parse<std::bitset<4>>(u8"1 0 1\t0", lenient);
    REQUIRE(succeeded(spaced));
    CHECK(spaced.value == value);
}

namespace pnp_tests
{
// Bits as words need a separator, and so delimiters too:
// [true,false,false,false].
struct bracketed_bitset_policy : pnp::support::sequence_pnp_default_base
{
    static constexpr std::u8string_view open_delimiter  = u8"[";
    static constexpr std::u8string_view close_delimiter = u8"]";
    static constexpr std::u8string_view separator       = u8",";
};
} // namespace pnp_tests

TEST_CASE(
    "bitset: bits as words need a separator",
    "[bitset]"
)
{
    using words_pnp = pnp::support::generic_bitset_pnp<
        4,
        pnp::support::bool_pnp<>,
        bracketed_bitset_policy>;
    check_bitset_round_trip_with(
        words_pnp{}, std::bitset<4>(0b1000), u8"[true,false,false,false]"
    );

    // Fewer words fill the low end too, since the delimiters mark the end.
    const auto short_bits
        = pnp::try_parse<std::bitset<4>, words_pnp>(u8"[true,false]");
    REQUIRE(succeeded(short_bits));
    CHECK(short_bits.value == std::bitset<4>(0b10));

    // Dropping the separator is refused when the options are built.
    CHECK_THROWS_AS(words_pnp({.separator = u8""}), std::invalid_argument);

    // A bool's single digit has a fixed width.
    CHECK(pnp::support::bitset_bit_pnp{}.fixed_output_width() == 1);
}

// The flat default policy with word bits can't even be formed: its default
// options break the rules, which is checked when the type is used, not when an
// object is built.
static_assert(not pnp::support::valid_default_options_c<
              pnp::support::sequence_pnp_options<
                  pnp::support::bool_pnp<>,
                  pnp::support::bitset_pnp_default,
                  std::bitset<4>>>);

TEST_CASE(
    "bitset: fixed output widths",
    "[bitset][fixed_width]"
)
{
    CHECK(bitset8_pnp{}.fixed_output_width() == 8);
    CHECK(bracketed_bitset8().fixed_output_width() == 10);
    CHECK(pnp::support::generic_bitset_pnp<0>{}.fixed_output_width() == 0);

    // So bitsets pack into other sequences: [0001,1100], or flat.
    const std::vector<std::bitset<4>> values{
        std::bitset<4>(1), std::bitset<4>(12)
    };
    CHECK(pnp::to_u8string(values) == u8"[0001,1100]");
    using packed_pnp = pnp::default_printer<std::vector<std::bitset<4>>>;
    CHECK(
        pnp::to_u8string(packed_pnp({.separator = u8""}), values)
        == u8"[00011100]"
    );
}

// --- Defaults
// ------------------------------------------------------------------------
static_assert(pnp::default_printable_c<std::bitset<8>>);
static_assert(pnp::default_parseable_c<std::bitset<8>>);
static_assert(pnp::default_parseable_c<std::bitset<0>>);
static_assert(pnp::support::sequence_fixed_count_v<std::bitset<8>> == 8);
static_assert(not pnp::printer_c<bitset8_pnp, std::bitset<16>>);
static_assert(not pnp::printer_c<bitset8_pnp, unsigned long>);