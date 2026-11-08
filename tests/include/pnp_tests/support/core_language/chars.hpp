#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pnp_tests
{
using default_char_pnp = pnp::support::generic_char_pnp<char>;

// Prints with the default pnp, parses the text back, and checks the value.
template <typename CharT>
void
    check_char_round_trip(
        const CharT value
    )
{
    const auto text = pnp::to_u8string(value);
    INFO(unicode_bridge::cast_string_view<char>(
        unicode_bridge::to_formatted_unicode_string<char8_t>(text)
    ));
    const auto parsed = pnp::try_parse<CharT>(text);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value);
}

// Prints with the given pnp, checks the text, parses it back, and checks the
// value.
template <typename P, typename CharT>
void
    check_char_round_trip(
        const P&           pnp_obj,
        const CharT        value,
        std::u8string_view expected
    )
{
    const auto text = pnp::to_u8string(pnp_obj, value);
    INFO(unicode_bridge::cast_string_view<char>(text));
    if (text != expected)
    {
        int x = 4;
    }
    CHECK(text == expected);
    const auto parsed = pnp::try_parse<CharT, P>(text, pnp_obj);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value);
}

// A user policy.
struct angle_brackets
{
    static constexpr std::u8string_view open_delimiter  = u8"<";
    static constexpr std::u8string_view close_delimiter = u8">";
};

// Rejected policies.
struct bare_policy
{
    static constexpr std::u8string_view open_delimiter  = u8"";
    static constexpr std::u8string_view close_delimiter = u8"";
};

struct empty_close_policy
{
    static constexpr std::u8string_view open_delimiter  = u8"'";
    static constexpr std::u8string_view close_delimiter = u8"";
};

struct leading_space_policy
{
    static constexpr std::u8string_view open_delimiter  = u8" '";
    static constexpr std::u8string_view close_delimiter = u8"'";
};

struct trailing_space_policy
{
    static constexpr std::u8string_view open_delimiter  = u8"'";
    static constexpr std::u8string_view close_delimiter = u8"' ";
};

struct missing_close_policy
{
    static constexpr std::u8string_view open_delimiter = u8"'";
};
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "chars: plain ASCII prints quoted",
    "[chars]"
)
{
    CHECK(pnp::to_u8string('a') == u8"'a'");
    CHECK(pnp::to_u8string(L'a') == u8"'a'");
    CHECK(pnp::to_u8string(u8'a') == u8"'a'");
    CHECK(pnp::to_u8string(u'a') == u8"'a'");
    CHECK(pnp::to_u8string(U'a') == u8"'a'");

    check_char_round_trip('a');
    check_char_round_trip(L'Z');
    check_char_round_trip(u8'0');
    check_char_round_trip(u'~');
    check_char_round_trip(U' ');
}

TEST_CASE(
    "chars: special and escaped characters round-trip",
    "[chars]"
)
{
    // Whatever escape ub chooses, the parser must read it back.
    check_char_round_trip('\''); // the delimiter itself
    check_char_round_trip('\\');
    check_char_round_trip('\t');
    check_char_round_trip('\n');
    check_char_round_trip('\0');
    check_char_round_trip(U'\'');
}

TEST_CASE(
    "chars: non-ASCII and lone code units round-trip",
    "[chars]"
)
{
    // Whole code points in the wide types.
    check_char_round_trip(U'\u00E9');     // é
    check_char_round_trip(U'\U0001F600'); // 😀, outside the BMP
    check_char_round_trip(u'\u00E9');
    check_char_round_trip(L'\u00E9');

    // Single code units that aren't characters on their own: printed as
    // escapes, and read back as the same raw unit.
    check_char_round_trip(static_cast<char8_t>(0xE9)); // a lone UTF-8 lead byte
    check_char_round_trip(static_cast<char8_t>(0x80)
    ); // a lone continuation byte
    check_char_round_trip(static_cast<char16_t>(0xD83D)
    ); // a lone high surrogate
    check_char_round_trip(static_cast<char>(0xE9)
    ); // a non-ASCII byte in a plain char
}

TEST_CASE(
    "chars: characters that don't fit in one code unit",
    "[chars]"
)
{
    // é is two UTF-8 code units, so it can't be a single char8_t.
    const auto two_units = pnp::try_parse<char8_t>(u8"'é'");
    CHECK(not succeeded(two_units));
    CHECK(
        first_message(two_units).find(u8"needs 2 code units")
        != std::u8string::npos
    );

    // 😀 is a surrogate pair in UTF-16, so it can't be a single char16_t.
    const auto surrogate_pair = pnp::try_parse<char16_t>(u8"'😀'");
    CHECK(not succeeded(surrogate_pair));
    CHECK(
        first_message(surrogate_pair).find(u8"needs 2 code units")
        != std::u8string::npos
    );

    // ...but both fit in a char32_t.
    const auto wide = pnp::try_parse<char32_t>(u8"'😀'");
    REQUIRE(succeeded(wide));
    CHECK(wide.value == U'\U0001F600');
}

TEST_CASE(
    "chars: delimiter errors",
    "[chars]"
)
{
    const auto no_open = pnp::try_parse<char>(u8"a'");
    CHECK(not succeeded(no_open));
    CHECK(first_message(no_open) == u8"Expected opening delimiter \"'\".");

    const auto no_close = pnp::try_parse<char>(u8"'a");
    CHECK(not succeeded(no_close));
    CHECK(first_message(no_close) == u8"Expected closing delimiter \"'\".");

    // Two characters: reads 'a', then expects the closing delimiter but finds
    // 'b'.
    CHECK(not succeeded(pnp::try_parse<char>(u8"'ab'")));

    // Empty quotes: the second quote is read as the character, and then there's
    // no closing one.
    CHECK(not succeeded(pnp::try_parse<char>(u8"''")));

    // Trailing input after a complete character is not a full parse.
    CHECK(not succeeded(pnp::try_parse<char>(u8"'a'x")));
}

TEST_CASE(
    "chars: invalid escapes report ub's reason",
    "[chars]"
)
{
    const auto bad_escape = pnp::try_parse<char>(u8"'\\q'");
    CHECK(not succeeded(bad_escape));
    CHECK(first_message(bad_escape)
              .starts_with(u8"Could not read a character. The Unicode parser "
                           u8"returned the following error: "));
}

TEST_CASE(
    "chars: custom delimiters through options",
    "[chars]"
)
{
    const default_char_pnp p({.open_delimiter = u8"<", .close_delimiter = u8">"}
    );
    CHECK(p.options().open_delimiter == u8"<");
    CHECK(p.options().close_delimiter == u8">");
    check_char_round_trip(p, 'x', u8"<x>");

    // The default quote is now an ordinary character, and the delimiters
    // themselves can be the value: the parser reads exactly one character
    // before the close.
    check_char_round_trip(p, '\'', u8"<\\'>");
    check_char_round_trip(p, '>', u8"<>>");
    check_char_round_trip(p, '<', u8"<<>");

    // Custom delimiters appear in the error messages.
    const auto no_close = pnp::try_parse<char>(u8"<x", p);
    CHECK(first_message(no_close) == u8"Expected closing delimiter \">\".");
}

TEST_CASE(
    "chars: more option combinations",
    "[chars]"
)
{
    // Only the named field changes; the other keeps its policy value.
    const default_char_pnp open_only({.open_delimiter = u8"`"});
    check_char_round_trip(open_only, 'x', u8"`x'");

    // Multi-character and non-ASCII delimiters.
    const default_char_pnp doubled(
        {.open_delimiter = u8"<<", .close_delimiter = u8">>"}
    );
    check_char_round_trip(doubled, 'x', u8"<<x>>");
    const default_char_pnp guillemets(
        {.open_delimiter = u8"«", .close_delimiter = u8"»"}
    );
    check_char_round_trip(guillemets, 'x', u8"«x»");

    // Whitespace inside a delimiter is fine: only the ends are restricted.
    const default_char_pnp spaced(
        {.open_delimiter = u8"[ '", .close_delimiter = u8"' ]"}
    );
    check_char_round_trip(spaced, 'x', u8"[ 'x' ]");

    // A user policy works the same way.
    const pnp::support::generic_char_pnp<char, angle_brackets> policy_pnp;
    check_char_round_trip(policy_pnp, 'x', u8"<x>");

    // A modified copy of an existing configuration.
    auto modified            = guillemets.options();
    modified.close_delimiter = u8"«";
    const default_char_pnp copy(modified);
    check_char_round_trip(copy, 'x', u8"«x«");

    // The defaults inherit the options constructor.
    const pnp::default_printer<char> default_angles(
        {.open_delimiter = u8"<", .close_delimiter = u8">"}
    );
    CHECK(pnp::to_u8string(default_angles, 'x') == u8"<x>");

    // Wide character types use the same options.
    const pnp::support::generic_char_pnp<char32_t> wide(
        {.open_delimiter = u8"<", .close_delimiter = u8">"}
    );
    check_char_round_trip(wide, U'\U0001F600', u8"<😀>");
}

TEST_CASE(
    "chars: invalid options throw on construction",
    "[chars]"
)
{
    // Empty delimiters.
    CHECK_THROWS_AS(
        default_char_pnp({.open_delimiter = u8""}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_char_pnp({.close_delimiter = u8""}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        (default_char_pnp({.open_delimiter = u8"", .close_delimiter = u8""})),
        std::invalid_argument
    );

    // Whitespace at either end.
    CHECK_THROWS_AS(
        default_char_pnp({.open_delimiter = u8" '"}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_char_pnp({.open_delimiter = u8"' "}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_char_pnp({.close_delimiter = u8"\t'"}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_char_pnp({.close_delimiter = u8"'\n"}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_char_pnp({.open_delimiter = u8" "}), std::invalid_argument
    );

    // Invalid UTF-8.
    CHECK_THROWS_AS(
        default_char_pnp({.open_delimiter = half_a_character}),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_char_pnp({.close_delimiter = u8"'" + half_a_character}),
        std::invalid_argument
    );

    // The messages name the options type and the rule.
    try
    {
        default_char_pnp p({.open_delimiter = u8""});
        FAIL("expected std::invalid_argument");
    }
    catch (const std::invalid_argument& error)
    {
        CHECK(
            std::string(error.what())
            == "char_pnp_options open delimiter must be non-empty"
        );
    }

    CHECK_NOTHROW(
        default_char_pnp({.open_delimiter = u8"<", .close_delimiter = u8">"})
    );
}

TEST_CASE(
    "chars: inside containers",
    "[chars]"
)
{
    const std::vector<char> letters{'a', 'b', 'c'};
    CHECK(pnp::to_u8string(letters) == u8"['a','b','c']");
    const auto parsed = pnp::try_parse<std::vector<char>>(u8"['a','b','c']");
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == letters);

    // Characters that are also the container's own punctuation.
    const std::vector<char> punctuation{'[', ',', ']'};
    const auto              punctuation_text = pnp::to_u8string(punctuation);
    CHECK(punctuation_text == u8"['[',',',']']");
    const auto parsed_punctuation
        = pnp::try_parse<std::vector<char>>(punctuation_text);
    REQUIRE(succeeded(parsed_punctuation));
    CHECK(parsed_punctuation.value == punctuation);
}

TEST_CASE(
    "read ahead: a char stops at its closing delimiter",
    "[io][read_ahead][chars]"
)
{
    {
        INFO("a plain character");
        const auto outcome
            = parse_from_pieces<char>({u8"'", u8"a", u8"'", u8"\n", u8"'b'"});
        CHECK(outcome.succeeded);
        CHECK(outcome.value == 'a');
        CHECK(outcome.pieces_read == 3); // nothing after the closing quote
    }
    {
        INFO("an escaped character");
        const auto outcome = parse_from_pieces<char>(
            {u8"'", u8"\\", u8"n", u8"'", u8"\n", u8"'b'"}
        );
        CHECK(outcome.succeeded);
        CHECK(outcome.value == '\n');
        CHECK(outcome.pieces_read == 4);
    }
    {
        INFO("an escaped quote");
        const auto outcome = parse_from_pieces<char>(
            {u8"'", u8"\\", u8"'", u8"'", u8"\n", u8"'b'"}
        );
        CHECK(outcome.succeeded);
        CHECK(outcome.value == '\'');
        CHECK(outcome.pieces_read == 4);
    }
    {
        INFO("a character of two code units, split between them");
        const auto outcome = parse_from_pieces<char8_t>(
            {u8"'",
             std::u8string(1, static_cast<char8_t>(0xC3)),
             std::u8string(1, static_cast<char8_t>(0xA9)),
             u8"'",
             u8"\n",
             u8"'b'"}
        );
        // é needs two code units, so a char8_t can't hold it, but the char pnp
        // still has to read the whole character to say so, and no further.
        CHECK(not outcome.succeeded);
        CHECK(outcome.pieces_read == 3);
    }
}

TEST_CASE(
    "read ahead: a char with a longer escape",
    "[io][read_ahead][chars]"
)
{
    // An escape whose body is several characters: read to its end, then the
    // closing quote, and no further. Adjust the escape to unicode_bridge's
    // syntax for a code point, if it differs.
    const auto outcome = parse_from_pieces<char16_t>(
        {u8"'",
         u8"\\",
         u8"u",
         u8"0",
         u8"0",
         u8"4",
         u8"1",
         u8"'",
         u8"\n",
         u8"'b'"}
    );
    CHECK(outcome.succeeded);
    CHECK(outcome.value == 'A');
    CHECK(outcome.pieces_read == 8);
}

TEST_CASE(
    "read ahead: char values typed one per line come back straight away",
    "[io][read_ahead][value_reader]"
)
{
    {
        counting_source         source({u8"'x'", u8"\n", u8"'y'", u8"\n"});
        pnp::value_reader<char> reader(source);
        const auto              first = reader.next();
        REQUIRE(first.has_value());
        CHECK(first->value == 'x');
        CHECK(
            source.given == 2
        ); // the char, then the newline the reader checks for
    }
}

// --- Policy validation at compile time ---------------------------------------
static_assert(pnp::support::valid_char_pnp_c<
              pnp::support::char_pnp_single_quoted>);
static_assert(pnp::support::valid_char_pnp_c<angle_brackets>);
static_assert(not pnp::support::valid_char_pnp_c<bare_policy>);
static_assert(not pnp::support::valid_char_pnp_c<empty_close_policy>);
static_assert(not pnp::support::valid_char_pnp_c<leading_space_policy>);
static_assert(not pnp::support::valid_char_pnp_c<trailing_space_policy>);
static_assert(not pnp::support::valid_char_pnp_c<missing_close_policy>);

// Validation stays wired up, and options are shared across character types.
static_assert(pnp::support::checkable_options_c<pnp::support::char_pnp_options<
                  pnp::support::char_pnp_single_quoted>>);
static_assert(std::same_as<
              pnp::support::generic_char_pnp<char>::options_type,
              pnp::support::generic_char_pnp<char32_t>::options_type>);

// --- The exact-type guard ----------------------------------------------------
static_assert(pnp::printer_c<pnp::default_printer<char>, char>);
static_assert(not pnp::printer_c<pnp::default_printer<char>, int>);
static_assert(not pnp::printer_c<pnp::default_printer<char>, char8_t>);
static_assert(not pnp::printer_c<pnp::default_printer<char32_t>, char>);
static_assert(not pnp::printer_c<pnp::default_printer<char8_t>, unsigned char>);

// signed char and unsigned char are integers, not characters.
static_assert(not pnp::
                  printer_c<pnp::support::generic_char_pnp<char>, signed char>);