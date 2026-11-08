#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>
#include <version>

#if defined(__cpp_lib_mdspan)
#include <mdspan>
#endif

namespace pnp_tests
{
    // Bits as 1 and 0: one code unit each, so fixed width.
    inline pnp::default_printer<bool>
        spans_test_digit_bits()
    {
        return pnp::default_printer<bool>({ .true_value = u8"1", .false_value = u8"0" });
    }

    // Bytes as two upper-case hex digits with no prefix: fixed width 2.
    using spans_test_hex_byte = pnp::support::fixed_width_pnp<std::uint8_t, pnp::support::fixed_width_pnp_unsigned_hex>;

    inline spans_test_hex_byte
        spans_test_hex_bytes()
    {
        return spans_test_hex_byte({ .prefix = u8"", .upper = true, .fixed_size_output = true });
    }
} // namespace pnp_tests

using namespace pnp_tests;

// ============================================================ span

TEST_CASE("span: prints as a sequence", "[span]")
{
    std::vector<int> values{ 1, 2, 3, 4 };

    CHECK(pnp::to_u8string(std::span<int>(values)) == u8"[1,2,3,4]");
    CHECK(pnp::to_u8string(std::span<const int>(values)) == u8"[1,2,3,4]");
    CHECK(pnp::to_u8string(std::span<int>(values).subspan(1, 2)) == u8"[2,3]");
    CHECK(pnp::to_u8string(std::span<int>{}) == u8"[]");

    // Static extents.
    std::array<int, 3> fixed{ 5, 6, 7 };
    CHECK(pnp::to_u8string(std::span<int, 3>(fixed)) == u8"[5,6,7]");

    // Characters are elements, not text; strings are elements too.
    std::array<char, 2> chars{ 'a', 'b' };
    CHECK(pnp::to_u8string(std::span<char>(chars)) == u8"['a','b']");
    std::vector<std::string> words{ "a", "b,c" };
    CHECK(pnp::to_u8string(std::span<std::string>(words)) == u8"[\"a\",\"b,c\"]");

    // A span prints exactly as the container it refers to.
    CHECK(pnp::to_u8string(std::span<int>(values)) == pnp::to_u8string(values));
}

TEST_CASE("span: configured like sequences", "[span]")
{
    std::vector<int> values{ 1, 2, 3 };
    using span_printer = pnp::default_printer<std::span<int>>;
    CHECK(pnp::to_u8string(span_printer({ .separator_spacing = u8" " }), std::span<int>(values)) == u8"[1, 2, 3]");
}

TEST_CASE("span: a static extent of fixed-width elements prints flat", "[span][fixed_width]")
{
    std::array<bool, 4> bits{ true, false, true, false };
    using bits_span = std::span<const bool, 4>;
    using span_printer = pnp::default_printer<bits_span>;

    // No tokens at all: a block of digits.
    const span_printer flat(
        { .open_delimiter = u8"", .element = spans_test_digit_bits(), .separator = u8"", .close_delimiter = u8"" });
    CHECK(pnp::to_u8string(flat, bits_span(bits)) == u8"1010");
    CHECK(flat.fixed_output_width() == 4);

    // With the default tokens it's still fixed: [1,0,1,0].
    const span_printer bracketed({ .element = spans_test_digit_bits() });
    CHECK(pnp::to_u8string(bracketed, bits_span(bits)) == u8"[1,0,1,0]");
    CHECK(bracketed.fixed_output_width() == 9);

    // Bytes in hex, back to back.
    std::array<std::uint8_t, 4> bytes{ 0xAF, 0xD4, 0x63, 0x81 };
    using bytes_span = std::span<const std::uint8_t, 4>;
    using bytes_printer = pnp::support::generic_sequence_pnp<bytes_span, spans_test_hex_byte>;
    const bytes_printer flat_bytes(
        { .open_delimiter = u8"", .element = spans_test_hex_bytes(), .separator = u8"", .close_delimiter = u8"" });
    CHECK(pnp::to_u8string(flat_bytes, bytes_span(bytes)) == u8"AFD46381");
    CHECK(flat_bytes.fixed_output_width() == 8);
}

TEST_CASE("span: a dynamic extent keeps its delimiters", "[span][fixed_width]")
{
    std::vector<bool> bits_storage{ true, false, true, false };
    std::array<bool, 4> bits{ true, false, true, false };
    using dynamic_span = std::span<const bool>;
    using span_printer = pnp::default_printer<dynamic_span>;

    // The separator can go, since the elements are fixed width: [1010].
    const span_printer no_separator({ .element = spans_test_digit_bits(), .separator = u8"" });
    CHECK(pnp::to_u8string(no_separator, dynamic_span(bits)) == u8"[1010]");
    CHECK(not no_separator.fixed_output_width().has_value());

    // But not the delimiters: nothing else would mark the end.
    CHECK_THROWS_AS(span_printer({ .open_delimiter = u8"", .element = spans_test_digit_bits(), .separator = u8"",
                                  .close_delimiter = u8"" }),
        std::invalid_argument);
}

static_assert(pnp::default_printable_c<std::span<int>>);
static_assert(pnp::default_printable_c<std::span<const int, 3>>);
static_assert(not pnp::default_parseable_c<std::span<int>>, "spans are print-only");
static_assert(not pnp::printer_c<pnp::default_printer<std::span<int>>, std::span<int, 3>>, "extents differ");
static_assert(not pnp::printer_c<pnp::default_printer<std::span<int>>, std::vector<int>>);

// A static extent is a fixed count; a dynamic one varies.
static_assert(pnp::support::sequence_fixed_count_v<std::span<int, 3>> == 3);
static_assert(not pnp::support::sequence_fixed_count_v<std::span<int>>.has_value());

// ============================================================ mdspan

#if defined(__cpp_lib_mdspan)

TEST_CASE("mdspan: prints as nested sequences", "[mdspan]")
{
    std::vector<int> data{ 1, 2, 3, 4, 5, 6 };

    // Row-major (the default layout).
    const std::mdspan<int, std::extents<std::size_t, 2, 3>> grid(data.data());
    CHECK(pnp::to_u8string(grid) == u8"[[1,2,3],[4,5,6]]");

    // Column-major: the same data, read the other way round.
    const std::mdspan<int, std::extents<std::size_t, 2, 3>, std::layout_left> columns(data.data());
    CHECK(pnp::to_u8string(columns) == u8"[[1,3,5],[2,4,6]]");

    // Dynamic extents, and three dimensions.
    const std::mdspan<int, std::dextents<std::size_t, 2>> dynamic(data.data(), 3, 2);
    CHECK(pnp::to_u8string(dynamic) == u8"[[1,2],[3,4],[5,6]]");
    const std::mdspan<int, std::extents<std::size_t, 2, 1, 3>> cube(data.data());
    CHECK(pnp::to_u8string(cube) == u8"[[[1,2,3]],[[4,5,6]]]");

    // One dimension, and none: a rank-0 mdspan is a single element.
    const std::mdspan<int, std::extents<std::size_t, 3>> line(data.data());
    CHECK(pnp::to_u8string(line) == u8"[1,2,3]");
    int        single = 7;
    const auto scalar = std::mdspan<int, std::extents<std::size_t>>(&single);
    CHECK(pnp::to_u8string(scalar) == u8"7");
}

TEST_CASE("mdspan: empty dimensions", "[mdspan]")
{
    std::vector<int> data{ 1, 2, 3 };

    const std::mdspan<int, std::dextents<std::size_t, 2>> no_rows(data.data(), 0, 3);
    CHECK(pnp::to_u8string(no_rows) == u8"[]");

    const std::mdspan<int, std::dextents<std::size_t, 2>> empty_rows(data.data(), 2, 0);
    CHECK(pnp::to_u8string(empty_rows) == u8"[[],[]]");
}

TEST_CASE("mdspan: each dimension is configured separately", "[mdspan]")
{
    std::vector<int> data{ 1, 2, 3, 4 };
    using grid_type = std::mdspan<int, std::extents<std::size_t, 2, 2>>;
    using printer = pnp::default_printer<grid_type>;
    using rows_pnp = printer::dimension_pnp_type<0>;
    using cells_pnp = printer::dimension_pnp_type<1>;
    const grid_type grid(data.data());

    // Spacing between cells only.
    CHECK(pnp::to_u8string(printer({ .dimensions = rows_pnp({.element = cells_pnp({.separator_spacing = u8" "})}) }), grid)
        == u8"[[1, 2],[3, 4]]");

    // Spacing between rows only.
    CHECK(pnp::to_u8string(printer({ .dimensions = rows_pnp({.separator_spacing = u8" "}) }), grid) == u8"[[1,2], [3,4]]");

    // Braces and semicolons for the rows, brackets for the cells.
    CHECK(pnp::to_u8string(
        printer({ .dimensions = rows_pnp({.open_delimiter = u8"{", .separator = u8";", .close_delimiter = u8"}"}) }),
        grid)
        == u8"{[1,2];[3,4]}");

    // The innermost "dimension" is the element pnp itself.
    using cell_pnp = printer::dimension_pnp_type<2>;
    static_assert(std::same_as<cell_pnp, pnp::default_printer<int>>);
    CHECK(pnp::to_u8string(
        printer({ .dimensions = rows_pnp({.element = cells_pnp({.element = cell_pnp({.use_plus_for_positive_numbers = true})})}) }),
        grid)
        == u8"[[+1,+2],[+3,+4]]");
}

TEST_CASE("mdspan: static extents of fixed-width elements print flat", "[mdspan][fixed_width]")
{
    // A 2x4 grid of bits.
    std::array<bool, 8> bits{ true, false, true, false, false, true, true, false };
    using grid_type = std::mdspan<bool, std::extents<std::size_t, 2, 4>>;
    using printer = pnp::default_printer<grid_type>;
    using rows_pnp = printer::dimension_pnp_type<0>;
    using cells_pnp = printer::dimension_pnp_type<1>;
    const grid_type grid(bits.data());

    // Every token empty at both levels: one block of digits, row after row.
    const cells_pnp flat_cells(
        { .open_delimiter = u8"", .element = spans_test_digit_bits(), .separator = u8"", .close_delimiter = u8"" });
    const printer flat({ .dimensions = rows_pnp({.open_delimiter = u8"", .element = flat_cells, .separator = u8"",
                                                .close_delimiter = u8""}) });
    CHECK(pnp::to_u8string(flat, grid) == u8"10100110");
    CHECK(flat.fixed_output_width() == 8);

    // Flat rows, but the rows themselves bracketed and separated: [1010,0110].
    const printer flat_rows({ .dimensions = rows_pnp({.element = flat_cells}) });
    CHECK(pnp::to_u8string(flat_rows, grid) == u8"[1010,0110]");
    CHECK(flat_rows.fixed_output_width() == 11);

    // With the default tokens, a static grid is still fixed width: [[1,0,1,0],[0,1,1,0]].
    const printer bracketed({ .dimensions = rows_pnp({.element = cells_pnp({.element = spans_test_digit_bits()})}) });
    CHECK(pnp::to_u8string(bracketed, grid) == u8"[[1,0,1,0],[0,1,1,0]]");
    CHECK(bracketed.fixed_output_width() == 21);

    // A 2x2 grid of bytes in hex, flat.
    std::array<std::uint8_t, 4> bytes{ 0xAF, 0xD4, 0x63, 0x81 };
    using bytes_grid = std::mdspan<std::uint8_t, std::extents<std::size_t, 2, 2>>;
    using bytes_printer = pnp::support::generic_mdspan_printer<bytes_grid, spans_test_hex_byte>;
    using byte_rows = bytes_printer::dimension_pnp_type<0>;
    using byte_cells = bytes_printer::dimension_pnp_type<1>;
    const byte_cells flat_byte_cells(
        { .open_delimiter = u8"", .element = spans_test_hex_bytes(), .separator = u8"", .close_delimiter = u8"" });
    const bytes_printer flat_bytes({ .dimensions = byte_rows({.open_delimiter = u8"", .element = flat_byte_cells,
                                                             .separator = u8"", .close_delimiter = u8""}) });
    CHECK(pnp::to_u8string(flat_bytes, bytes_grid(bytes.data())) == u8"AFD46381");
    CHECK(flat_bytes.fixed_output_width() == 8);

    // One dimension, flat: 101.
    using line_type = std::mdspan<bool, std::extents<std::size_t, 3>>;
    using line_printer = pnp::default_printer<line_type>;
    const line_printer flat_line({ .dimensions = line_printer::dimension_pnp_type<0>(
                                      {.open_delimiter = u8"", .element = spans_test_digit_bits(), .separator = u8"",
                                       .close_delimiter = u8""}) });
    CHECK(pnp::to_u8string(flat_line, line_type(bits.data())) == u8"101");

    // No dimensions: the element's own width.
    using scalar_type = std::mdspan<bool, std::extents<std::size_t>>;
    using scalar_printer = pnp::default_printer<scalar_type>;
    const scalar_printer scalar_bit({ .dimensions = spans_test_digit_bits() });
    CHECK(pnp::to_u8string(scalar_bit, scalar_type(bits.data())) == u8"1");
    CHECK(scalar_bit.fixed_output_width() == 1);
}

TEST_CASE("mdspan: dynamic extents vary, level by level", "[mdspan][fixed_width]")
{
    std::array<bool, 8> bits{ true, false, true, false, false, true, true, false };

    // Fully dynamic: never fixed, and no level may drop its delimiters.
    using dynamic_grid = std::mdspan<bool, std::dextents<std::size_t, 2>>;
    using dynamic_pnp = pnp::default_printer<dynamic_grid>;
    CHECK(not dynamic_pnp{}.fixed_output_width().has_value());
    CHECK_THROWS_AS(dynamic_pnp({ .dimensions = dynamic_pnp::dimension_pnp_type<0>({.close_delimiter = u8""}) }),
        std::invalid_argument);

    // A dynamic outer extent with static rows: the rows can be flat, the outer level can't.
    using mixed_grid = std::mdspan<bool, std::extents<std::size_t, std::dynamic_extent, 4>>;
    using mixed_pnp = pnp::default_printer<mixed_grid>;
    using mixed_rows = mixed_pnp::dimension_pnp_type<0>;
    using mixed_row = mixed_pnp::dimension_pnp_type<1>;
    const mixed_row flat_row(
        { .open_delimiter = u8"", .element = spans_test_digit_bits(), .separator = u8"", .close_delimiter = u8"" });
    const mixed_pnp flat_rows({ .dimensions = mixed_rows({.element = flat_row}) });
    CHECK(pnp::to_u8string(flat_rows, mixed_grid(bits.data(), 2)) == u8"[1010,0110]");
    CHECK(not flat_rows.fixed_output_width().has_value());
    CHECK_THROWS_AS(mixed_pnp({ .dimensions = mixed_rows({.open_delimiter = u8"", .element = flat_row,
                                                         .close_delimiter = u8""}) }),
        std::invalid_argument);
}

// --- Compile-time checks -------------------------------------------------------------
using test_grid_type = std::mdspan<int, std::extents<std::size_t, 2, 3>>;
static_assert(pnp::default_printable_c<test_grid_type>);
static_assert(not pnp::default_parseable_c<test_grid_type>, "mdspans are print-only");
static_assert(not pnp::printer_c<pnp::default_printer<test_grid_type>, std::mdspan<int, std::extents<std::size_t, 2, 2>>>);

// Each level's count comes from its own static extent.
static_assert(pnp::support::sequence_fixed_count_v<pnp::support::mdspan_slice<test_grid_type, 0>> == 2);
static_assert(pnp::support::sequence_fixed_count_v<pnp::support::mdspan_slice<test_grid_type, 1>> == 3);
using test_mixed_type = std::mdspan<int, std::extents<std::size_t, std::dynamic_extent, 3>>;
static_assert(not pnp::support::sequence_fixed_count_v<pnp::support::mdspan_slice<test_mixed_type, 0>>.has_value());
static_assert(pnp::support::sequence_fixed_count_v<pnp::support::mdspan_slice<test_mixed_type, 1>> == 3);

#endif