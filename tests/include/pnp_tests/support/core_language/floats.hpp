#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <charconv>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pnp_tests
{
using general_d
    = pnp::support::generic_float_pnp<double, pnp::support::float_pnp_general>;
using scientific_d = pnp::support::
    generic_float_pnp<double, pnp::support::float_pnp_scientific>;
using fixed_d
    = pnp::support::generic_float_pnp<double, pnp::support::float_pnp_fixed>;
using hex_d
    = pnp::support::generic_float_pnp<double, pnp::support::float_pnp_hex>;
using general_f
    = pnp::support::generic_float_pnp<float, pnp::support::float_pnp_general>;

// Prints with the pnp, checks the text, parses it back and checks the value.
template <typename P, typename T>
void
    check_float_round_trip(
        const P&           pnp_obj,
        const T            value,
        std::u8string_view expected
    )
{
    const auto text = pnp::to_u8string(pnp_obj, value);
    CHECK(text == expected);
    const auto parsed = pnp::try_parse<T, P>(text, pnp_obj);
    REQUIRE(succeeded(parsed));
    if (std::isnan(value))
    {
        CHECK(std::isnan(parsed.value));
    }
    else
    {
        CHECK(parsed.value == value);
        CHECK(
            std::signbit(parsed.value) == std::signbit(value)
        ); // distinguishes -0.0 from 0.0
    }
}

// A custom policy with longer words.
struct word_ids : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view nan_id = u8"nan";
    static constexpr std::u8string_view inf_id = u8"infinity";
    static constexpr std::chars_format  chars_format
        = std::chars_format::general;
};

// A hex policy with a custom prefix.
struct hash_hex : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view hex_prefix   = u8"#";
    static constexpr std::chars_format  chars_format = std::chars_format::hex;
};

// Non-hex policies with prefixes that would be invalid in hex mode: accepted,
// because the prefix is only checked when the format is hex.
struct general_bad_prefix : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view hex_prefix = u8"";
    static constexpr std::chars_format  chars_format
        = std::chars_format::general;
};

struct general_spaced_prefix : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view hex_prefix = u8" 0x";
    static constexpr std::chars_format  chars_format
        = std::chars_format::general;
};

// Rejected policies.
struct empty_id : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view nan_id = u8"";
    static constexpr std::chars_format  chars_format
        = std::chars_format::general;
};

struct digit_id : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view nan_id = u8"0NaN";
    static constexpr std::chars_format  chars_format
        = std::chars_format::general;
};

struct sign_id : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view inf_id = u8"+inf";
    static constexpr std::chars_format  chars_format
        = std::chars_format::general;
};

struct dot_id : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view nan_id = u8".nan";
    static constexpr std::chars_format  chars_format
        = std::chars_format::general;
};

struct leading_space_id : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view nan_id = u8" NaN";
    static constexpr std::chars_format  chars_format
        = std::chars_format::general;
};

struct trailing_space_id : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view inf_id = u8"inf ";
    static constexpr std::chars_format  chars_format
        = std::chars_format::general;
};

struct nan_prefix_of_inf : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view nan_id = u8"in";
    static constexpr std::chars_format  chars_format
        = std::chars_format::general;
};

struct inf_prefix_of_nan : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view inf_id = u8"N";
    static constexpr std::chars_format  chars_format
        = std::chars_format::general;
};

struct unsupported_format : pnp::support::float_pnp_default_base
{
    static constexpr std::chars_format chars_format
        = std::chars_format::scientific | std::chars_format::fixed
          | std::chars_format::hex;
};

struct hex_empty_prefix : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view hex_prefix   = u8"";
    static constexpr std::chars_format  chars_format = std::chars_format::hex;
};

struct hex_sign_prefix : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view hex_prefix   = u8"-x";
    static constexpr std::chars_format  chars_format = std::chars_format::hex;
};

struct hex_prefix_overlaps_id : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view hex_prefix
        = u8"in"; // inf_id starts with it
    static constexpr std::chars_format chars_format = std::chars_format::hex;
};

struct hex_spaced_prefix : pnp::support::float_pnp_default_base
{
    static constexpr std::u8string_view hex_prefix   = u8"0x ";
    static constexpr std::chars_format  chars_format = std::chars_format::hex;
};
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "floats: general mode round trips",
    "[floats]"
)
{
    const general_d p;
    check_float_round_trip(p, 1.5, u8"1.5");
    check_float_round_trip(p, -2.25, u8"-2.25");
    check_float_round_trip(p, 0.0, u8"0");
    check_float_round_trip(p, -0.0, u8"-0");
    check_float_round_trip(
        p, 1e20, u8"1e+20"
    );                                       // general picks the shorter form
    check_float_round_trip(p, 0.1, u8"0.1"); // shortest round-trip digits
    check_float_round_trip(p, std::numeric_limits<double>::infinity(), u8"inf");
    check_float_round_trip(
        p, -std::numeric_limits<double>::infinity(), u8"-inf"
    );
    check_float_round_trip(
        p, std::numeric_limits<double>::quiet_NaN(), u8"NaN"
    );
    check_float_round_trip(
        p,
        std::numeric_limits<double>::max(),
        pnp::to_u8string(p, std::numeric_limits<double>::max())
    );
    check_float_round_trip(
        p,
        std::numeric_limits<double>::denorm_min(),
        pnp::to_u8string(p, std::numeric_limits<double>::denorm_min())
    );

    // General mode accepts both notations.
    const auto plain = pnp::try_parse<double>(u8"1.5", p);
    const auto sci   = pnp::try_parse<double>(u8"1.5e3", p);
    REQUIRE(succeeded(plain));
    REQUIRE(succeeded(sci));
    CHECK(plain.value == 1.5);
    CHECK(sci.value == 1500.0);
}

TEST_CASE(
    "floats: scientific mode requires an exponent",
    "[floats]"
)
{
    const scientific_d p;
    check_float_round_trip(p, 1.5, u8"1.5e+00");
    check_float_round_trip(p, -0.0, u8"-0e+00");

    const auto no_exponent = pnp::try_parse<double>(u8"1.5", p);
    CHECK(not succeeded(no_exponent));
    CHECK(
        first_message(no_exponent)
        == u8"Expected a number in scientific notation, such as \"1.5e+00\". "
           u8"Scientific mode requires an exponent."
    );
}

TEST_CASE(
    "floats: fixed mode never writes an exponent",
    "[floats]"
)
{
    const fixed_d p;
    check_float_round_trip(p, 1.5, u8"1.5");
    check_float_round_trip(p, 1e20, u8"100000000000000000000");

    // Fixed mode stops at the 'e', so "1e5" is not a complete parse.
    CHECK(not succeeded(pnp::try_parse<double>(u8"1e5", p)));
}

TEST_CASE(
    "floats: hex mode",
    "[floats]"
)
{
    const hex_d p;
    check_float_round_trip(p, 3.0, u8"0x1.8p+1");
    check_float_round_trip(
        p, -3.0, u8"-0x1.8p+1"
    ); // the sign comes before the prefix
    check_float_round_trip(p, 1.0, u8"0x1p+0");
    check_float_round_trip(p, 0.5, u8"0x1p-1");
    check_float_round_trip(p, 0.0, u8"0x0p+0");
    check_float_round_trip(p, -0.0, u8"-0x0p+0");
    check_float_round_trip(
        p, std::numeric_limits<double>::infinity(), u8"inf"
    ); // no prefix on ids
    check_float_round_trip(
        p, std::numeric_limits<double>::quiet_NaN(), u8"NaN"
    );

    // Hex is exact: values with no short decimal form round-trip bit for bit.
    check_float_round_trip(p, 0.1, pnp::to_u8string(p, 0.1));
    check_float_round_trip(
        p,
        std::numeric_limits<double>::denorm_min(),
        pnp::to_u8string(p, std::numeric_limits<double>::denorm_min())
    );

    // The exponent is optional, and digits after the prefix may be letters.
    const auto no_exponent = pnp::try_parse<double>(u8"0x1.8", p);
    REQUIRE(succeeded(no_exponent));
    CHECK(no_exponent.value == 1.5);
    const auto letter_digit = pnp::try_parse<double>(u8"0xa", p);
    REQUIRE(succeeded(letter_digit));
    CHECK(letter_digit.value == 10.0);
}

TEST_CASE(
    "floats: hex mode rejects malformed input",
    "[floats]"
)
{
    const hex_d p;

    // Digits without the prefix.
    const auto no_prefix = pnp::try_parse<double>(u8"1.8p+1", p);
    CHECK(not succeeded(no_prefix));
    CHECK(
        first_message(no_prefix)
        == u8"Expected a hexadecimal number starting with \"0x\", such as "
           u8"\"0x1.8p+1\"."
    );

    // Nothing usable after the prefix: a second sign, an id, or nothing at all.
    for (const std::u8string_view input :
         {u8"0x-1.8p+1", u8"-0x-1.8p+1", u8"0x+1", u8"0xinf", u8"0xNaN", u8"0x"
         })
    {
        INFO(std::string(input.begin(), input.end()));
        const auto parsed = pnp::try_parse<double>(input, p);
        CHECK(not succeeded(parsed));
        CHECK(
            first_message(parsed)
            == u8"Expected hexadecimal digits after \"0x\", such as "
               u8"\"0x1.8p+1\"."
        );
    }

    // Decimal input isn't accepted in hex mode.
    CHECK(not succeeded(pnp::try_parse<double>(u8"3.0", p)));
}

TEST_CASE(
    "floats: hex mode with a custom prefix",
    "[floats]"
)
{
    using hash_d = pnp::support::generic_float_pnp<double, hash_hex>;
    const hash_d p;
    check_float_round_trip(p, 3.0, u8"#1.8p+1");
    check_float_round_trip(p, -3.0, u8"-#1.8p+1");

    // The default prefix is no longer recognised, and messages name the custom
    // one.
    const auto old_prefix = pnp::try_parse<double>(u8"0x1.8p+1", p);
    CHECK(not succeeded(old_prefix));
    CHECK(
        first_message(old_prefix)
        == u8"Expected a hexadecimal number starting with \"#\", such as "
           u8"\"#1.8p+1\"."
    );

    // The same, configured through options rather than a policy.
    const hex_d q({.hex_prefix = u8"#"});
    check_float_round_trip(q, 3.0, u8"#1.8p+1");

    // Multi-character, non-ASCII and spaced-inside prefixes all work.
    const hex_d long_prefix({.hex_prefix = u8"hex:"});
    check_float_round_trip(long_prefix, 3.0, u8"hex:1.8p+1");
    const hex_d non_ascii_prefix({.hex_prefix = u8"§"});
    check_float_round_trip(non_ascii_prefix, 3.0, u8"§1.8p+1");
    const hex_d spaced_inside({.hex_prefix = u8"h x"});
    check_float_round_trip(spaced_inside, 3.0, u8"h x1.8p+1");
}

TEST_CASE(
    "floats: signs",
    "[floats]"
)
{
    const general_d p;
    const auto      plus = pnp::try_parse<double>(u8"+1.5", p);
    REQUIRE(succeeded(plus));
    CHECK(plus.value == 1.5);

    const auto plus_inf = pnp::try_parse<double>(u8"+inf", p);
    REQUIRE(succeeded(plus_inf));
    CHECK(plus_inf.value == std::numeric_limits<double>::infinity());

    // The plus option on the printer.
    const general_d with_plus({.use_plus_for_positive_numbers = true});
    check_float_round_trip(with_plus, 1.5, u8"+1.5");
    check_float_round_trip(
        with_plus, std::numeric_limits<double>::infinity(), u8"+inf"
    );
    check_float_round_trip(with_plus, -1.5, u8"-1.5"); // unaffected
    check_float_round_trip(
        with_plus, std::numeric_limits<double>::quiet_NaN(), u8"NaN"
    );                                                 // never signed

    const hex_d hex_with_plus({.use_plus_for_positive_numbers = true});
    check_float_round_trip(hex_with_plus, 3.0, u8"+0x1.8p+1");

    // A second sign is rejected.
    CHECK(not succeeded(pnp::try_parse<double>(u8"+-1.5", p)));
    CHECK(not succeeded(pnp::try_parse<double>(u8"--1.5", p)));
}

TEST_CASE(
    "floats: configuring through options",
    "[floats]"
)
{
    // Only the named fields change; the rest keep their policy values.
    const general_d p({.format = std::chars_format::scientific});
    CHECK(p.options().format == std::chars_format::scientific);
    CHECK(p.options().nan_id == u8"NaN");
    CHECK(p.options().inf_id == u8"inf");
    check_float_round_trip(p, 1.5, u8"1.5e+00");

    // Custom ids through options, including non-ASCII ones and ones with spaces
    // inside.
    const general_d words({.nan_id = u8"nan", .inf_id = u8"infinity"});
    check_float_round_trip(
        words, std::numeric_limits<double>::infinity(), u8"infinity"
    );
    check_float_round_trip(
        words, std::numeric_limits<double>::quiet_NaN(), u8"nan"
    );
    const general_d symbols({.nan_id = u8"∅", .inf_id = u8"∞"});
    check_float_round_trip(
        symbols, std::numeric_limits<double>::infinity(), u8"∞"
    );
    check_float_round_trip(
        symbols, -std::numeric_limits<double>::infinity(), u8"-∞"
    );
    check_float_round_trip(
        symbols, std::numeric_limits<double>::quiet_NaN(), u8"∅"
    );
    const general_d spaced_inside(
        {.nan_id = u8"not a number", .inf_id = u8"infinite value"}
    );
    check_float_round_trip(
        spaced_inside,
        std::numeric_limits<double>::quiet_NaN(),
        u8"not a number"
    );

    // One options value can configure pnps for different floating-point types.
    general_d::options_type shared{.format = std::chars_format::fixed};
    const general_d         d(shared);
    const general_f         f(shared);
    check_float_round_trip(d, 1e20, u8"100000000000000000000");
    check_float_round_trip(f, 1.5f, u8"1.5");

    // A modified copy of an existing configuration.
    auto modified   = p.options();
    modified.format = std::chars_format::general;
    const general_d copy(modified);
    check_float_round_trip(copy, 1e20, u8"1e+20");

    // The defaults inherit the options constructor.
    const pnp::default_printer<double> default_hex(
        {.format = std::chars_format::hex}
    );
    CHECK(pnp::to_u8string(default_hex, 3.0) == u8"0x1.8p+1");
}

TEST_CASE(
    "floats: invalid options throw on construction",
    "[floats]"
)
{
    // Ids that look like the start of a number, or are empty.
    CHECK_THROWS_AS(general_d({.nan_id = u8""}), std::invalid_argument);
    CHECK_THROWS_AS(general_d({.inf_id = u8"1inf"}), std::invalid_argument);
    CHECK_THROWS_AS(general_d({.inf_id = u8".inf"}), std::invalid_argument);
    CHECK_THROWS_AS(general_d({.nan_id = u8"-NaN"}), std::invalid_argument);

    // Ids that are prefixes of each other, or identical.
    CHECK_THROWS_AS(
        (general_d({.nan_id = u8"in", .inf_id = u8"inf"})),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        general_d({.inf_id = u8"N"}), std::invalid_argument
    ); // prefix of "NaN"
    CHECK_THROWS_AS(
        (general_d({.nan_id = u8"x", .inf_id = u8"x"})), std::invalid_argument
    );

    // Unsupported formats.
    CHECK_THROWS_AS(
        general_d({.format = static_cast<std::chars_format>(7)}),
        std::invalid_argument
    );

    // Ids with whitespace at either end.
    CHECK_THROWS_AS(general_d({.nan_id = u8" NaN"}), std::invalid_argument);
    CHECK_THROWS_AS(general_d({.nan_id = u8"NaN "}), std::invalid_argument);
    CHECK_THROWS_AS(general_d({.inf_id = u8"\tinf"}), std::invalid_argument);
    CHECK_THROWS_AS(general_d({.inf_id = u8"inf\n"}), std::invalid_argument);

    // Ids that aren't valid UTF-8.
    CHECK_THROWS_AS(
        general_d({.nan_id = half_a_character}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        general_d({.inf_id = u8"inf" + half_a_character}), std::invalid_argument
    );
}

TEST_CASE(
    "floats: the hex prefix is checked only in hex mode",
    "[floats]"
)
{
    // Invalid prefixes in hex mode.
    CHECK_THROWS_AS(hex_d({.hex_prefix = u8""}), std::invalid_argument);
    CHECK_THROWS_AS(hex_d({.hex_prefix = u8"+x"}), std::invalid_argument);
    CHECK_THROWS_AS(
        hex_d({.hex_prefix = u8"in"}), std::invalid_argument
    ); // inf_id starts with it
    CHECK_THROWS_AS(
        hex_d({.hex_prefix = u8"NaNx"}), std::invalid_argument
    ); // it starts with nan_id
    CHECK_THROWS_AS(hex_d({.hex_prefix = u8" 0x"}), std::invalid_argument);
    CHECK_THROWS_AS(hex_d({.hex_prefix = u8"0x "}), std::invalid_argument);
    CHECK_THROWS_AS(
        hex_d({.hex_prefix = half_a_character}), std::invalid_argument
    );

    // The same prefixes are accepted when the format isn't hex.
    CHECK_NOTHROW(general_d({.hex_prefix = u8""}));
    CHECK_NOTHROW(general_d({.hex_prefix = u8"in"}));
    CHECK_NOTHROW(general_d({.hex_prefix = u8" 0x"}));
    CHECK_NOTHROW(general_d({.hex_prefix = half_a_character}));

    // Switching an accepted configuration to hex re-validates it.
    const general_d lenient({.hex_prefix = u8""});
    auto            switched = lenient.options();
    switched.format          = std::chars_format::hex;
    CHECK_THROWS_AS(general_d(switched), std::invalid_argument);

    // A hex pnp built through options in a non-hex policy.
    CHECK_THROWS_AS(
        (general_d({.format = std::chars_format::hex, .hex_prefix = u8""})),
        std::invalid_argument
    );
    CHECK_NOTHROW(
        general_d({.format = std::chars_format::hex, .hex_prefix = u8"0x"})
    );
}

TEST_CASE(
    "floats: construction errors name the rule that was broken",
    "[floats]"
)
{
    CHECK(
        invalid_argument_message(
            []
            {
                general_d p({.format = static_cast<std::chars_format>(7)});
            }
        )
        == "float_pnp_options format must be scientific, fixed, general or hex"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                general_d p({.nan_id = half_a_character});
            }
        )
        == "float_pnp_options nan id must be valid UTF-8"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                general_d p({.inf_id = u8" inf"});
            }
        )
        == "float_pnp_options inf id must not start or end with whitespace"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                general_d p({.nan_id = u8""});
            }
        )
        == "float_pnp_options nan id must be non-empty, and must not start "
           "with a digit, '.', '+' or '-'"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                general_d p({.inf_id = u8"N"});
            }
        )
        == "float_pnp_options nan and inf ids must not be prefixes of each "
           "other"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                hex_d p({.hex_prefix = u8"0x "});
            }
        )
        == "float_pnp_options hex prefix must not start or end with whitespace"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                hex_d p({.hex_prefix = u8""});
            }
        )
        == "float_pnp_options hex prefix must be non-empty, must not start "
           "with '+' or '-', and must not overlap "
           "either id"
    );

    // The most basic problem is reported first: an id with leading whitespace
    // that also starts with a digit reports the whitespace.
    CHECK(
        invalid_argument_message(
            []
            {
                general_d p({.nan_id = u8" 1"});
            }
        )
        == "float_pnp_options nan id must not start or end with whitespace"
    );
}

TEST_CASE(
    "floats: error messages",
    "[floats]"
)
{
    const general_d p;

    const auto      signed_nan = pnp::try_parse<double>(u8"-NaN", p);
    CHECK(not succeeded(signed_nan));
    CHECK(first_message(signed_nan) == u8"\"NaN\" cannot have a sign.");

    const auto letter = pnp::try_parse<double>(u8"x5", p);
    CHECK(not succeeded(letter));
    CHECK(
        first_message(letter)
        == u8"Expected a number, \"NaN\" or \"inf\", but found \"x\"."
    );

    // A non-ASCII character is quoted whole, not as half a code point.
    const auto non_ascii = pnp::try_parse<double>(u8"é5", p);
    CHECK(not succeeded(non_ascii));
    CHECK(first_message(non_ascii).starts_with(
        u8"Expected a number, \"NaN\" or \"inf\", but found \""
    ));
    CHECK(first_message(non_ascii).ends_with(u8"\"."));

    // Custom ids appear in the messages.
    const general_d words({.nan_id = u8"nan", .inf_id = u8"infinity"});
    const auto      words_letter = pnp::try_parse<double>(u8"x", words);
    CHECK(
        first_message(words_letter)
        == u8"Expected a number, \"nan\" or \"infinity\", but found \"x\"."
    );
    const auto words_signed_nan = pnp::try_parse<double>(u8"+nan", words);
    CHECK(first_message(words_signed_nan) == u8"\"nan\" cannot have a sign.");

    const auto just_sign = pnp::try_parse<double>(u8"-", p);
    CHECK(not succeeded(just_sign));
    CHECK(
        first_message(just_sign) == u8"Expected a number, but the input ended."
    );

    CHECK(not succeeded(pnp::try_parse<double>(u8"", p)));

    const auto too_big = pnp::try_parse<double>(u8"1e400", p);
    CHECK(not succeeded(too_big));
    CHECK(
        first_message(too_big)
        == u8"The value \"1e400\" is out of range for a floating-point type of "
           u8"8 bytes."
    );

    const general_f pf;
    const auto      too_big_float = pnp::try_parse<float>(u8"-1e40", pf);
    CHECK(not succeeded(too_big_float));
    CHECK(
        first_message(too_big_float)
        == u8"The value \"-1e40\" is out of range for a floating-point type of "
           u8"4 bytes."
    );

    // Out of range in hex quotes the prefix too.
    const hex_d hex;
    const auto  too_big_hex = pnp::try_parse<double>(u8"0x1p+2000", hex);
    CHECK(not succeeded(too_big_hex));
    CHECK(
        first_message(too_big_hex)
        == u8"The value \"0x1p+2000\" is out of range for a floating-point "
           u8"type of 8 bytes."
    );

    // A lone '.' has no digits.
    CHECK(not succeeded(pnp::try_parse<double>(u8".", p)));
}

TEST_CASE(
    "floats: custom policies",
    "[floats]"
)
{
    using words_d = pnp::support::generic_float_pnp<double, word_ids>;
    const words_d p;
    check_float_round_trip(
        p, std::numeric_limits<double>::infinity(), u8"infinity"
    );
    check_float_round_trip(
        p, -std::numeric_limits<double>::infinity(), u8"-infinity"
    );
    check_float_round_trip(
        p, std::numeric_limits<double>::quiet_NaN(), u8"nan"
    );

    // The default spellings aren't accepted by this policy.
    CHECK(not succeeded(pnp::try_parse<double>(u8"inf", p)));
    CHECK(not succeeded(pnp::try_parse<double>(u8"NaN", p)));

    // Non-hex policies may carry prefixes that would be invalid in hex mode.
    using lenient_d
        = pnp::support::generic_float_pnp<double, general_bad_prefix>;
    const lenient_d q;
    check_float_round_trip(q, 1.5, u8"1.5");
    using spaced_d
        = pnp::support::generic_float_pnp<double, general_spaced_prefix>;
    const spaced_d r;
    check_float_round_trip(r, 1.5, u8"1.5");
}

TEST_CASE(
    "floats: defaults and containers",
    "[floats]"
)
{
    CHECK(pnp::to_u8string(1.5) == u8"1.5");
    CHECK(pnp::to_u8string(1.5f) == u8"1.5");

    const std::vector<double> values{
        1.5, -0.0, std::numeric_limits<double>::infinity()
    };
    const auto text = pnp::to_u8string(values);
    CHECK(text == u8"[1.5,-0,inf]");
    const auto parsed = pnp::try_parse<std::vector<double>>(text);
    REQUIRE(succeeded(parsed));
    REQUIRE(parsed.value.size() == 3);
    CHECK(parsed.value[0] == 1.5);
    CHECK(std::signbit(parsed.value[1]));
    CHECK(std::isinf(parsed.value[2]));
}

TEST_CASE(
    "read ahead: a float stops at the first character after it",
    "[io][read_ahead][floats]"
)
{
    {
        INFO("a whole number");
        const auto outcome
            = parse_from_pieces<double>({u8"5", u8"\n", u8"6", u8"\n"});
        CHECK(outcome.succeeded);
        CHECK(outcome.value == 5.0);
        CHECK(outcome.pieces_read == 2);
    }
    {
        INFO("with a decimal point");
        const auto outcome = parse_from_pieces<double>(
            {u8"1", u8".", u8"5", u8"\n", u8"7", u8"\n"}
        );
        CHECK(outcome.succeeded);
        CHECK(outcome.value == 1.5);
        CHECK(outcome.pieces_read == 4);
    }
    {
        INFO("with a sign");
        const auto outcome
            = parse_from_pieces<double>({u8"-", u8"2", u8"\n", u8"8", u8"\n"});
        CHECK(outcome.succeeded);
        CHECK(outcome.value == -2.0);
        CHECK(outcome.pieces_read == 3);
    }
    {
        INFO("with an exponent");
        const auto outcome
            = parse_from_pieces<double>({u8"1e", u8"3", u8"\n", u8"9", u8"\n"});
        CHECK(outcome.succeeded);
        CHECK(outcome.value == 1000.0);
        CHECK(outcome.pieces_read == 3);
    }
}

TEST_CASE(
    "read ahead: a float's special values stop at the end of their name",
    "[io][read_ahead][floats]"
)
{
    // The name is matched as soon as it's all there: nothing after it is
    // needed.
    const auto outcome
        = parse_from_pieces<double>({u8"inf", u8"\n", u8"1", u8"\n"});
    CHECK(outcome.succeeded);
    CHECK(outcome.value == std::numeric_limits<double>::infinity());
    CHECK(outcome.pieces_read == 1);
}

TEST_CASE(
    "read ahead: a hex float stops at the first character after it",
    "[io][read_ahead][floats]"
)
{
    using hex_double
        = pnp::support::generic_float_pnp<double, pnp::support::float_pnp_hex>;
    counting_source   source({u8"0x1p+0", u8"\n", u8"0x2p+0", u8"\n"});
    pnp::parser_input input(source);
    double            value = 0;
    CHECK(input.run_parser(hex_double{}, value).succeeded());
    CHECK(value == 1.0);
    CHECK(source.given == 2);
}

TEST_CASE(
    "read ahead: float values typed one per line come back straight away",
    "[io][read_ahead][value_reader]"
)
{
    {
        counting_source           source({u8"2.5", u8"\n", u8"3.5", u8"\n"});
        pnp::value_reader<double> reader(source);
        const auto                first = reader.next();
        REQUIRE(first.has_value());
        CHECK(first->value == 2.5);
        CHECK(source.given == 2);
    }
}

// --- Policy validation at compile time ---------------------------------------
static_assert(pnp::support::valid_float_pnp_c<pnp::support::float_pnp_general>);
static_assert(pnp::support::valid_float_pnp_c<
              pnp::support::float_pnp_scientific>);
static_assert(pnp::support::valid_float_pnp_c<pnp::support::float_pnp_fixed>);
static_assert(pnp::support::valid_float_pnp_c<pnp::support::float_pnp_hex>);
static_assert(pnp::support::valid_float_pnp_c<word_ids>);
static_assert(pnp::support::valid_float_pnp_c<hash_hex>);
static_assert(
    pnp::support::valid_float_pnp_c<general_bad_prefix>,
    "prefix only checked in hex mode"
);
static_assert(
    pnp::support::valid_float_pnp_c<general_spaced_prefix>,
    "prefix only checked in hex mode"
);
static_assert(not pnp::support::valid_float_pnp_c<empty_id>);
static_assert(not pnp::support::valid_float_pnp_c<digit_id>);
static_assert(not pnp::support::valid_float_pnp_c<sign_id>);
static_assert(not pnp::support::valid_float_pnp_c<dot_id>);
static_assert(not pnp::support::valid_float_pnp_c<leading_space_id>);
static_assert(not pnp::support::valid_float_pnp_c<trailing_space_id>);
static_assert(not pnp::support::valid_float_pnp_c<nan_prefix_of_inf>);
static_assert(not pnp::support::valid_float_pnp_c<inf_prefix_of_nan>);
static_assert(not pnp::support::valid_float_pnp_c<unsupported_format>);
static_assert(not pnp::support::valid_float_pnp_c<hex_empty_prefix>);
static_assert(not pnp::support::valid_float_pnp_c<hex_sign_prefix>);
static_assert(not pnp::support::valid_float_pnp_c<hex_prefix_overlaps_id>);
static_assert(not pnp::support::valid_float_pnp_c<hex_spaced_prefix>);

// Validation stays wired up.
static_assert(pnp::support::checkable_options_c<pnp::support::float_pnp_options<
                  pnp::support::float_pnp_general>>);

// Options for the same policy are the same type whatever the floating-point
// type.
static_assert(std::same_as<general_d::options_type, general_f::options_type>);
static_assert(not std::same_as<general_d::options_type, hex_d::options_type>);

// --- The exact-type guard ----------------------------------------------------
static_assert(pnp::printer_c<general_d, double>);
static_assert(not pnp::printer_c<general_d, int>);
static_assert(not pnp::printer_c<general_d, float>);
static_assert(not pnp::printer_c<general_d, long double>);
static_assert(not pnp::printer_c<pnp::default_printer<float>, double>);