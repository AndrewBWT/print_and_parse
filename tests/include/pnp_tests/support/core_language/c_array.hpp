#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

namespace pnp_tests
{
// Element-by-element equality, since built-in arrays can't be compared with ==.
template <typename E, std::size_t N>
bool
    arrays_equal(
        const E (&left_arg)[N],
        const E (&right_arg)[N]
    )
{
    for (std::size_t idx = 0; idx < N; ++idx)
    {
        if constexpr (std::is_array_v<E>)
        {
            if (not arrays_equal(left_arg[idx], right_arg[idx]))
            {
                return false;
            }
        }
        else if (not (left_arg[idx] == right_arg[idx]))
        {
            return false;
        }
    }
    return true;
}

template <typename E, std::size_t N>
void
    check_array_round_trip(
        const E (&value_arg)[N],
        std::u8string_view expected = {}
    )
{
    const auto text = pnp::to_u8string(value_arg);
    if (not expected.empty())
    {
        CHECK(text == expected);
    }
    const auto parsed = pnp::try_parse<E[N]>(text);
    INFO(unicode_bridge::cast_ascii<char>(text));
    if (not (succeeded(parsed)))
    {
        int x = 4;
    }
    REQUIRE(succeeded(parsed));
    CHECK(arrays_equal(parsed.value, value_arg));
}

// The same, with a given pnp. The text is checked only when an expectation is
// given.
template <typename P, typename E, std::size_t N>
requires (not std::is_array_v<P>)
void
    check_array_round_trip_with(
        const P& pnp_obj,
        const E (&value_arg)[N],
        std::u8string_view expected = {}
    )
{
    const auto text = pnp::to_u8string(pnp_obj, value_arg);
    if (not expected.empty())
    {
        CHECK(text == expected);
    }
    const auto parsed = pnp::try_parse<E[N]>(text, pnp_obj);
    REQUIRE(succeeded(parsed));
    CHECK(arrays_equal(parsed.value, value_arg));
}

using char8_array_pnp = pnp::support::generic_c_string_array_pnp<char[8]>;

// Bits as 1 and 0, for a default printer or parser: one code unit each, so
// fixed width.
template <typename Bool_Pnp = pnp::default_printer<bool>>
Bool_Pnp
    c_array_test_digit_bits()
{
    return Bool_Pnp({.true_value = u8"1", .false_value = u8"0"});
}

// Bytes as two upper-case hex digits with no prefix: fixed width 2.
using c_array_test_hex_byte = pnp::support::
    fixed_width_pnp<std::uint8_t, pnp::support::fixed_width_pnp_unsigned_hex>;

inline c_array_test_hex_byte
    c_array_test_hex_bytes()
{
    return c_array_test_hex_byte(
        {.prefix = u8"", .upper = true, .fixed_size_output = true}
    );
}
} // namespace pnp_tests

using namespace pnp_tests;

// ============================================================ arrays as
// sequences

TEST_CASE(
    "c arrays: round trips as sequences",
    "[arrays]"
)
{
    const int ints[3] = {1, 2, 3};
    check_array_round_trip(ints, u8"[1,2,3]");

    const int single[1] = {42};
    check_array_round_trip(single, u8"[42]");

    const double doubles[2] = {1.5, -0.25};
    check_array_round_trip(doubles, u8"[1.5,-0.25]");

    const bool flags[3] = {true, false, true};
    check_array_round_trip(flags, u8"[true,false,true]");

    // Elements that are themselves strings.
    const std::string words[2] = {"a", "b,c"};
    check_array_round_trip(words, u8"[\"a\",\"b,c\"]");
}

TEST_CASE(
    "c arrays: byte arrays are sequences of numbers, not text",
    "[arrays]"
)
{
    const unsigned char unsigned_bytes[3] = {1, 2, 255};
    check_array_round_trip(unsigned_bytes, u8"[1,2,255]");

    const signed char signed_bytes[3] = {-1, 65, 0};
    check_array_round_trip(signed_bytes, u8"[-1,65,0]");
}

TEST_CASE(
    "c arrays: multi-dimensional arrays nest",
    "[arrays]"
)
{
    const int grid[2][3] = {
        {1, 2, 3},
        {4, 5, 6}
    };
    check_array_round_trip(grid, u8"[[1,2,3],[4,5,6]]");

    // Each row of a two-dimensional character array is a string.
    const char rows[2][4] = {"ab", "cde"};
    check_array_round_trip(rows, u8"[\"ab\",\"cde\"]");
}

TEST_CASE(
    "c arrays: the element count must match exactly",
    "[arrays]"
)
{
    // Too few elements, including none at all.
    const auto too_few = pnp::try_parse<int[3]>(u8"[1,2]");
    CHECK(not succeeded(too_few));
    CHECK(first_message(too_few) == u8"Expected 3 elements, but found 2.");

    const auto none = pnp::try_parse<int[3]>(u8"[]");
    CHECK(not succeeded(none));
    CHECK(first_message(none) == u8"Expected 3 elements, but found 0.");

    // Too many elements: reported at the element that didn't fit.
    const auto too_many = pnp::try_parse<int[3]>(u8"[1,2,3,4]");
    CHECK(not succeeded(too_many));
    CHECK(
        first_message(too_many)
        == u8"Could not add element 3 to the sequence: Too many elements; "
           u8"expected exactly 3."
    );

    // A bad element reports the element's own error.
    CHECK(not succeeded(pnp::try_parse<int[3]>(u8"[1,x,3]")));

    // A row of the wrong length in a two-dimensional array.
    CHECK(not succeeded(pnp::try_parse<int[2][3]>(u8"[[1,2,3],[4,5]]")));
}

TEST_CASE(
    "c arrays: configuring the sequence and its elements",
    "[arrays]"
)
{
    using int_array_pnp
        = pnp::support::generic_c_array_pnp<int[3], pnp::default_printer<int>>;
    const int ints[3] = {1, 2, 3};

    // Sequence options: tokens and spacing.
    check_array_round_trip_with(
        int_array_pnp({.separator_spacing = u8" "}), ints, u8"[1, 2, 3]"
    );
    check_array_round_trip_with(
        int_array_pnp(
            {.open_delimiter  = u8"{",
             .separator       = u8";",
             .close_delimiter = u8"}"}
        ),
        ints,
        u8"{1;2;3}"
    );

    // Lenient parsing.
    const int_array_pnp lenient({.skip_whitespace = true});
    const auto spaced = pnp::try_parse<int[3]>(u8"[ 1 ,2,   3 ]", lenient);
    REQUIRE(succeeded(spaced));
    CHECK(arrays_equal(spaced.value, ints));

    // A configured element part: floats in hex.
    using hex_float = pnp::support::generic_float_pnp<double>;
    using hex_array_pnp
        = pnp::support::generic_c_array_pnp<double[2], hex_float>;
    const double values[2] = {3.0, 1.0};
    check_array_round_trip_with(
        hex_array_pnp({.element = hex_float({.format = std::chars_format::hex})}
        ),
        values,
        u8"[0x1.8p+1,0x1p+0]"
    );

    // The defaults inherit the options constructor.
    const pnp::default_printer<int[3]> default_spaced(
        {.separator_spacing = u8" "}
    );
    CHECK(pnp::to_u8string(default_spaced, ints) == u8"[1, 2, 3]");
}

// ============================================================ character arrays
// as strings

TEST_CASE(
    "char arrays: print as quoted strings up to the first null",
    "[arrays][char_arrays]"
)
{
    const char text[8] = "abc";
    CHECK(pnp::to_u8string(text) == u8"\"abc\"");

    // Every opted-in character type.
    const wchar_t  wide[4] = L"ab";
    const char8_t  u8s[4]  = u8"ab";
    const char16_t u16s[4] = u"ab";
    const char32_t u32s[4] = U"ab";
    CHECK(pnp::to_u8string(wide) == u8"\"ab\"");
    CHECK(pnp::to_u8string(u8s) == u8"\"ab\"");
    CHECK(pnp::to_u8string(u16s) == u8"\"ab\"");
    CHECK(pnp::to_u8string(u32s) == u8"\"ab\"");

    // An empty string.
    const char empty[4] = "";
    CHECK(pnp::to_u8string(empty) == u8"\"\"");

    // Content after the first null isn't printed.
    const char embedded[8] = {'a', 'b', '\0', 'c', 'd', '\0', '\0', '\0'};
    CHECK(pnp::to_u8string(embedded) == u8"\"ab\"");

    // An array with no terminator is read safely, up to its size.
    const char unterminated[3] = {'x', 'y', 'z'};
    CHECK(pnp::to_u8string(unterminated) == u8"\"xyz\"");
}

TEST_CASE(
    "char arrays: parsing fills the rest with nulls",
    "[arrays][char_arrays]"
)
{
    const auto parsed = pnp::try_parse<char[5]>(u8"\"ab\"");
    REQUIRE(succeeded(parsed));
    const char expected[5] = {'a', 'b', '\0', '\0', '\0'};
    CHECK(arrays_equal(parsed.value, expected));

    // Round trips through the printer.
    const char text[8] = "hello";
    check_array_round_trip(text, u8"\"hello\"");

    // Escapes and non-ASCII text.
    const char8_t non_ascii[16] = u8"héllo";
    check_array_round_trip(non_ascii, u8"\"héllo\"");
    const char quoted[16] = "say \"hi\"";
    check_array_round_trip(quoted);
}

TEST_CASE(
    "char arrays: in stop-at-null mode, the string must leave room for the "
    "terminator",
    "[arrays][char_arrays]"
)
{
    // A char[4] holds at most 3 characters and its null.
    CHECK(succeeded(pnp::try_parse<char[4]>(u8"\"abc\"")));

    const auto too_long = pnp::try_parse<char[4]>(u8"\"abcd\"");
    CHECK(not succeeded(too_long));
    CHECK(
        first_message(too_long)
        == u8"The string has 4 characters, but an array of 4 holds at most 3 "
           u8"characters and its null terminator."
    );

    // A string with an embedded null can't become a C string. Its text comes
    // from a full-array printer, so the test doesn't depend on how ub escapes a
    // null.
    const char with_null[4] = {'a', '\0', 'b', '\0'};
    const auto full_text    = pnp::to_u8string(
        pnp::support::generic_c_string_array_pnp<char[4]>(
            {.stop_at_null = false}
        ),
        with_null
    );
    const auto embedded = pnp::try_parse<char[4]>(full_text);
    CHECK(not succeeded(embedded));
    CHECK(
        first_message(embedded)
        == u8"A C string can't contain an embedded null character. To keep "
           u8"nulls, set stop_at_null to false."
    );
}

TEST_CASE(
    "char arrays: full-array mode",
    "[arrays][char_arrays]"
)
{
    using full_pnp = pnp::support::generic_c_string_array_pnp<char[4]>;
    const full_pnp full({.stop_at_null = false});
    CHECK(not full.options().stop_at_null);

    // Every element is printed, nulls included, and round-trips.
    const char with_nulls[4] = {'a', '\0', 'b', '\0'};
    check_array_round_trip_with(full, with_nulls);

    // All N elements are available for text.
    const char filled[4] = {'a', 'b', 'c', 'd'};
    check_array_round_trip_with(full, filled, u8"\"abcd\"");

    const auto too_long = pnp::try_parse<char[4]>(u8"\"abcde\"", full);
    CHECK(not succeeded(too_long));
    CHECK(
        first_message(too_long)
        == u8"The string has 5 characters, but the array holds 4."
    );

    // A short string is padded with nulls.
    const auto short_text = pnp::try_parse<char[4]>(u8"\"a\"", full);
    REQUIRE(succeeded(short_text));
    const char padded[4] = {'a', '\0', '\0', '\0'};
    CHECK(arrays_equal(short_text.value, padded));
}

TEST_CASE(
    "char arrays: configuring the text part",
    "[arrays][char_arrays]"
)
{
    const char            text[8] = "abc";

    const char8_array_pnp angled(
        {.text = char8_array_pnp::text_part_type(
             {.open_delimiter = u8"<'", .close_delimiter = u8"'>"}
         )}
    );
    check_array_round_trip_with(angled, text, u8"<'abc'>");

    // Both options at once.
    const char8_array_pnp both({
        .text = char8_array_pnp::text_part_type(
            {.open_delimiter = u8"«", .close_delimiter = u8"\"»"}
        ),
        .stop_at_null = false,
    });
    CHECK(not both.options().stop_at_null);
    CHECK(both.options().text.options().open_delimiter == u8"«");

    // An invalid text part throws when the part is built.
    CHECK_THROWS_AS(
        char8_array_pnp::text_part_type({.close_delimiter = u8">"}),
        std::invalid_argument
    );

    // The defaults inherit the options constructor.
    const pnp::default_printer<char[8]> default_full({.stop_at_null = false});
    CHECK(not default_full.options().stop_at_null);
}

TEST_CASE(
    "char arrays: parse failures from the text part",
    "[arrays][char_arrays]"
)
{
    CHECK(not succeeded(pnp::try_parse<char[8]>(u8"abc"))); // no quotes
    CHECK(not succeeded(pnp::try_parse<char[8]>(u8"\"abc"))
    );                                                      // no closing quote
    CHECK(not succeeded(pnp::try_parse<char[8]>(u8"\"abc\"x"))
    );                                                      // trailing input
}

TEST_CASE(
    "c arrays: fixed-width elements print flat",
    "[arrays][fixed_width]"
)
{
    using bits_pnp     = pnp::default_printer<bool[4]>;
    const bool bits[4] = {true, false, true, false};

    // Only the separator gone: [1010].
    check_array_round_trip_with(
        pnp::support::generic_c_array_pnp<bool[4], pnp::support::bool_pnp<>>(
            {.element   = c_array_test_digit_bits<pnp::support::bool_pnp<>>(),
             .separator = u8""}
        ),
        bits,
        u8"[1010]"
    );

    // Everything gone: 1010.
    using flat_bits_pnp
        = pnp::support::generic_c_array_pnp<bool[4], pnp::support::bool_pnp<>>;
    const flat_bits_pnp flat(
        {.open_delimiter  = u8"",
         .element         = c_array_test_digit_bits<pnp::support::bool_pnp<>>(),
         .separator       = u8"",
         .close_delimiter = u8""}
    );
    check_array_round_trip_with(flat, bits, u8"1010");

    // Hex bytes, back to back.
    using bytes_pnp = pnp::support::
        generic_c_array_pnp<std::uint8_t[4], c_array_test_hex_byte>;
    const bytes_pnp flat_bytes(
        {.open_delimiter  = u8"",
         .element         = c_array_test_hex_bytes(),
         .separator       = u8"",
         .close_delimiter = u8""}
    );
    const std::uint8_t bytes[4] = {0xAF, 0xD4, 0x63, 0x81};
    check_array_round_trip_with(flat_bytes, bytes, u8"AFD46381");

    // The default printer takes the same options.
    const bits_pnp default_flat(
        {.open_delimiter  = u8"",
         .element         = c_array_test_digit_bits(),
         .separator       = u8"",
         .close_delimiter = u8""}
    );
    CHECK(pnp::to_u8string(default_flat, bits) == u8"1010");
}

TEST_CASE(
    "c arrays: multi-dimensional arrays can be flat at every level",
    "[arrays][fixed_width]"
)
{
    using grid_pnp = pnp::support::generic_c_array_pnp<
        bool[2][4],
        pnp::support::generic_c_array_pnp<bool[4], pnp::support::bool_pnp<>>>;
    using row_pnp = grid_pnp::element_part_type;

    const row_pnp flat_row(
        {.open_delimiter  = u8"",
         .element         = c_array_test_digit_bits<pnp::support::bool_pnp<>>(),
         .separator       = u8"",
         .close_delimiter = u8""}
    );
    const bool grid[2][4] = {
        {true,  false, true, false},
        {false, true,  true, false}
    };

    // Flat rows inside bracketed rows: [1010,0110].
    check_array_round_trip_with(
        grid_pnp({.element = flat_row}), grid, u8"[1010,0110]"
    );

    // Flat throughout: 10100110.
    const grid_pnp flat_grid(
        {.open_delimiter  = u8"",
         .element         = flat_row,
         .separator       = u8"",
         .close_delimiter = u8""}
    );
    check_array_round_trip_with(flat_grid, grid, u8"10100110");
    CHECK(flat_grid.fixed_output_width() == 8);
}

TEST_CASE(
    "c arrays: parsing flat forms",
    "[arrays][fixed_width]"
)
{
    using flat_bits_pnp
        = pnp::support::generic_c_array_pnp<bool[4], pnp::support::bool_pnp<>>;
    const flat_bits_pnp flat(
        {.open_delimiter  = u8"",
         .element         = c_array_test_digit_bits<pnp::support::bool_pnp<>>(),
         .separator       = u8"",
         .close_delimiter = u8""}
    );

    // Too few, too many, and not a bit.
    CHECK(not succeeded(pnp::try_parse<bool[4]>(u8"101", flat)));
    CHECK(not succeeded(pnp::try_parse<bool[4]>(u8"10101", flat)));
    CHECK(not succeeded(pnp::try_parse<bool[4]>(u8"1021", flat)));

    // Lenient: any whitespace between the elements.
    const flat_bits_pnp lenient(
        {.open_delimiter  = u8"",
         .element         = c_array_test_digit_bits<pnp::support::bool_pnp<>>(),
         .separator       = u8"",
         .close_delimiter = u8"",
         .skip_whitespace = true}
    );
    const auto parsed = pnp::try_parse<bool[4]>(u8"1 0\t1  0", lenient);
    REQUIRE(succeeded(parsed));
    const bool expected[4] = {true, false, true, false};
    CHECK(arrays_equal(parsed.value, expected));
}

TEST_CASE(
    "c arrays: when tokens may be empty",
    "[arrays][fixed_width]"
)
{
    using int_array_pnp = pnp::default_printer<int[3]>;

    // Elements that aren't fixed width need a separator, and delimiters.
    CHECK(
        invalid_argument_message(
            []
            {
                int_array_pnp p({.separator = u8""});
            }
        )
        == "sequence_pnp_options separator may only be empty when every "
           "element prints with the same width"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                int_array_pnp p(
                    {.open_delimiter = u8"", .close_delimiter = u8""}
                );
            }
        )
        == "sequence_pnp_options delimiters may only be empty for a fixed "
           "number of elements that all print with "
           "the same width"
    );

    // A two-dimensional character array's rows are strings, which vary, so the
    // outer array needs its separator.
    using rows_pnp = pnp::default_printer<char[2][4]>;
    CHECK_THROWS_AS(rows_pnp({.separator = u8""}), std::invalid_argument);
}

TEST_CASE(
    "c arrays: fixed output widths",
    "[arrays][fixed_width]"
)
{
    using bits_pnp
        = pnp::support::generic_c_array_pnp<bool[4], pnp::support::bool_pnp<>>;

    // [1,0,1,0], [ 1, 0, 1, 0 ], and flat.
    CHECK(
        bits_pnp({.element = c_array_test_digit_bits<pnp::support::bool_pnp<>>()
                 })
            .fixed_output_width()
        == 9
    );
    CHECK(
        bits_pnp({.element
                  = c_array_test_digit_bits<pnp::support::bool_pnp<>>(),
                  .separator_spacing = u8" ",
                  .delimiter_spacing = u8" "})
            .fixed_output_width()
        == 14
    );
    CHECK(
        bits_pnp({.open_delimiter = u8"",
                  .element
                  = c_array_test_digit_bits<pnp::support::bool_pnp<>>(),
                  .separator       = u8"",
                  .close_delimiter = u8""})
            .fixed_output_width()
        == 4
    );

    // [[1,0,1,0],[0,1,1,0]]: each row is 9, so 2 + 9 + 1 + 9.
    using grid_pnp = pnp::support::generic_c_array_pnp<bool[2][4], bits_pnp>;
    CHECK(
        grid_pnp({.element = bits_pnp(
                      {.element
                       = c_array_test_digit_bits<pnp::support::bool_pnp<>>()}
                  )})
            .fixed_output_width()
        == 21
    );

    // Not fixed: elements that vary, lenient whitespace, and character arrays,
    // which are strings rather than sequences.
    CHECK(not pnp::default_printer<int[3]>{}.fixed_output_width().has_value());
    CHECK(not bits_pnp({.element
                        = c_array_test_digit_bits<pnp::support::bool_pnp<>>(),
                        .skip_whitespace = true})
                  .fixed_output_width()
                  .has_value());
    CHECK(not pnp::default_printer<char[8]>{}.fixed_output_width().has_value());

    // Every output really has the reported width.
    const bits_pnp spaced(
        {.element = c_array_test_digit_bits<pnp::support::bool_pnp<>>(),
         .separator_spacing = u8" "}
    );
    const bool all_set[4] = {true, true, true, true};
    CHECK(
        pnp::to_u8string(spaced, all_set).size()
        == spaced.fixed_output_width().value()
    );
}

// ============================================================ compile-time
// checks

// --- Which default each array type gets
// ----------------------------------------
template <typename Array>
inline constexpr bool array_prints_as_string = std::is_base_of_v<
    pnp::support::generic_c_string_array_pnp<Array>,
    pnp::default_printer<Array>>;

template <typename Array>
inline constexpr bool array_prints_as_sequence = std::is_base_of_v<
    pnp::support::generic_c_array_pnp<
        Array,
        pnp::default_printer<std::remove_extent_t<Array>>>,
    pnp::default_printer<Array>>;

static_assert(array_prints_as_string<char[8]>);
static_assert(array_prints_as_string<wchar_t[4]>);
static_assert(array_prints_as_string<char8_t[4]>);
static_assert(array_prints_as_string<char16_t[4]>);
static_assert(array_prints_as_string<char32_t[4]>);

static_assert(array_prints_as_sequence<int[3]>);
static_assert(array_prints_as_sequence<double[2]>);
static_assert(
    array_prints_as_sequence<unsigned char[4]>,
    "a byte buffer, not text"
);
static_assert(
    array_prints_as_sequence<signed char[4]>,
    "a byte buffer, not text"
);
static_assert(array_prints_as_sequence<int[2][3]>);

// Every array has exactly one default.
static_assert(
    not (array_prints_as_sequence<char[8]> || array_prints_as_string<int[3]>)
);

// --- Parsing
// --------------------------------------------------------------------
static_assert(pnp::default_parseable_c<int[3]>);
static_assert(pnp::default_printable_c<char[8]>);
static_assert(
    not pnp::default_printable_c<const char[8]>,
    "defaults are for unqualified types, as for const int"
);
static_assert(
    not pnp::default_parseable_c<const char[8]>,
    "const characters can't be written to"
);

// --- Policies
// -------------------------------------------------------------------
static_assert(pnp::support::valid_c_string_array_pnp_c<
              pnp::support::c_string_array_pnp_default>);

// --- The exact-type guard ----------------------------------------------------
static_assert(pnp::printer_c<pnp::default_printer<int[3]>, int[3]>);
static_assert(not pnp::printer_c<pnp::default_printer<int[3]>, int[4]>);
static_assert(not pnp::printer_c<pnp::default_printer<int[3]>, long[3]>);
static_assert(pnp::printer_c<pnp::default_printer<char[8]>, char[8]>);
static_assert(not pnp::printer_c<pnp::default_printer<char[8]>, char[4]>);
static_assert(not pnp::printer_c<pnp::default_printer<char[8]>, const char*>);
static_assert(not pnp::printer_c<pnp::default_printer<char[8]>, std::string>);

// --- Counts
// ----------------------------------------------------------------------
static_assert(pnp::support::sequence_fixed_count_v<int[3]> == 3);
static_assert(
    pnp::support::sequence_fixed_count_v<int[2][3]> == 2,
    "the outer dimension"
);
static_assert(pnp::support::sequence_fixed_count_v<int[3]> == pnp::support::sequence_fixed_count_v<std::array<int, 3>>);