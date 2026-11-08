#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace pnp_tests
{
// Prints with the default printer, checks the text, parses it back with the
// default parser, and checks the value. The text is checked only when an
// expectation is given.
template <typename Map>
void
    check_map_round_trip(
        const Map&         value,
        std::u8string_view expected = {}
    )
{
    const auto text = pnp::to_u8string(value);
    if (not expected.empty())
    {
        CHECK(text == expected);
    }
    const auto parsed = pnp::try_parse<Map>(text);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value);
}

// The same, with a given printer and parser: a map's printer and parser have
// different entry types, so a configured round trip needs one of each.
template <typename Printer, typename Parser, typename Map>
void
    check_map_round_trip_with(
        const Printer&     printer_arg,
        const Parser&      parser_arg,
        const Map&         value,
        std::u8string_view expected
    )
{
    const auto text = pnp::to_u8string(printer_arg, value);
    CHECK(text == expected);
    const auto parsed = pnp::try_parse<Map>(text, parser_arg);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value);
}

using int_string_map         = std::map<int, std::string>;
using int_string_map_printer = pnp::default_printer<int_string_map>;
using int_string_map_parser  = pnp::default_parser<int_string_map>;

struct map_test_no_defaults
{};

// Keys as two upper-case hex digits with no prefix, values as 1 and 0: both
// fixed width.
using map_test_hex_byte = pnp::support::
    fixed_width_pnp<std::uint8_t, pnp::support::fixed_width_pnp_unsigned_hex>;

inline map_test_hex_byte
    map_test_hex_bytes()
{
    return map_test_hex_byte(
        {.prefix = u8"", .upper = true, .fixed_size_output = true}
    );
}

inline pnp::support::bool_pnp<>
    map_test_digit_bits()
{
    return pnp::support::bool_pnp<>({.true_value = u8"1", .false_value = u8"0"}
    );
}

// A map of bytes to bits, with fixed-width entries: the printer sees
// std::pair<const K, V>, the parser std::pair<K, V>, as for every map.
using packed_map   = std::map<std::uint8_t, bool>;
using packed_parts = std::tuple<map_test_hex_byte, pnp::support::bool_pnp<>>;
using packed_entry_printer = pnp::support::generic_tuple_pnp<
    std::pair<const std::uint8_t, bool>,
    packed_parts,
    pnp::support::map_entry_pnp_default>;
using packed_entry_parser = pnp::support::generic_tuple_pnp<
    std::pair<std::uint8_t, bool>,
    packed_parts,
    pnp::support::map_entry_pnp_default>;
using packed_map_printer = pnp::support::generic_sequence_pnp<
    packed_map,
    packed_entry_printer,
    pnp::support::map_pnp_default>;
using packed_map_parser = pnp::support::generic_sequence_pnp<
    packed_map,
    packed_entry_parser,
    pnp::support::map_pnp_default>;

// The entry options, with or without the colon.
template <typename Entry>
Entry
    packed_entry(
        const bool with_colon_arg
    )
{
    return Entry({
        .elements  = {map_test_hex_bytes(), map_test_digit_bits()},
        .separator = with_colon_arg ? u8":" : u8""
    });
}
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "across lines: a map, one entry per line",
    "[io][lines]"
)
{
    using map_pnp = pnp::default_parser<std::map<std::u8string, int>>;
    const map_pnp lenient(
        {.skip_whitespace = true, .allow_trailing_separator = true}
    );
    auto       str = "{\n  \"apples\":3,\n  \"pears\":5,\n  \"plums\":0,\n}\n";
    const auto parsed
        = parse_lines_with<map_pnp, std::map<std::u8string, int>>(lenient, str);
    INFO(unicode_bridge::cast_ascii<char>(str));
    if (not succeeded(parsed))
    {
        int x = 4;
    }
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == std::map<std::u8string, int>{{u8"apples", 3}, { u8"pears", 5 }, { u8"plums", 0 }});
}

TEST_CASE(
    "maps: std::map round-trips in key order",
    "[maps]"
)
{
    check_map_round_trip(
        int_string_map{
            {1, "a"},
            {2, "b"}
    },
        u8"{1:\"a\",2:\"b\"}"
    );
    check_map_round_trip(
        int_string_map{
            {2, "b"},
            {1, "a"}
    },
        u8"{1:\"a\",2:\"b\"}"
    ); // sorted by key
    check_map_round_trip(
        int_string_map{
            {1, "a"}
    },
        u8"{1:\"a\"}"
    );
    check_map_round_trip(int_string_map{}, u8"{}");

    // Containers and maps as values.
    check_map_round_trip(
        std::map<std::string, std::vector<int>>{
            {"a", {1, 2}},
            {"b", {}    }
    },
        u8"{\"a\":[1,2],\"b\":[]}"
    );
    check_map_round_trip(
        std::map<int, std::map<int, int>>{
            {1, {{2, 3}}}
    },
        u8"{1:{2:3}}"
    );

    // Keys and values containing the map's own punctuation.
    check_map_round_trip(std::map<std::string, std::string>{
        {"a:b", "c,d"},
        {"{x}", "y}" }
    });
}

TEST_CASE(
    "maps: a custom comparator decides the order",
    "[maps]"
)
{
    check_map_round_trip(
        std::map<int, int, std::greater<int>>{
            {1, 10},
            {2, 20}
    },
        u8"{2:20,1:10}"
    );
}

TEST_CASE(
    "maps: std::multimap keeps repeated keys",
    "[maps]"
)
{
    // Equal keys keep their insertion order.
    const std::multimap<int, std::string> values{
        {1, "a"},
        {1, "b"},
        {2, "c"}
    };
    check_map_round_trip(values, u8"{1:\"a\",1:\"b\",2:\"c\"}");
    check_map_round_trip(std::multimap<int, std::string>{}, u8"{}");

    // Parsing keeps repeated keys too.
    const auto parsed
        = pnp::try_parse<std::multimap<int, int>>(u8"{1:10,1:11,1:12}");
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value.count(1) == 3);
}

TEST_CASE(
    "maps: std::unordered_map round-trips",
    "[maps]"
)
{
    // With one entry or none, the text is predictable.
    check_map_round_trip(
        std::unordered_map<int, std::string>{
            {1, "a"}
    },
        u8"{1:\"a\"}"
    );
    check_map_round_trip(std::unordered_map<int, std::string>{}, u8"{}");

    // Otherwise, the order is unspecified, so only the round trip is checked.
    check_map_round_trip(std::unordered_map<int, std::string>{
        {1, "a"},
        {2, "b"},
        {3, "c"}
    });
    check_map_round_trip(std::unordered_map<std::string, std::vector<int>>{
        {"x", {1}   },
        {"y", {2, 3}}
    });
}

TEST_CASE(
    "maps: std::unordered_multimap keeps repeated keys",
    "[maps]"
)
{
    check_map_round_trip(
        std::unordered_multimap<int, std::string>{
            {1, "a"}
    },
        u8"{1:\"a\"}"
    );
    check_map_round_trip(std::unordered_multimap<int, std::string>{
        {1, "a"},
        {1, "b"},
        {2, "c"}
    });

    const auto parsed
        = pnp::try_parse<std::unordered_multimap<int, int>>(u8"{1:10,1:11}");
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value.count(1) == 2);
}

TEST_CASE(
    "maps: repeated keys are an error for unique maps",
    "[maps]"
)
{
    const auto ordered = pnp::try_parse<int_string_map>(u8"{1:\"a\",1:\"b\"}");
    CHECK(not succeeded(ordered));
    CHECK(
        first_message(ordered)
        == u8"Could not add element 1 to the sequence: An element with the "
           u8"same key is already in the container."
    );

    const auto unordered = pnp::try_parse<std::unordered_map<int, std::string>>(
        u8"{1:\"a\",1:\"b\"}"
    );
    CHECK(not succeeded(unordered));
    CHECK(
        first_message(unordered)
        == u8"Could not add element 1 to the sequence: An element with the "
           u8"same key is already in the container."
    );

    // A repeated key with the same value is still a repeated key.
    CHECK(not succeeded(pnp::try_parse<int_string_map>(u8"{1:\"a\",1:\"a\"}")));
}

TEST_CASE(
    "maps: configuring the map and its entries",
    "[maps]"
)
{
    const int_string_map values{
        {1, "a"},
        {2, "b"}
    };

    // A different key-value separator, on both the printer and the parser.
    check_map_round_trip_with(
        int_string_map_printer(
            {.element
             = int_string_map_printer::element_part_type({.separator = u8"=>"})}
        ),
        int_string_map_parser(
            {.element
             = int_string_map_parser::element_part_type({.separator = u8"=>"})}
        ),
        values,
        u8"{1=>\"a\",2=>\"b\"}"
    );

    // Spacing between entries, and inside each entry.
    check_map_round_trip_with(
        int_string_map_printer({.separator_spacing = u8" "}),
        int_string_map_parser({.separator_spacing = u8" "}),
        values,
        u8"{1:\"a\", 2:\"b\"}"
    );
    check_map_round_trip_with(
        int_string_map_printer(
            {.element = int_string_map_printer::element_part_type(
                 {.separator_spacing = u8" "}
             )}
        ),
        int_string_map_parser(
            {.element = int_string_map_parser::element_part_type(
                 {.separator_spacing = u8" "}
             )}
        ),
        values,
        u8"{1: \"a\",2: \"b\"}"
    );

    // The entry reads back through the map's options.
    const int_string_map_printer arrows(
        {.element
         = int_string_map_printer::element_part_type({.separator = u8"=>"})}
    );
    CHECK(arrows.options().element.options().separator == u8"=>");
}

TEST_CASE(
    "maps: lenient whitespace",
    "[maps]"
)
{
    // The map and its entries each have their own option, so both are needed
    // for spaces around the braces, the commas and the colons.
    const int_string_map_parser lenient({
        .element
        = int_string_map_parser::element_part_type({.skip_whitespace = true}),
        .skip_whitespace = true,
    });
    const auto                  parsed
        = pnp::try_parse<int_string_map>(u8"{ 1 : \"a\" ,\n2:\"b\" }", lenient);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == int_string_map{ {1, "a"}, {2, "b"} });

    // With only the map lenient, spaces around a colon are still an error.
    const int_string_map_parser map_only({.skip_whitespace = true});
    CHECK(not succeeded(
        pnp::try_parse<int_string_map>(u8"{ 1 : \"a\" }", map_only)
    ));
}

TEST_CASE(
    "maps: parse errors",
    "[maps]"
)
{
    const auto no_open = pnp::try_parse<int_string_map>(u8"1:\"a\"}");
    CHECK(not succeeded(no_open));
    CHECK(first_message(no_open) == u8"Expected opening delimiter \"{\".");

    // An entry without its colon.
    const auto no_colon = pnp::try_parse<int_string_map>(u8"{1\"a\"}");
    CHECK(not succeeded(no_colon));
    CHECK(
        first_message(no_colon)
        == u8"Expected separator \":\" after element 0 of a tuple."
    );

    // An unclosed map.
    const auto no_close = pnp::try_parse<int_string_map>(u8"{1:\"a\"");
    CHECK(not succeeded(no_close));
    CHECK(
        first_message(no_close)
        == u8"Expected separator \",\" or closing delimiter \"}\"."
    );

    // A bad key or value, square brackets, and trailing input.
    CHECK(not succeeded(pnp::try_parse<int_string_map>(u8"{x:\"a\"}")));
    CHECK(not succeeded(pnp::try_parse<int_string_map>(u8"{1:a}")));
    CHECK(not succeeded(pnp::try_parse<int_string_map>(u8"[1:\"a\"]")));
    CHECK(not succeeded(pnp::try_parse<int_string_map>(u8"{}x")));
}

TEST_CASE(
    "maps: fixed-width entries can drop the separator",
    "[maps][fixed_width]"
)
{
    const packed_map values{
        {0xAF, true },
        {0xDD, false}
    };

    // The defaults: {AF:1,DD:0}.
    check_map_round_trip_with(
        packed_map_printer({.element = packed_entry<packed_entry_printer>(true)}
        ),
        packed_map_parser({.element = packed_entry<packed_entry_parser>(true)}),
        values,
        u8"{AF:1,DD:0}"
    );

    // Entries packed: {AF:1DD:0}.
    check_map_round_trip_with(
        packed_map_printer(
            {.element   = packed_entry<packed_entry_printer>(true),
             .separator = u8""}
        ),
        packed_map_parser(
            {.element   = packed_entry<packed_entry_parser>(true),
             .separator = u8""}
        ),
        values,
        u8"{AF:1DD:0}"
    );

    // Entries packed, and each entry packed: {AF1DD0}.
    check_map_round_trip_with(
        packed_map_printer(
            {.element   = packed_entry<packed_entry_printer>(false),
             .separator = u8""}
        ),
        packed_map_parser(
            {.element   = packed_entry<packed_entry_parser>(false),
             .separator = u8""}
        ),
        values,
        u8"{AF1DD0}"
    );

    // An empty map still has its braces.
    check_map_round_trip_with(
        packed_map_printer(
            {.element   = packed_entry<packed_entry_printer>(false),
             .separator = u8""}
        ),
        packed_map_parser(
            {.element   = packed_entry<packed_entry_parser>(false),
             .separator = u8""}
        ),
        packed_map{},
        u8"{}"
    );
}

TEST_CASE(
    "maps: packed maps still check their keys",
    "[maps][fixed_width]"
)
{
    const packed_map_parser packed(
        {.element = packed_entry<packed_entry_parser>(false), .separator = u8""}
    );

    // A repeated key is still an error, at the second entry.
    const auto repeated = pnp::try_parse<packed_map>(u8"{AF1AF0}", packed);
    CHECK(not succeeded(repeated));
    CHECK(
        first_message(repeated)
        == u8"Could not add element 1 to the sequence: An element with the "
           u8"same key is already in the container."
    );

    // A short entry, and an unclosed map.
    CHECK(not succeeded(pnp::try_parse<packed_map>(u8"{AF}", packed)));
    CHECK(not succeeded(pnp::try_parse<packed_map>(u8"{AF1", packed)));
}

TEST_CASE(
    "maps: when tokens may be empty",
    "[maps][fixed_width]"
)
{
    // A map's length varies, so its braces stay, whatever its entries.
    CHECK_THROWS_AS(
        packed_map_printer(
            {.open_delimiter  = u8"",
             .element         = packed_entry<packed_entry_printer>(false),
             .separator       = u8"",
             .close_delimiter = u8""}
        ),
        std::invalid_argument
    );

    // Entries whose values vary need a separator between them...
    CHECK_THROWS_AS(
        int_string_map_printer({.separator = u8""}), std::invalid_argument
    );

    // ...and an entry whose parts vary needs its own separator.
    CHECK_THROWS_AS(
        int_string_map_printer::element_part_type({.separator = u8""}),
        std::invalid_argument
    );
}

TEST_CASE(
    "maps: fixed output widths",
    "[maps][fixed_width]"
)
{
    // An entry is fixed when its key and value are: AF:1 is 4, AF1 is 3.
    CHECK(packed_entry<packed_entry_printer>(true).fixed_output_width() == 4);
    CHECK(packed_entry<packed_entry_printer>(false).fixed_output_width() == 3);

    // A map never is, since its length varies.
    CHECK(not packed_map_printer({.element
                                  = packed_entry<packed_entry_printer>(false),
                                  .separator = u8""})
                  .fixed_output_width()
                  .has_value());
    CHECK(not int_string_map_printer{}.fixed_output_width().has_value());

    // An entry with a value that varies isn't fixed either.
    CHECK(not int_string_map_printer::element_part_type{}
                  .fixed_output_width()
                  .has_value());
}

// --- Defaults
// ------------------------------------------------------------------
static_assert(pnp::default_printable_c<std::map<int, std::string>>);
static_assert(pnp::default_parseable_c<std::map<int, std::string>>);
static_assert(pnp::default_printable_c<std::multimap<int, std::string>>);
static_assert(pnp::default_parseable_c<std::multimap<int, std::string>>);
static_assert(pnp::default_printable_c<std::unordered_map<int, std::string>>);
static_assert(pnp::default_parseable_c<std::unordered_map<int, std::string>>);
static_assert(pnp::default_printable_c<
              std::unordered_multimap<int, std::string>>);
static_assert(not pnp::printer_c<
              pnp::default_printer<std::map<int, int>>,
              std::map<int, long>>);