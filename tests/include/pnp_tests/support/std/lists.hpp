#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <deque>
#include <forward_list>
#include <list>
#include <string>
#include <string_view>
#include <vector>

namespace pnp_tests
{
// Prints with the default printer, checks the text, parses it back, and checks
// the value.
template <typename Container>
void
    check_list_like_round_trip(
        const Container&   value,
        std::u8string_view expected
    )
{
    const auto text = pnp::to_u8string(value);
    CHECK(text == expected);
    const auto parsed = pnp::try_parse<Container>(text);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value);
}

// Bits as 1 and 0: fixed width 1.
inline pnp::support::bool_pnp<>
    list_test_digit_bits()
{
    return pnp::support::bool_pnp<>({.true_value = u8"1", .false_value = u8"0"}
    );
}

// Bytes as two upper-case hex digits with no prefix: fixed width 2.
using list_test_hex_byte = pnp::support::
    fixed_width_pnp<std::uint8_t, pnp::support::fixed_width_pnp_unsigned_hex>;

inline list_test_hex_byte
    list_test_hex_bytes()
{
    return list_test_hex_byte(
        {.prefix = u8"", .upper = true, .fixed_size_output = true}
    );
}

// Prints with a given pnp, checks the text, parses it back with the same pnp,
// and checks the value.
template <typename P, typename Container>
void
    check_list_like_round_trip_with(
        const P&           pnp_obj,
        const Container&   value,
        std::u8string_view expected
    )
{
    const auto text = pnp::to_u8string(pnp_obj, value);
    CHECK(text == expected);
    const auto parsed = pnp::try_parse<Container>(text, pnp_obj);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value);
}

// The checks shared by all three containers, for one container template.
template <template <typename...> typename Container>
void
    check_list_like_container()
{
    check_list_like_round_trip(Container<int>{1, 2, 3}, u8"[1,2,3]");
    check_list_like_round_trip(Container<int>{42}, u8"[42]");
    check_list_like_round_trip(Container<int>{}, u8"[]");

    // The order is kept, including when it isn't sorted.
    check_list_like_round_trip(Container<int>{3, 1, 2}, u8"[3,1,2]");

    // Strings with punctuation, and nested containers.
    check_list_like_round_trip(
        Container<std::string>{"a", "b,c", "[d]"}, u8"[\"a\",\"b,c\",\"[d]\"]"
    );
    check_list_like_round_trip(
        Container<std::vector<int>>{
            {1, 2},
            {}
    },
        u8"[[1,2],[]]"
    );
    pnp::to_u8string(std::pmr::vector<int>{1, 2});
    // Parse errors.
    const auto no_open = pnp::try_parse<Container<int>>(u8"1,2]");
    CHECK(not succeeded(no_open));
    CHECK(first_message(no_open) == u8"Expected opening delimiter \"[\".");

    const auto no_close = pnp::try_parse<Container<int>>(u8"[1,2");
    CHECK(not succeeded(no_close));
    CHECK(
        first_message(no_close)
        == u8"Expected separator \",\" or closing delimiter \"]\"."
    );

    const auto bad_element = pnp::try_parse<Container<int>>(u8"[1,x]");
    CHECK(not succeeded(bad_element));
    CHECK(
        first_message(bad_element)
        == u8"Expected a valid integer, but no valid digits were found."
    );

    CHECK(not succeeded(pnp::try_parse<Container<int>>(u8"[1,2,]")));
    CHECK(not succeeded(pnp::try_parse<Container<int>>(u8"[1,2]x")));

    // Configuration: spacing, lenient whitespace and trailing separators.
    using printer = pnp::default_printer<Container<int>>;
    using parser  = pnp::default_parser<Container<int>>;
    CHECK(
        pnp::to_u8string(
            printer({.separator_spacing = u8" "}), Container<int>{1, 2, 3}
        )
        == u8"[1, 2, 3]"
    );

    const parser lenient(
        {.skip_whitespace = true, .allow_trailing_separator = true}
    );
    const auto relaxed
        = pnp::try_parse<Container<int>>(u8"[ 1 , 2,\n3, ]", lenient);
    REQUIRE(succeeded(relaxed));
    CHECK(relaxed.value == Container<int>{1, 2, 3});
    // Fixed-width elements can drop the separator: [1011] and [FFDDEC].
    using bits_pnp = pnp::support::
        generic_sequence_pnp<Container<bool>, pnp::support::bool_pnp<>>;
    const bits_pnp no_separator(
        {.element = list_test_digit_bits(), .separator = u8""}
    );
    check_list_like_round_trip_with(
        no_separator, Container<bool>{true, false, true, true}, u8"[1011]"
    );
    check_list_like_round_trip_with(no_separator, Container<bool>{}, u8"[]");

    using bytes_pnp = pnp::support::
        generic_sequence_pnp<Container<std::uint8_t>, list_test_hex_byte>;
    const bytes_pnp packed_bytes(
        {.element = list_test_hex_bytes(), .separator = u8""}
    );
    check_list_like_round_trip_with(
        packed_bytes, Container<std::uint8_t>{0xFF, 0xDD, 0xEC}, u8"[FFDDEC]"
    );

    // Lenient whitespace still works between packed elements.
    const bits_pnp lenient_bits(
        {.element         = list_test_digit_bits(),
         .separator       = u8"",
         .skip_whitespace = true}
    );
    const auto spaced_bits = pnp::try_parse<Container<bool>>(
        u8"[ 1 0\t1 ]", lenient_bits
    );
    REQUIRE(succeeded(spaced_bits));
    CHECK(spaced_bits.value == Container<bool>{true, false, true});

    // A list's length varies, so its delimiters stay, whatever its elements.
    CHECK(
        invalid_argument_message(
            []
            {
                bits_pnp p(
                    {.open_delimiter  = u8"",
                     .element         = list_test_digit_bits(),
                     .separator       = u8"",
                     .close_delimiter = u8""}
                );
            }
        )
        == "sequence_pnp_options delimiters may only be empty for a fixed "
           "number of elements that all print with "
           "the same width"
    );

    // Elements that aren't fixed width need a separator.
    CHECK_THROWS_AS(printer({.separator = u8""}), std::invalid_argument);

    // And a list is never fixed width.
    CHECK(not no_separator.fixed_output_width().has_value());
    CHECK(not printer{}.fixed_output_width().has_value());
}
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "in pieces: sequences",
    "[io][pieces][sequences]"
)
{
    check_round_trip_in_pieces(std::vector<int>{1, 22, 333, -4'444});
    check_round_trip_in_pieces(std::vector<int>{});
    check_round_trip_in_pieces(std::vector<std::vector<int>>{
        {1, 2},
        {},
        {3}
    });
    check_round_trip_in_pieces(
        std::vector<std::u8string>{u8"a,b", u8"]", u8"\"quoted\""}
    );

    // Spacing, which strict parsing checks with a lookahead.
    using vector_pnp = pnp::default_parser<std::vector<int>>;
    check_parses_in_pieces_with<vector_pnp, std::vector<int>>(
        vector_pnp({.separator_spacing = u8" ", .delimiter_spacing = u8"  "}),
        u8"[  1, 2, 3  ]"
    );

    // Lenient whitespace, read across pieces.
    check_parses_in_pieces_with<vector_pnp, std::vector<int>>(
        vector_pnp({.skip_whitespace = true}), u8"[ 1 ,\t2,  3 ]"
    );

    // A trailing separator, when allowed.
    check_parses_in_pieces_with<vector_pnp, std::vector<int>>(
        vector_pnp({.allow_trailing_separator = true}), u8"[1,2,]"
    );

    // Failures fail at the same place, however the input is split.
    check_parses_in_pieces<std::vector<int>>(u8"[1,2");
    check_parses_in_pieces<std::vector<int>>(u8"[1;2]");
    check_parses_in_pieces<std::vector<int>>(u8"[1,,2]");
    check_parses_in_pieces_with<vector_pnp, std::vector<int>>(
        vector_pnp({.delimiter_spacing = u8" "}), u8"[ 1]"
    );
}

TEST_CASE(
    "across lines: one entry per line, parsed strictly",
    "[io][lines]"
)
{
    // A format where each entry is on its own line: newlines as the spacing.
    using vector_pnp = pnp::default_parser<std::vector<int>>;
    const vector_pnp one_per_line(
        {.separator_spacing = u8"\n", .delimiter_spacing = u8"\n"}
    );

    const std::vector<int> values{10, 20, 30};
    const std::u8string    printed = pnp::to_u8string(one_per_line, values);
    CHECK(printed == u8"[\n10,\n20,\n30\n]");

    const auto parsed = parse_lines_with<vector_pnp, std::vector<int>>(
        one_per_line, std::string(printed.begin(), printed.end())
    );
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == values);
}

TEST_CASE(
    "across lines: lenient whitespace",
    "[io][lines]"
)
{
    using vector_pnp = pnp::default_parser<std::vector<int>>;
    const vector_pnp lenient({.skip_whitespace = true});

    // Entries on separate lines, with indentation and blank lines.
    const auto parsed = parse_lines_with<vector_pnp, std::vector<int>>(
        lenient, "[\n    1,\n    2,\n\n    3\n]\n"
    );
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == std::vector<int>{1, 2, 3});

    // Windows line endings.
    const auto crlf = parse_lines_with<vector_pnp, std::vector<int>>(
        lenient, "[\r\n1,\r\n2\r\n]\r\n"
    );
    REQUIRE(succeeded(crlf));
    CHECK(crlf.value == std::vector<int>{1, 2});

    // An entry split across lines is still an error: whitespace can't go inside
    // a number.
    const auto split
        = parse_lines_with<vector_pnp, std::vector<int>>(lenient, "[1,2\n3]");
    CHECK(not succeeded(split));
}

TEST_CASE(
    "across lines: nested lists",
    "[io][lines]"
)
{
    using inner_pnp = pnp::default_parser<std::vector<int>>;
    using outer_pnp = pnp::default_parser<std::vector<std::vector<int>>>;
    const outer_pnp lenient(
        {.element         = inner_pnp({.skip_whitespace = true}),
         .skip_whitespace = true}
    );

    const auto parsed
        = parse_lines_with<outer_pnp, std::vector<std::vector<int>>>(
            lenient,
            "[\n"
            "  [1, 2],\n"
            "  [],\n"
            "  [\n"
            "    3\n"
            "  ]\n"
            "]\n"
        );
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == std::vector<std::vector<int>>{{1, 2}, {}, { 3 }});
}

TEST_CASE(
    "across lines: a long list, read in small blocks",
    "[io][lines]"
)
{
    // A thousand entries, one per line, read from a stream in 7-byte blocks, so
    // entries are constantly split across reads.
    std::string      text = "[\n";
    std::vector<int> expected;
    for (int idx = 0; idx < 1'000; ++idx)
    {
        text += std::to_string(idx * 37) + (idx + 1 < 1'000 ? ",\n" : "\n");
        expected.push_back(idx * 37);
    }
    text             += "]";

    using vector_pnp  = pnp::default_parser<std::vector<int>>;
    std::istringstream  stream(text);
    pnp::istream_source source(stream, true, 7);
    const auto          parsed = pnp::try_parse<std::vector<int>>(
        source, vector_pnp({.skip_whitespace = true})
    );
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == expected);
}

TEST_CASE(
    "std::deque",
    "[deque]"
)
{
    check_list_like_container<std::deque>();
}

TEST_CASE(
    "std::list",
    "[list]"
)
{
    check_list_like_container<std::list>();
}

TEST_CASE(
    "std::forward_list",
    "[forward_list]"
)
{
    check_list_like_container<std::forward_list>();

    // Elements are appended after the last one, not pushed to the front.
    const auto parsed = pnp::try_parse<std::forward_list<int>>(u8"[1,2,3,4,5]");
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == std::forward_list<int>{1, 2, 3, 4, 5});
}

TEST_CASE(
    "std::vector",
    "[vector]"
)
{
    check_list_like_container<std::vector>();

    // vector<bool> stores proxies rather than bools, but prints and parses like
    // any other.
    check_list_like_round_trip(
        std::vector<bool>{true, false, true}, u8"[true,false,true]"
    );
}

// --- Defaults and appenders
// ------------------------------------------------------
static_assert(pnp::default_printable_c<std::deque<int>>);
static_assert(pnp::default_parseable_c<std::deque<int>>);
static_assert(pnp::default_printable_c<std::list<int>>);
static_assert(pnp::default_parseable_c<std::list<int>>);
static_assert(pnp::default_printable_c<std::forward_list<int>>);
static_assert(pnp::default_parseable_c<std::forward_list<int>>);

static_assert(std::is_base_of_v<
              pnp::support::push_back_appender<std::deque<int>>,
              pnp::support::default_appender<std::deque<int>>>);
static_assert(std::is_base_of_v<
              pnp::support::push_back_appender<std::list<int>>,
              pnp::support::default_appender<std::list<int>>>);
static_assert(std::is_base_of_v<
              pnp::support::insert_after_appender<std::forward_list<int>>,
              pnp::support::default_appender<std::forward_list<int>>>);

// --- The exact-type guard ----------------------------------------------------
static_assert(not pnp::printer_c<
              pnp::default_printer<std::deque<int>>,
              std::list<int>>);
static_assert(not pnp::printer_c<
              pnp::default_printer<std::list<int>>,
              std::forward_list<int>>);
static_assert(not pnp::printer_c<
              pnp::default_printer<std::forward_list<int>>,
              std::vector<int>>);
static_assert(not pnp::printer_c<
              pnp::default_printer<std::deque<int>>,
              std::deque<long>>);