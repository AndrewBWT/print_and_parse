#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pnp_tests
{
template <typename T, std::size_t N>
void
    check_std_array_round_trip(
        const std::array<T, N>& value,
        std::u8string_view      expected
    )
{
    const auto text = pnp::to_u8string(value);
    CHECK(text == expected);
    const auto parsed = pnp::try_parse<std::array<T, N>>(text);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value);
}

// Bits as 1 and 0, for a default printer or parser: one code unit each, so
// fixed width.
template <typename Bool_Pnp = pnp::default_printer<bool>>
Bool_Pnp
    array_test_digit_bits()
{
    return Bool_Pnp({.true_value = u8"1", .false_value = u8"0"});
}

// Bytes as two upper-case hex digits with no prefix: fixed width 2.
using array_test_hex_byte = pnp::support::
    fixed_width_pnp<std::uint8_t, pnp::support::fixed_width_pnp_unsigned_hex>;

inline array_test_hex_byte
    array_test_hex_bytes()
{
    return array_test_hex_byte(
        {.prefix = u8"", .upper = true, .fixed_size_output = true}
    );
}
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "std::array: round trips",
    "[std_array]"
)
{
    check_std_array_round_trip(std::array<int, 3>{1, 2, 3}, u8"[1,2,3]");
    check_std_array_round_trip(std::array<int, 1>{42}, u8"[42]");
    check_std_array_round_trip(std::array<int, 0>{}, u8"[]");
    check_std_array_round_trip(
        std::array<std::string, 2>{"a", "b,c"}, u8"[\"a\",\"b,c\"]"
    );
    check_std_array_round_trip(
        std::array<std::array<int, 2>, 2>{
            {{1, 2}, {3, 4}}
    },
        u8"[[1,2],[3,4]]"
    );

    // Characters are elements, not text.
    check_std_array_round_trip(std::array<char, 2>{'a', 'b'}, u8"['a','b']");
}

TEST_CASE(
    "std::array: the element count must match exactly",
    "[std_array]"
)
{
    const auto too_few = pnp::try_parse<std::array<int, 3>>(u8"[1,2]");
    CHECK(not succeeded(too_few));
    CHECK(first_message(too_few) == u8"Expected 3 elements, but found 2.");

    const auto too_many = pnp::try_parse<std::array<int, 3>>(u8"[1,2,3,4]");
    CHECK(not succeeded(too_many));
    CHECK(
        first_message(too_many)
        == u8"Could not add element 3 to the sequence: Too many elements; "
           u8"expected exactly 3."
    );

    CHECK(not succeeded(pnp::try_parse<std::array<int, 0>>(u8"[1]")));
    CHECK(not succeeded(pnp::try_parse<std::array<int, 2>>(u8"[1,x]")));
}

TEST_CASE(
    "std::array: configuring the sequence",
    "[std_array]"
)
{
    using array_pnp = pnp::default_printer<std::array<int, 3>>;
    CHECK(
        pnp::to_u8string(
            array_pnp({.separator_spacing = u8" "}), std::array<int, 3>{1, 2, 3}
        )
        == u8"[1, 2, 3]"
    );

    const pnp::default_parser<std::array<int, 3>> lenient(
        {.skip_whitespace = true}
    );
    const auto parsed
        = pnp::try_parse<std::array<int, 3>>(u8"[ 1 , 2,3 ]", lenient);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == std::array<int, 3>{1, 2, 3});
}

TEST_CASE(
    "std::array: fixed-width elements print flat",
    "[std_array][fixed_width]"
)
{
    using bits_printer = pnp::default_printer<std::array<bool, 4>>;
    using bits_parser  = pnp::default_parser<std::array<bool, 4>>;
    const std::array<bool, 4> bits{true, false, true, false};

    // Only the separator gone: [1010].
    const bits_printer no_separator(
        {.element = array_test_digit_bits(), .separator = u8""}
    );
    CHECK(pnp::to_u8string(no_separator, bits) == u8"[1010]");

    // Everything gone: 1010.
    const bits_printer flat(
        {.open_delimiter  = u8"",
         .element         = array_test_digit_bits(),
         .separator       = u8"",
         .close_delimiter = u8""}
    );
    const bits_parser flat_parser(
        {.open_delimiter  = u8"",
         .element         = array_test_digit_bits<pnp::default_parser<bool>>(),
         .separator       = u8"",
         .close_delimiter = u8""}
    );
    CHECK(pnp::to_u8string(flat, bits) == u8"1010");
    const auto parsed
        = pnp::try_parse<std::array<bool, 4>>(u8"1010", flat_parser);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == bits);

    // Hex bytes, back to back.
    using bytes_pnp = pnp::support::
        generic_sequence_pnp<std::array<std::uint8_t, 4>, array_test_hex_byte>;
    const bytes_pnp flat_bytes(
        {.open_delimiter  = u8"",
         .element         = array_test_hex_bytes(),
         .separator       = u8"",
         .close_delimiter = u8""}
    );
    const std::array<std::uint8_t, 4> bytes{0xAF, 0xD4, 0x63, 0x81};
    CHECK(pnp::to_u8string(flat_bytes, bytes) == u8"AFD46381");
    const auto parsed_bytes
        = pnp::try_parse<std::array<std::uint8_t, 4>>(u8"AFD46381", flat_bytes);
    REQUIRE(succeeded(parsed_bytes));
    CHECK(parsed_bytes.value == bytes);

    // Nested: a flat array of flat arrays, 10100110.
    using row      = std::array<bool, 4>;
    using grid     = std::array<row, 2>;
    using grid_pnp = pnp::default_printer<grid>;
    using row_pnp  = grid_pnp::element_part_type;
    const row_pnp flat_row(
        {.open_delimiter  = u8"",
         .element         = array_test_digit_bits(),
         .separator       = u8"",
         .close_delimiter = u8""}
    );
    const grid_pnp flat_grid(
        {.open_delimiter  = u8"",
         .element         = flat_row,
         .separator       = u8"",
         .close_delimiter = u8""}
    );
    CHECK(
        pnp::to_u8string(
            flat_grid,
            grid{
                {{true, false, true, false}, {false, true, true, false}}
    }
        )
        == u8"10100110"
    );

    // An empty array of fixed-width elements prints as nothing at all.
    using empty_pnp = pnp::default_printer<std::array<bool, 0>>;
    const empty_pnp flat_empty(
        {.open_delimiter  = u8"",
         .element         = array_test_digit_bits(),
         .separator       = u8"",
         .close_delimiter = u8""}
    );
    CHECK(pnp::to_u8string(flat_empty, std::array<bool, 0>{}) == u8"");
    CHECK(flat_empty.fixed_output_width() == 0);
}

TEST_CASE(
    "std::array: parsing flat forms",
    "[std_array][fixed_width]"
)
{
    using bits_parser = pnp::default_parser<std::array<bool, 4>>;
    const bits_parser flat(
        {.open_delimiter  = u8"",
         .element         = array_test_digit_bits<pnp::default_parser<bool>>(),
         .separator       = u8"",
         .close_delimiter = u8""}
    );

    // Too few: the missing element's own error, for the element that's missing.
    CHECK(not succeeded(pnp::try_parse<std::array<bool, 4>>(u8"101", flat)));

    // Too many: the count is reached, so the rest is left over.
    CHECK(not succeeded(pnp::try_parse<std::array<bool, 4>>(u8"10101", flat)));

    // Not a bit.
    CHECK(not succeeded(pnp::try_parse<std::array<bool, 4>>(u8"1021", flat)));

    // Strict spacing between flat elements: 1 0 1 0, exactly.
    const bits_parser spaced(
        {.open_delimiter  = u8"",
         .element         = array_test_digit_bits<pnp::default_parser<bool>>(),
         .separator       = u8"",
         .close_delimiter = u8"",
         .separator_spacing = u8" "}
    );
    const auto spaced_parsed
        = pnp::try_parse<std::array<bool, 4>>(u8"1 0 1 0", spaced);
    REQUIRE(succeeded(spaced_parsed));
    CHECK(spaced_parsed.value == std::array<bool, 4>{true, false, true, false});
    CHECK(not succeeded(pnp::try_parse<std::array<bool, 4>>(u8"1010", spaced)));

    // Lenient: any whitespace between the elements.
    const bits_parser lenient(
        {.open_delimiter  = u8"",
         .element         = array_test_digit_bits<pnp::default_parser<bool>>(),
         .separator       = u8"",
         .close_delimiter = u8"",
         .skip_whitespace = true}
    );
    const auto lenient_parsed
        = pnp::try_parse<std::array<bool, 4>>(
            u8"1 0\t1  0", lenient
        );
    REQUIRE(succeeded(lenient_parsed));
    CHECK(
        lenient_parsed.value == std::array<bool, 4>{true, false, true, false}
    );
}

TEST_CASE(
    "std::array: when tokens may be empty",
    "[std_array][fixed_width]"
)
{
    using int_array_pnp = pnp::default_printer<std::array<int, 3>>;

    // Elements that aren't fixed width need a separator...
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

    // ...and delimiters, even with a fixed count.
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

    // The default bool words differ in length, so they aren't fixed width
    // either.
    using bool_array_pnp = pnp::default_printer<std::array<bool, 4>>;
    CHECK_THROWS_AS(bool_array_pnp({.separator = u8""}), std::invalid_argument);

    // One delimiter can go without the other.
    CHECK_NOTHROW(bool_array_pnp(
        {.open_delimiter = u8"", .element = array_test_digit_bits()}
    ));
}

TEST_CASE(
    "std::array: fixed output widths",
    "[std_array][fixed_width]"
)
{
    using bits_pnp = pnp::default_printer<std::array<bool, 4>>;

    // [1,0,1,0]: two delimiters, four bits, three separators.
    CHECK(
        bits_pnp({.element = array_test_digit_bits()}).fixed_output_width() == 9
    );

    // [ 1, 0, 1, 0 ]: spacing counts.
    CHECK(
        bits_pnp({.element           = array_test_digit_bits(),
                  .separator_spacing = u8" ",
                  .delimiter_spacing = u8" "})
            .fixed_output_width()
        == 14
    );

    // Flat.
    CHECK(
        bits_pnp({.open_delimiter  = u8"",
                  .element         = array_test_digit_bits(),
                  .separator       = u8"",
                  .close_delimiter = u8""})
            .fixed_output_width()
        == 4
    );

    // Padded hex bytes: [AF,D4,63,81].
    using bytes_pnp = pnp::support::
        generic_sequence_pnp<std::array<std::uint8_t, 4>, array_test_hex_byte>;
    CHECK(
        bytes_pnp({.element = array_test_hex_bytes()}).fixed_output_width()
        == 13
    );

    // Not fixed: elements that vary, or lenient whitespace.
    CHECK(not pnp::default_printer<std::array<int, 3>>{}
                  .fixed_output_width()
                  .has_value());
    CHECK(not bits_pnp({.element         = array_test_digit_bits(),
                        .skip_whitespace = true})
                  .fixed_output_width()
                  .has_value());

    // Every output really has the reported width.
    const bits_pnp spaced(
        {.element = array_test_digit_bits(), .separator_spacing = u8" "}
    );
    for (const auto& value : {
             std::array<bool, 4>{},
              std::array<bool, 4>{true, true, true, true}
    })
    {
        CHECK(
            pnp::to_u8string(spaced, value).size()
            == spaced.fixed_output_width().value()
        );
    }
}

static_assert(pnp::default_printable_c<std::array<int, 3>>);
static_assert(pnp::default_parseable_c<std::array<int, 3>>);
static_assert(pnp::default_parseable_c<std::array<int, 0>>);
static_assert(not pnp::printer_c<
              pnp::default_printer<std::array<int, 3>>,
              std::array<int, 4>>);
static_assert(not pnp::printer_c<
              pnp::default_printer<std::array<int, 3>>,
              std::vector<int>>);
static_assert(pnp::support::sequence_fixed_count_v<std::array<int, 3>> == 3);
static_assert(pnp::support::sequence_fixed_count_v<std::array<int, 0>> == 0);