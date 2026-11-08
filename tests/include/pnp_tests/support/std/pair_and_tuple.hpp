#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <charconv>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace pnp_tests
{
using int_double_pair     = std::pair<int, double>;
using int_double_pair_pnp = pnp::default_printer<int_double_pair>;

// Prints with the default pnp, checks the text, parses it back, and checks the
// value.
template <typename T>
void
    check_tuple_round_trip(
        const T&           value,
        std::u8string_view expected
    )
{
    const auto text = pnp::to_u8string(value);
    CHECK(text == expected);
    const auto parsed = pnp::try_parse<T>(text);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value);
}

// The same, with a given pnp.
template <typename P, typename T>
void
    check_tuple_round_trip_with(
        const P&           pnp_obj,
        const T&           value,
        std::u8string_view expected
    )
{
    const auto text = pnp::to_u8string(pnp_obj, value);
    CHECK(text == expected);
    const auto parsed = pnp::try_parse<T>(text, pnp_obj);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value);
}

inline pnp::default_printer<bool>
    tuples_test_digit_bits()
{
    return pnp::default_printer<bool>(
        {.true_value = u8"1", .false_value = u8"0"}
    );
}

using tuples_test_hex_byte = pnp::support::
    fixed_width_pnp<std::uint8_t, pnp::support::fixed_width_pnp_unsigned_hex>;

inline tuples_test_hex_byte
    tuples_test_hex_bytes()
{
    return tuples_test_hex_byte(
        {.prefix = u8"", .upper = true, .fixed_size_output = true}
    );
}

// A map entry: no delimiters, a colon between key and value.
struct map_entry_policy : pnp::support::tuple_pnp_default_base
{
    static constexpr std::u8string_view open_delimiter  = u8"";
    static constexpr std::u8string_view close_delimiter = u8"";
    static constexpr std::u8string_view separator       = u8":";
};

// A policy with no separator: valid as a policy, since whether it's allowed
// depends on the parts, which are checked when options are built.
struct empty_separator_policy : pnp::support::tuple_pnp_default_base
{
    static constexpr std::u8string_view open_delimiter  = u8"(";
    static constexpr std::u8string_view close_delimiter = u8")";
    static constexpr std::u8string_view separator       = u8"";
};

// Rejected policies.
struct spaced_separator_policy : pnp::support::tuple_pnp_default_base
{
    static constexpr std::u8string_view open_delimiter  = u8"(";
    static constexpr std::u8string_view close_delimiter = u8")";
    static constexpr std::u8string_view separator       = u8", ";
};

struct bad_spacing_policy : pnp::support::tuple_pnp_default
{
    static constexpr std::u8string_view separator_spacing = u8" x";
};
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "tuples: pairs and tuples round-trip",
    "[tuples]"
)
{
    check_tuple_round_trip(int_double_pair{1, 2.5}, u8"(1,2.5)");
    check_tuple_round_trip(
        std::tuple<int, std::string, bool>{1, "a", true}, u8"(1,\"a\",true)"
    );

    // One element, and none.
    check_tuple_round_trip(std::tuple<int>{42}, u8"(42)");
    check_tuple_round_trip(std::tuple<>{}, u8"()");

    // Nested tuples and containers as elements.
    check_tuple_round_trip(
        std::tuple<std::pair<int, int>, std::vector<int>>{
            {1, 2},
            {3, 4}
    },
        u8"((1,2),[3,4])"
    );

    // Elements containing the tuple's own punctuation.
    check_tuple_round_trip(
        std::tuple<std::string, std::string>{"a,b", "(c)"},
        u8"(\"a,b\",\"(c)\")"
    );
}

TEST_CASE(
    "in pieces: tuples, pairs and map entries",
    "[io][pieces][tuples]"
)
{
    check_round_trip_in_pieces(std::pair<int, std::u8string>{5, u8"five"});
    check_round_trip_in_pieces(
        std::tuple<int, double, std::u8string>{1, 2.5, u8"three"}
    );
    check_round_trip_in_pieces(std::map<int, std::u8string>{
        {1, u8"a"},
        {2, u8"b"}
    });
    check_round_trip_in_pieces(std::complex<double>(1.5, -2));

    check_parses_in_pieces<std::pair<int, int>>(u8"(1,");
    check_parses_in_pieces<std::pair<int, int>>(u8"(1;2)");
}

TEST_CASE(
    "tuples: tokens and spacing",
    "[tuples]"
)
{
    const int_double_pair value{1, 2.5};

    check_tuple_round_trip_with(
        int_double_pair_pnp({.separator_spacing = u8" "}), value, u8"(1, 2.5)"
    );
    check_tuple_round_trip_with(
        int_double_pair_pnp(
            {.separator_spacing = u8" ", .delimiter_spacing = u8" "}
        ),
        value,
        u8"( 1, 2.5 )"
    );
    check_tuple_round_trip_with(
        int_double_pair_pnp(
            {.open_delimiter  = u8"<",
             .separator       = u8";",
             .close_delimiter = u8">"}
        ),
        value,
        u8"<1;2.5>"
    );

    // An empty tuple has no spacing, even when spacing is configured.
    using empty_pnp = pnp::default_printer<std::tuple<>>;
    check_tuple_round_trip_with(
        empty_pnp({.delimiter_spacing = u8" "}), std::tuple<>{}, u8"()"
    );

    // Empty delimiters: a map entry.
    using entry_pnp = pnp::support::generic_tuple_pnp<
        std::pair<int, std::string>,
        std::
            tuple<pnp::default_printer<int>, pnp::default_printer<std::string>>,
        map_entry_policy>;
    check_tuple_round_trip_with(
        entry_pnp{}, std::pair<int, std::string>{1, "a"}, u8"1:\"a\""
    );

    // The options read back, with unnamed fields at their policy values.
    const int_double_pair_pnp spaced({.separator_spacing = u8" "});
    CHECK(spaced.options().separator_spacing == u8" ");
    CHECK(spaced.options().open_delimiter == u8"(");
}

TEST_CASE(
    "tuples: lenient and strict whitespace",
    "[tuples]"
)
{
    // Lenient: any whitespace between the pieces.
    const int_double_pair_pnp lenient({.skip_whitespace = true});
    for (const std::u8string_view input :
         {u8"(1,2.5)", u8"( 1 , 2.5 )", u8"(1 ,\t2.5)", u8"(\n1,\n2.5\n)"})
    {
        INFO(std::string(input.begin(), input.end()));
        const auto parsed = pnp::try_parse<int_double_pair>(input, lenient);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == int_double_pair{1, 2.5});
    }

    // Strict: exactly the configured spacing.
    const int_double_pair_pnp strict({.separator_spacing = u8" "});
    const auto compact = pnp::try_parse<int_double_pair>(u8"(1,2.5)", strict);
    CHECK(not succeeded(compact));
    CHECK(
        first_message(compact)
        == u8"Expected the spacing \" \" after the separator."
    );

    // The default is strict with no spacing, so spaces are rejected.
    CHECK(not succeeded(pnp::try_parse<int_double_pair>(u8"(1, 2.5)")));
}

TEST_CASE(
    "tuples: configuring element parts",
    "[tuples]"
)
{
    using pair_pnp = pnp::support::generic_tuple_pnp<
        int_double_pair,
        std::tuple<
            pnp::default_printer<int>,
            pnp::support::generic_float_pnp<double>>>;
    const int_double_pair value{1, 2.5};

    // All the parts, when constructing.
    const pair_pnp hex({
        .elements
        = {pair_pnp::part_type<0>{},
           pair_pnp::part_type<1>({.format = std::chars_format::hex})}
    });
    check_tuple_round_trip_with(hex, value, u8"(1,0x1.4p+1)");

    // One part of an existing configuration, by copying its options.
    auto opts = pair_pnp{}.options();
    std::get<1>(opts.elements)
        = pair_pnp::part_type<1>({.format = std::chars_format::scientific});
    const pair_pnp scientific(std::move(opts));
    check_tuple_round_trip_with(scientific, value, u8"(1,2.5e+00)");

    // The parts read back.
    CHECK(
        std::get<1>(hex.options().elements).options().format
        == std::chars_format::hex
    );

    // The defaults inherit the options constructor.
    const int_double_pair_pnp default_spaced({.separator_spacing = u8" "});
    CHECK(pnp::to_u8string(default_spaced, value) == u8"(1, 2.5)");
}

TEST_CASE(
    "tuples: parse errors",
    "[tuples]"
)
{
    // A missing opening delimiter.
    const auto no_open = pnp::try_parse<int_double_pair>(u8"1,2.5)");
    CHECK(not succeeded(no_open));
    CHECK(first_message(no_open) == u8"Expected opening delimiter \"(\".");

    // Too few elements: a closing delimiter where a separator was expected.
    const auto too_few = pnp::try_parse<int_double_pair>(u8"(1)");
    CHECK(not succeeded(too_few));
    CHECK(
        first_message(too_few)
        == u8"Expected separator \",\" after element 0 of a tuple."
    );

    // Too many elements: a separator where the closing delimiter was expected.
    const auto too_many = pnp::try_parse<int_double_pair>(u8"(1,2.5,3)");
    CHECK(not succeeded(too_many));
    CHECK(
        first_message(too_many)
        == u8"Expected closing delimiter \")\" after the last element of a "
           u8"tuple."
    );

    // A failing element keeps its own message.
    const auto bad_element = pnp::try_parse<int_double_pair>(u8"(x,2.5)");
    CHECK(not succeeded(bad_element));
    CHECK(
        first_message(bad_element)
        == u8"Expected a valid integer, but no valid digits were found."
    );

    // An empty tuple with something inside.
    CHECK(not succeeded(pnp::try_parse<std::tuple<>>(u8"(1)")));

    // Trailing input, and empty input.
    CHECK(not succeeded(pnp::try_parse<int_double_pair>(u8"(1,2.5)x")));
    CHECK(not succeeded(pnp::try_parse<int_double_pair>(u8"")));
}

TEST_CASE(
    "tuples: invalid options throw on construction",
    "[tuples]"
)
{
    CHECK_THROWS_AS(
        int_double_pair_pnp({.separator = u8""}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        int_double_pair_pnp({.separator = u8", "}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        int_double_pair_pnp({.open_delimiter = u8" ("}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        int_double_pair_pnp({.open_delimiter = half_a_character}),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        int_double_pair_pnp({.separator_spacing = u8" x"}),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        int_double_pair_pnp({.delimiter_spacing = u8","}), std::invalid_argument
    );

    // Empty delimiters are allowed.
    CHECK_NOTHROW(
        int_double_pair_pnp({.open_delimiter = u8"", .close_delimiter = u8""})
    );

    CHECK(
        invalid_argument_message(
            []
            {
                int_double_pair_pnp p({.separator = u8""});
            }
        )
        == "tuple_pnp_options separator may only be empty when every part "
           "prints with the same width"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                int_double_pair_pnp p({.separator_spacing = u8" x"});
            }
        )
        == "tuple_pnp_options separator spacing must contain only whitespace"
    );
}

TEST_CASE(
    "tuples: map-style pairs with a const key print but don't parse",
    "[tuples]"
)
{
    // A map's elements are std::pair<const K, V>.
    const std::pair<const int, double> entry{1, 2.5};
    CHECK(pnp::to_u8string(entry) == u8"(1,2.5)");
}

TEST_CASE(
    "tuples: fixed-width sequences inside flat tuples",
    "[tuples][fixed_width]"
)
{
    // A tuple of a bit and a flat array of bits: 1|0110 without the bar, 10110.
    using bits_row = std::array<bool, 4>;
    using row_pnp  = pnp::support::
        generic_sequence_pnp<bits_row, pnp::default_printer<bool>>;
    using mixed     = std::tuple<bool, bits_row>;
    using mixed_pnp = pnp::support::generic_tuple_pnp<
        mixed,
        std::tuple<pnp::default_printer<bool>, row_pnp>>;

    const row_pnp flat_row(
        {.open_delimiter  = u8"",
         .element         = tuples_test_digit_bits(),
         .separator       = u8"",
         .close_delimiter = u8""}
    );
    const mixed_pnp flat_mixed({
        .open_delimiter  = u8"",
        .elements        = {tuples_test_digit_bits(), flat_row},
        .separator       = u8"",
        .close_delimiter = u8"",
    });

    const mixed     value{
        true, bits_row{false, true, true, false}
    };
    check_tuple_round_trip_with(flat_mixed, value, u8"10110");
    CHECK(flat_mixed.fixed_output_width() == 5);

    // Three parts, of three different widths: a bit, a byte and a flat row,
    // 1AF0110.
    using triple     = std::tuple<bool, std::uint8_t, bits_row>;
    using triple_pnp = pnp::support::generic_tuple_pnp<
        triple,
        std::tuple<pnp::default_printer<bool>, tuples_test_hex_byte, row_pnp>>;
    const triple_pnp flat_triple({
        .open_delimiter = u8"",
        .elements
        = {tuples_test_digit_bits(), tuples_test_hex_bytes(), flat_row},
        .separator       = u8"",
        .close_delimiter = u8"",
    });
    check_tuple_round_trip_with(
        flat_triple,
        triple{
            true, std::uint8_t{0xAF},
             bits_row{false, true, true, false}
    },
        u8"1AF0110"
    );
    CHECK(flat_triple.fixed_output_width() == 7);
}

TEST_CASE(
    "tuples: parsing flat tuples",
    "[tuples][fixed_width]"
)
{
    using bits_pair     = std::pair<bool, bool>;
    using bits_pair_pnp = pnp::default_printer<bits_pair>;
    const bits_pair_pnp flat({
        .open_delimiter  = u8"",
        .elements        = {tuples_test_digit_bits(), tuples_test_digit_bits()},
        .separator       = u8"",
        .close_delimiter = u8"",
    });

    // Too short, too long, and not a bit.
    CHECK(not succeeded(pnp::try_parse<bits_pair>(u8"1", flat)));
    CHECK(not succeeded(pnp::try_parse<bits_pair>(u8"101", flat)));
    CHECK(not succeeded(pnp::try_parse<bits_pair>(u8"12", flat)));

    // Lenient whitespace between flat parts.
    const bits_pair_pnp lenient({
        .open_delimiter  = u8"",
        .elements        = {tuples_test_digit_bits(), tuples_test_digit_bits()},
        .separator       = u8"",
        .close_delimiter = u8"",
        .skip_whitespace = true,
    });
    const auto          parsed = pnp::try_parse<bits_pair>(u8"1 \t0", lenient);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == bits_pair{true, false});
}

TEST_CASE(
    "tuples: every part must be fixed width to drop the separator",
    "[tuples][fixed_width]"
)
{
    // A bit and an int: one part fixed, one not.
    using mixed     = std::pair<bool, int>;
    using mixed_pnp = pnp::default_printer<mixed>;
    CHECK(
        invalid_argument_message(
            []
            {
                mixed_pnp p({
                    .elements
                    = {tuples_test_digit_bits(), pnp::default_printer<int>{}},
                    .separator = u8""
                });
            }
        )
        == "tuple_pnp_options separator may only be empty when every part "
           "prints with the same width"
    );

    // With a separator, it's fine, but the tuple isn't fixed width.
    const mixed_pnp separated({
        .elements = {tuples_test_digit_bits(), pnp::default_printer<int>{}}
    });
    CHECK(not separated.fixed_output_width().has_value());
}

TEST_CASE(
    "tuples: an empty tuple with no delimiters prints nothing",
    "[tuples][fixed_width]"
)
{
    using empty_pnp = pnp::default_printer<std::tuple<>>;
    const empty_pnp nothing(
        {.open_delimiter = u8"", .separator = u8"", .close_delimiter = u8""}
    );
    check_tuple_round_trip_with(nothing, std::tuple<>{}, u8"");
    CHECK(nothing.fixed_output_width() == 0);
}

// --- Policies and options ----------------------------------------------------
static_assert(pnp::support::valid_tuple_pnp_c<pnp::support::tuple_pnp_default>);
static_assert(
    pnp::support::valid_tuple_pnp_c<map_entry_policy>,
    "delimiters may be empty"
);
static_assert(
    pnp::support::valid_tuple_pnp_c<empty_separator_policy>,
    "whether it's allowed depends on the parts"
);
static_assert(not pnp::support::valid_tuple_pnp_c<spaced_separator_policy>);
static_assert(not pnp::support::valid_tuple_pnp_c<bad_spacing_policy>);
static_assert(pnp::support::checkable_options_c<
              int_double_pair_pnp::options_type>);

// --- Parts
// ---------------------------------------------------------------------
using two_int_parts
    = std::tuple<pnp::default_printer<int>, pnp::default_printer<int>>;
static_assert(pnp::support::tuple_parts_c<std::pair<int, int>, two_int_parts>);
static_assert(
    not pnp::support::tuple_parts_c<std::tuple<int, int, int>, two_int_parts>,
    "wrong number of parts"
);
static_assert(
    not pnp::support::tuple_parts_c<std::pair<int, int>, int>,
    "parts must be a std::tuple"
);
static_assert(
    not pnp::support::
        tuple_parts_print_c<std::pair<int, double>, two_int_parts>,
    "an int part can't print a double"
);

// --- Defaults
// ------------------------------------------------------------------
static_assert(pnp::default_printable_c<std::pair<int, double>>);
static_assert(pnp::default_parseable_c<std::pair<int, double>>);
static_assert(pnp::default_printable_c<std::tuple<>>);
static_assert(pnp::default_parseable_c<std::tuple<int, std::string, bool>>);
static_assert(pnp::default_printable_c<std::pair<const int, double>>);
static_assert(
    not pnp::default_parseable_c<std::pair<const int, double>>,
    "a const key can't be assigned"
);

// --- The exact-type guard ----------------------------------------------------
static_assert(pnp::printer_c<int_double_pair_pnp, std::pair<int, double>>);
static_assert(not pnp::printer_c<int_double_pair_pnp, std::pair<int, float>>);
static_assert(not pnp::printer_c<int_double_pair_pnp, std::tuple<int, double>>);
static_assert(not pnp::
                  printer_c<int_double_pair_pnp, std::pair<const int, double>>);

namespace pnp_tests
{
} // namespace pnp_tests

TEST_CASE(
    "tuples: fixed-width parts can be written back to back",
    "[tuples][fixed_width]"
)
{
    using bits_pair     = std::pair<bool, bool>;
    using bits_pair_pnp = pnp::default_printer<bits_pair>;

    // No separator: (10).
    const bits_pair_pnp no_separator({
        .elements  = {tuples_test_digit_bits(), tuples_test_digit_bits()},
        .separator = u8""
    });
    check_tuple_round_trip_with(no_separator, bits_pair{true, false}, u8"(10)");

    // Nothing at all: 10.
    const bits_pair_pnp flat({
        .open_delimiter  = u8"",
        .elements        = {tuples_test_digit_bits(), tuples_test_digit_bits()},
        .separator       = u8"",
        .close_delimiter = u8"",
    });
    check_tuple_round_trip_with(flat, bits_pair{false, true}, u8"01");

    // Parts of different fixed widths: a bit and a byte, 1AF.
    using mixed     = std::tuple<bool, std::uint8_t>;
    using mixed_pnp = pnp::support::generic_tuple_pnp<
        mixed,
        std::tuple<pnp::default_printer<bool>, tuples_test_hex_byte>>;
    const mixed_pnp flat_mixed({
        .open_delimiter  = u8"",
        .elements        = {tuples_test_digit_bits(), tuples_test_hex_bytes()},
        .separator       = u8"",
        .close_delimiter = u8"",
    });
    check_tuple_round_trip_with(
        flat_mixed, mixed{true, std::uint8_t{0xAF}}, u8"1AF"
    );

    // A part that isn't fixed width can't go without a separator.
    CHECK_THROWS_AS(
        int_double_pair_pnp({.separator = u8""}), std::invalid_argument
    );
}

TEST_CASE(
    "tuples: fixed output widths",
    "[tuples][fixed_width]"
)
{
    using bits_pair_pnp = pnp::default_printer<std::pair<bool, bool>>;

    // (1,0): two delimiters, two bits, one separator.
    CHECK(
        bits_pair_pnp(
            {
                .elements = {tuples_test_digit_bits(), tuples_test_digit_bits()}
    }
        ).fixed_output_width()
        == 5
    );

    // ( 1, 0 ): spacing counts too.
    CHECK(
        bits_pair_pnp({
                          .elements
                          = {tuples_test_digit_bits(), tuples_test_digit_bits()
                          },
                          .separator_spacing = u8" ",
                          .delimiter_spacing = u8" ",
    })
            .fixed_output_width()
        == 8
    );

    // An empty tuple is just its delimiters.
    CHECK(pnp::default_printer<std::tuple<>>{}.fixed_output_width() == 2);

    // Not fixed: a part that varies, or lenient whitespace.
    CHECK(not int_double_pair_pnp{}.fixed_output_width().has_value());
    CHECK(not bits_pair_pnp({
                                .elements
                                = {tuples_test_digit_bits(),
                                   tuples_test_digit_bits()},
                                .skip_whitespace = true
    })
                  .fixed_output_width()
                  .has_value());

    // Every output really has the reported width.
    const bits_pair_pnp spaced({
        .elements = {tuples_test_digit_bits(), tuples_test_digit_bits()},
        .separator_spacing = u8" "
    });
    CHECK(
        pnp::to_u8string(spaced, std::pair<bool, bool>{true, true}).size()
        == spaced.fixed_output_width().value()
    );
}

TEST_CASE(
    "tuples: fixed-width tuples inside flat sequences",
    "[tuples][fixed_width]"
)
{
    // An array of flat pairs is itself flat: 1001.
    using bits_pair     = std::pair<bool, bool>;
    using pairs_array   = std::array<bits_pair, 2>;
    using pairs_pnp     = pnp::default_printer<pairs_array>;
    using bits_pair_pnp = pairs_pnp::element_part_type;

    const bits_pair_pnp flat_pair({
        .open_delimiter  = u8"",
        .elements        = {tuples_test_digit_bits(), tuples_test_digit_bits()},
        .separator       = u8"",
        .close_delimiter = u8"",
    });
    const pairs_pnp     flat_pairs(
        {.open_delimiter  = u8"",
             .element         = flat_pair,
             .separator       = u8"",
             .close_delimiter = u8""}
    );

    const pairs_array value{
        bits_pair{true,  false},
        bits_pair{false, true }
    };
    CHECK(pnp::to_u8string(flat_pairs, value) == u8"1001");
    CHECK(flat_pairs.fixed_output_width() == 4);
    const auto parsed = pnp::try_parse<pairs_array>(u8"1001", flat_pairs);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value);
}