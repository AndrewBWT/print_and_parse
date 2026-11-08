#pragma once
#include "pnp/sources/istream_text_source.hpp"
#include "pnp/sources/testing/piece_source.hpp"
#include "pnp/sources/string_source.hpp"
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <optional>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace pnp_tests;

// ============================================================ types

TEST_CASE(
    "input_text and input_mark: moved, never copied",
    "[parser_input][types]"
)
{
    STATIC_CHECK(not std::is_copy_constructible_v<pnp::input_text>);
    STATIC_CHECK(not std::is_copy_assignable_v<pnp::input_text>);
    STATIC_CHECK(std::is_nothrow_move_constructible_v<pnp::input_text>);
    STATIC_CHECK(std::is_nothrow_move_assignable_v<pnp::input_text>);
    STATIC_CHECK(not std::is_default_constructible_v<pnp::input_text>);

    STATIC_CHECK(not std::is_copy_constructible_v<pnp::input_mark>);
    STATIC_CHECK(not std::is_copy_assignable_v<pnp::input_mark>);
    STATIC_CHECK(std::is_nothrow_move_constructible_v<pnp::input_mark>);
    STATIC_CHECK(std::is_nothrow_move_assignable_v<pnp::input_mark>);
    STATIC_CHECK(not std::is_default_constructible_v<pnp::input_mark>);
}

TEST_CASE(
    "parser_input: neither copied nor moved, and never made by accident",
    "[parser_input][types]"
)
{
    STATIC_CHECK(not std::is_copy_constructible_v<pnp::parser_input>);
    STATIC_CHECK(not std::is_copy_assignable_v<pnp::parser_input>);
    STATIC_CHECK(not std::is_move_constructible_v<pnp::parser_input>);
    STATIC_CHECK(not std::is_move_assignable_v<pnp::parser_input>);
    STATIC_CHECK(not std::is_default_constructible_v<pnp::parser_input>);

    // Built only on purpose: a string doesn't quietly become a parser_input.
    STATIC_CHECK(std::is_constructible_v<pnp::parser_input, std::u8string_view>);
    STATIC_CHECK(std::is_constructible_v<pnp::parser_input, std::string_view>);
    STATIC_CHECK(not std::is_convertible_v<std::u8string_view, pnp::parser_input>);
    STATIC_CHECK(not std::is_convertible_v<std::string_view, pnp::parser_input>);
    STATIC_CHECK(not std::is_convertible_v<pnp::string_source&, pnp::parser_input>);
}

TEST_CASE(
    "input_text: moving hands the text over",
    "[parser_input][types]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abc def"));
    auto              first = input.peek_chars(3).value();
    auto              moved = std::move(first);
    CHECK(first.get().empty());
    CHECK(moved.get() == u8"abc");

    // Assigning onto text that's already held replaces it.
    input.advance(4);
    auto second = input.peek_chars(3).value();
    moved = std::move(second);
    CHECK(second.get().empty());
    CHECK(moved.get() == u8"def");
    // Every hold is gone by the end: input's destructor checks.
}

// ============================================================ construction
// and settings

TEST_CASE(
    "parser_input: settings default from the options",
    "[parser_input][options]"
)
{
    const pnp::pnp_options& options = pnp::current_pnp_options();
    {
        pnp::parser_input input(std::u8string_view(u8"abc"));
        CHECK(input.context_chars() == options.parser_input_context_chars);
        CHECK(
            input.fraction_of_buffer_threshold()
            == options.parser_input_fraction_of_buffer_threshold
        );
    }
    {
        pnp::parser_input input(std::string_view("abc"));
        CHECK(input.context_chars() == options.parser_input_context_chars);
    }
    {
        pnp::string_source source(u8"abc");
        pnp::parser_input  input(source);
        CHECK(input.context_chars() == options.parser_input_context_chars);
        CHECK(
            input.fraction_of_buffer_threshold()
            == options.parser_input_fraction_of_buffer_threshold
        );
    }
}

TEST_CASE(
    "parser_input: settings given explicitly",
    "[parser_input][options]"
)
{
    {
        pnp::parser_input input(std::u8string_view(u8"abc"), 7);
        CHECK(input.context_chars() == 7);
    }
    {
        pnp::parser_input input(std::string_view("abc"), 0);
        CHECK(input.context_chars() == 0);
    }
    {
        repeating_source  source(u8"1 ", 10);
        pnp::parser_input input(source, 3, 0.25);
        CHECK(input.context_chars() == 3);
        CHECK(input.fraction_of_buffer_threshold() == 0.25);
    }
}

TEST_CASE(
    "parser_input: an out-of-range release fraction is treated as 0",
    "[parser_input][options]"
)
{
    repeating_source source(u8"1 ", 10);
    for (const double bad :
    {1.0,
        1.5,
        -0.1,
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity()})
    {
        pnp::parser_input input(source, 2, bad);
        CHECK(input.fraction_of_buffer_threshold() == 0.0);
    }
    // The edges of [0, 1) are kept.
    {
        pnp::parser_input input(source, 2, 0.0);
        CHECK(input.fraction_of_buffer_threshold() == 0.0);
    }
    {
        pnp::parser_input input(source, 2, 0.95);
        CHECK(input.fraction_of_buffer_threshold() == 0.95);
    }
    // Every one of those let the source go again.
    CHECK_FALSE(source.in_use());
}

TEST_CASE(
    "parser_input: usable during constant evaluation",
    "[parser_input][constexpr]"
)
{
    // If this doesn't compile, a unicode_bridge function it reaches isn't
    // constexpr yet.
    constexpr auto check = []()
        {
            pnp::parser_input input(std::u8string_view(u8"ab  cd"));
            const bool        matched = input.advance_if_starts_with(u8"ab");
            input.skip_whitespace();
            const bool cd = input.peek_chars(2).value().get() == u8"cd";
            return matched && cd && input.position() == 4;
        };
    STATIC_CHECK(check());
}

// ============================================================ parser_input
// over a string

TEST_CASE(
    "parser_input: over a fixed string",
    "[parser_input]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abc def"));
    CHECK(input.position() == 0);
    CHECK(input.starts_with(u8"abc"));
    CHECK(input.position() == 0); // looking doesn't move
    CHECK(input.advance_if_starts_with(u8"abc"));
    CHECK(not input.advance_if_starts_with(u8"xyz"));
    CHECK(input.position() == 3);

    input.skip_whitespace();
    auto chars_to_check = input.peek_chars(3);
    CHECK(chars_to_check.value().get() == u8"def");
    // There's never any more: just what there is.
    CHECK(input.peek_chars(4) == std::nullopt);

    input.advance(3);
    CHECK(input.at_end());
}

TEST_CASE(
    "parser_input: a narrow string is read as UTF-8",
    "[parser_input]"
)
{
    const std::string narrow = "caf\xC3\xA9"; // "café", as UTF-8 bytes
    pnp::parser_input input{ std::string_view(narrow) };
    CHECK(input.peek_chars(4).value().get() == u8"café");
}

TEST_CASE(
    "parser_input: reading characters",
    "[parser_input]"
)
{
    pnp::parser_input input(std::u8string_view(u8"é!"));
    const auto        first = input.peek_char();
    REQUIRE(first.has_value());
    CHECK(first->first == U'é');
    CHECK(first->second == 2); // é is two code units
    input.advance(first->second);
    CHECK(input.peek_char().value().first == U'!');
    input.advance(1);
    CHECK(not input.peek_char().has_value()); // the end: no character
}

TEST_CASE(
    "parser_input: starts_with a single token",
    "[parser_input]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abc"));
    CHECK(input.starts_with(u8"ab"));
    CHECK(not input.starts_with(u8"ac"));
    CHECK(not input.starts_with(u8"abcd")); // longer than the input
    CHECK(input.starts_with(u8""));         // empty always matches
    CHECK(input.position() == 0);

    // At the end, only the empty token matches.
    input.advance(3);
    CHECK(input.starts_with(u8""));
    CHECK(not input.starts_with(u8"a"));
}

TEST_CASE(
    "parser_input: starts_with a sequence of tokens",
    "[parser_input]"
)
{
    pnp::parser_input input(std::u8string_view(u8"ab"));
    CHECK(input.starts_with({ u8"a", u8"b" }));
    CHECK(not input.starts_with({ u8"a", u8"c" }));
    CHECK(not input.starts_with({ u8"a", u8"b", u8"c" })
    ); // longer than the input
    CHECK(input.starts_with({}));
    CHECK(input.starts_with({ u8"", u8"ab", u8"" })); // empty parts are fine
    CHECK(input.position() == 0);

    // The same, a byte at a time.
    pnp::testing::piece_source source(std::u8string(u8"ab"), 1);
    pnp::parser_input split(source);
    CHECK(split.starts_with({ u8"a", u8"b" }));
    CHECK(not split.starts_with({ u8"a", u8"c" }));
}

TEST_CASE(
    "parser_input: advance_if_starts_with",
    "[parser_input]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abc"));
    // An empty token matches without moving.
    CHECK(input.advance_if_starts_with(u8""));
    CHECK(input.position() == 0);
    // A mismatch doesn't move.
    CHECK(not input.advance_if_starts_with(u8"ax"));
    CHECK(input.position() == 0);
    // A token longer than what's left doesn't match, or move.
    CHECK(not input.advance_if_starts_with(u8"abcd"));
    CHECK(input.position() == 0);
    CHECK(input.advance_if_starts_with(u8"abc"));
    CHECK(input.at_end());
}

TEST_CASE(
    "parser_input: peek_while",
    "[parser_input]"
)
{
    // Stops before the first byte the predicate rejects, without moving.
    {
        pnp::parser_input input(std::u8string_view(u8"123x"));
        CHECK(input.peek_while(is_digit).get() == u8"123");
        CHECK(input.position() == 0);
    }
    // Reads to the end, if everything is accepted.
    {
        pnp::parser_input input(std::u8string_view(u8"123"));
        CHECK(input.peek_while(is_digit).get() == u8"123");
    }
    // Empty, if the first byte is rejected.
    {
        pnp::parser_input input(std::u8string_view(u8"x1"));
        CHECK(input.peek_while(is_digit).get().empty());
    }
    // Empty, at the end.
    {
        pnp::parser_input input(std::u8string_view(u8""));
        CHECK(input.peek_while(is_digit).get().empty());
    }
    // An ASCII predicate stops before a multi-byte character, at its start.
    {
        pnp::parser_input input(std::u8string_view(u8"12é"));
        CHECK(input.peek_while(is_digit).get() == u8"12");
    }
}

TEST_CASE(
    "parser_input: peek_while with a predicate that splits a character",
    "[parser_input][errors]"
)
{
    // Accepts é's first byte, rejects its second: the text would end inside é.
    const auto splits_e_acute = [](const char8_t c)
        {
            return c != static_cast<char8_t>(0xA9);
        };
    {
        pnp::parser_input input(std::u8string_view(u8"aé"));
        CHECK(
            thrown_code(
                [&]
                {
                    (void)input.peek_while(splits_e_acute);
                }
            )
            == error_code::invalid_parser_split_character
        );
    }
    // The same, with é arriving a byte at a time.
    {
        pnp::testing::piece_source source(std::u8string(u8"aé"), 1);
        pnp::parser_input input(source);
        CHECK(
            thrown_code(
                [&]
                {
                    (void)input.peek_while(splits_e_acute);
                }
            )
            == error_code::invalid_parser_split_character
        );
    }
}

TEST_CASE(
    "parser_input: peek_chars_while",
    "[parser_input]"
)
{
    // Whole characters, multi-byte ones included.
    {
        pnp::parser_input input(std::u8string_view(u8"ééx"));
        CHECK(
            input
            .peek_chars_while(
                [](const char32_t c)
                {
                    return c == U'é';
                }
            )
            .get()
            == u8"éé"
        );
        CHECK(input.position() == 0);
    }
    // The same, a byte at a time.
    {
        pnp::testing::piece_source source(std::u8string(u8"ééx"), 1);
        pnp::parser_input input(source);
        CHECK(
            input
            .peek_chars_while(
                [](const char32_t c)
                {
                    return c == U'é';
                }
            )
            .get()
            == u8"éé"
        );
    }
    // The predicate is called once per character, in order, including the one
    // it rejects.
    {
        pnp::parser_input input(std::u8string_view(u8"aéb!c"));
        std::u32string    seen;
        const auto        text = input.peek_chars_while(
            [&](const char32_t c)
            {
                seen.push_back(c);
                return c != U'!';
            }
        );
        CHECK(text.get() == u8"aéb");
        CHECK(seen == U"aéb!");
    }
    // Reads to the end, if everything is accepted.
    {
        pnp::parser_input input(std::u8string_view(u8"abc"));
        CHECK(
            input
            .peek_chars_while(
                [](const char32_t)
                {
                    return true;
                }
            )
            .get()
            == u8"abc"
        );
    }
    // Empty, if the first character is rejected.
    {
        pnp::parser_input input(std::u8string_view(u8"éa"));
        CHECK(
            input
            .peek_chars_while(
                [](const char32_t c)
                {
                    return c == U'a';
                }
            )
            .get()
            .empty()
        );
    }
}

TEST_CASE(
    "parser_input: peek_until",
    "[parser_input]"
)
{
    // Up to the token, without moving.
    {
        pnp::parser_input input(std::u8string_view(u8"abc\"def"));
        const auto        found = input.peek_until(u8"\"");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8"abc");
        CHECK(input.position() == 0);
    }
    // An escaped token is skipped.
    {
        pnp::parser_input input(std::u8string_view(u8R"(ab\"cd"rest)"));
        const auto        found = input.peek_until(u8"\"", u8"\\");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8R"(ab\"cd)");
    }
    // An escaped escape doesn't escape the token after it.
    {
        pnp::parser_input input(std::u8string_view(u8R"(ab\\"rest)"));
        const auto        found = input.peek_until(u8"\"", u8"\\");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8R"(ab\\)");
    }
    // The examples from the docs.
    {
        pnp::parser_input input(std::u8string_view(u8"hello#_x_"));
        const auto        found = input.peek_until(u8"_", u8"#");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8"hello#_x");
    }
    {
        pnp::parser_input input(std::u8string_view(u8"hello##_x_"));
        const auto        found = input.peek_until(u8"_", u8"#");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8"hello##");
    }
    // A multi-code-unit escape.
    {
        pnp::parser_input input(std::u8string_view(u8"a&&;b;c"));
        const auto        found = input.peek_until(u8";", u8"&&");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8"a&&;b");
    }
    // A longer token: a lone first character isn't it.
    {
        pnp::parser_input input(std::u8string_view(u8"a]b]]c"));
        const auto        found = input.peek_until(u8"]]");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8"a]b");
    }
    // An escape stops the token starting where it is, not one character later.
    {
        pnp::parser_input input(std::u8string_view(u8R"(a\]]])"));
        const auto        found = input.peek_until(u8"]]", u8"\\");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8R"(a\])");
    }
    // An escape covers a whole multi-byte character.
    {
        pnp::parser_input input(std::u8string_view(u8"x\\éyéz"));
        const auto        found = input.peek_until(u8"é", u8"\\");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8"x\\éy");
    }
    // A token at the very start: empty text.
    {
        pnp::parser_input input(std::u8string_view(u8"\"abc"));
        const auto        found = input.peek_until(u8"\"");
        REQUIRE(found.has_value());
        CHECK(found->get().empty());
    }
    // No token before the end.
    {
        pnp::parser_input input(std::u8string_view(u8"abc"));
        CHECK(input.peek_until(u8"\"") == std::nullopt);
    }
    // An escape with nothing after it, at the end.
    {
        pnp::parser_input input(std::u8string_view(u8"abc\\"));
        CHECK(input.peek_until(u8"\"", u8"\\") == std::nullopt);
    }
    // An empty token is found straight away.
    {
        pnp::parser_input input(std::u8string_view(u8"abc"));
        const auto        found = input.peek_until(u8"");
        REQUIRE(found.has_value());
        CHECK(found->get().empty());
    }
}

TEST_CASE(
    "parser_input: peek_until, partway through the input",
    "[parser_input]"
)
{
    // The position found is relative to wherever the search starts.
    pnp::parser_input input(std::u8string_view(u8"skip:ab\"cd"));
    input.advance(5);
    const auto found = input.peek_until(u8"\"");
    REQUIRE(found.has_value());
    CHECK(found->get() == u8"ab");
    CHECK(input.position() == 5);
}

TEST_CASE(
    "parser_input: peek_until, with the text arriving a byte at a time",
    "[parser_input][io]"
)
{
    // Escapes and tokens split across pieces.
    {
        pnp::testing::piece_source source(std::u8string(u8R"(ab\"c]]d)"), 1);
        pnp::parser_input input(source);
        const auto        found = input.peek_until(u8"]]", u8"\\");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8R"(ab\"c)");
    }
    {
        pnp::testing::piece_source source(std::u8string(u8R"(a\]]])"), 1);
        pnp::parser_input input(source);
        const auto        found = input.peek_until(u8"]]", u8"\\");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8R"(a\])");
    }
    {
        pnp::testing::piece_source source(std::u8string(u8"a&&;b;c"), 1);
        pnp::parser_input input(source);
        const auto        found = input.peek_until(u8";", u8"&&");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8"a&&;b");
    }
    {
        pnp::testing::piece_source source(std::u8string(u8"ab]"), 1);
        pnp::parser_input input(source);
        CHECK(input.peek_until(u8"]]") == std::nullopt);
    }
}

TEST_CASE(
    "parser_input: peek_until's errors",
    "[parser_input][errors]"
)
{
    // The escape and the token can't start with the same code unit.
    {
        pnp::parser_input input(std::u8string_view(u8"abc"));
        CHECK(
            thrown_code(
                [&]()
                {
                    (void)input.peek_until(u8"\"", u8"\"x");
                }
            )
            == error_code::invalid_parser_escape_matches_token
        );
    }
    // Invalid UTF-8 before the token is reported, not skipped.
    {
        const std::u8string text = u8"a" + byte(0x80) + u8"\"";
        pnp::parser_input   input{ std::u8string_view(text) };
        CHECK(
            thrown_code(
                [&]
                {
                    (void)input.peek_until(u8"\"");
                }
            )
            == error_code::invalid_utf8
        );
    }
}

TEST_CASE(
    "parser_input: peek_chars",
    "[parser_input]"
)
{
    pnp::parser_input input(std::u8string_view(u8"é€x"));
    // Zero characters is empty text, not a failure.
    const auto none = input.peek_chars(0);
    REQUIRE(none.has_value());
    CHECK(none->get().empty());
    // Counts characters, not code units.
    CHECK(input.peek_chars(1).value().get() == u8"é");
    CHECK(input.peek_chars(2).value().get() == u8"é€");
    CHECK(input.peek_chars(3).value().get() == u8"é€x");
    CHECK(input.peek_chars(4) == std::nullopt);
    CHECK(input.position() == 0);
}

TEST_CASE(
    "parser_input: advance",
    "[parser_input]"
)
{
    pnp::parser_input input(std::u8string_view(u8"é!"));
    input.advance(0);
    CHECK(input.position() == 0);
    // Into the middle of é: the parser's mistake, and nothing moves.
    CHECK(
        thrown_code(
            [&]
            {
                input.advance(1);
            }
        )
        == error_code::invalid_parser_advanced_into_character
    );
    CHECK(input.position() == 0);
    input.advance(2);
    CHECK(input.position() == 2);
    // Exactly to the end is fine.
    input.advance(1);
    CHECK(input.at_end());
    input.advance(0);
    CHECK(input.position() == 3);
}

TEST_CASE(
    "parser_input: skip_whitespace",
    "[parser_input]"
)
{
    // Nothing to skip: nothing moves.
    {
        pnp::parser_input input(std::u8string_view(u8"x "));
        input.skip_whitespace();
        CHECK(input.position() == 0);
    }
    // All whitespace: to the end.
    {
        pnp::parser_input input(std::u8string_view(u8" \t\r\n"));
        input.skip_whitespace();
        CHECK(input.at_end());
    }
    // Already at the end.
    {
        pnp::parser_input input(std::u8string_view(u8""));
        CHECK_NOTHROW(input.skip_whitespace());
        CHECK(input.at_end());
    }
    // Non-ASCII whitespace is skipped too.
    {
        pnp::parser_input input(std::u8string_view(u8" \u3000x"));
        input.skip_whitespace();
        CHECK(input.position() == 4);
        CHECK(input.starts_with(u8"x"));
    }
    // Stops at the start of a multi-byte character that isn't whitespace.
    {
        pnp::parser_input input(std::u8string_view(u8"  é"));
        input.skip_whitespace();
        CHECK(input.position() == 2);
        CHECK(input.peek_char().value().first == U'é');
    }
}

// ============================================================ advancing past
// text without looking at it

TEST_CASE(
    "parser_input: advance_while",
    "[parser_input][advance]"
)
{
    // Moves past what's accepted, and says how far.
    {
        pnp::parser_input input(std::u8string_view(u8"123x"));
        CHECK(input.advance_while(is_digit) == 3);
        CHECK(input.position() == 3);
        CHECK(input.starts_with(u8"x"));
    }
    // Nothing accepted: 0, and nothing moves.
    {
        pnp::parser_input input(std::u8string_view(u8"x1"));
        CHECK(input.advance_while(is_digit) == 0);
        CHECK(input.position() == 0);
    }
    // Everything accepted: to the end.
    {
        pnp::parser_input input(std::u8string_view(u8"123"));
        CHECK(input.advance_while(is_digit) == 3);
        CHECK(input.at_end());
    }
    // An ASCII predicate stops at the start of a multi-byte character.
    {
        pnp::parser_input input(std::u8string_view(u8"12é"));
        CHECK(input.advance_while(is_digit) == 2);
        CHECK(input.peek_char().value().first == U'é');
    }
    // A line comment: skip to the newline.
    {
        pnp::parser_input input(std::u8string_view(u8"# comment é\nnext"));
        CHECK(input.advance_if_starts_with(u8"#"));
        (void)input.advance_while(
            [](const char8_t c)
            {
                return c != u8'\n';
            }
        );
        CHECK(input.advance_if_starts_with(u8"\nnext"));
    }
    // The same, a byte at a time.
    {
        pnp::testing::piece_source source(std::u8string(u8"123x"), 1);
        pnp::parser_input input(source);
        CHECK(input.advance_while(is_digit) == 3);
        CHECK(input.starts_with(u8"x"));
    }
}

TEST_CASE(
    "parser_input: advance_while's errors leave the position alone",
    "[parser_input][advance][errors]"
)
{
    const auto splits_e_acute = [](const char8_t c)
        {
            return c != static_cast<char8_t>(0xA9);
        };
    {
        pnp::parser_input input(std::u8string_view(u8"aé"));
        CHECK(
            thrown_code(
                [&]
                {
                    (void)input.advance_while(splits_e_acute);
                }
            )
            == error_code::invalid_parser_split_character
        );
        CHECK(input.position() == 0);
    }
    {
        const std::u8string text = u8"12" + byte(0x80);
        pnp::parser_input   input{ std::u8string_view(text) };
        CHECK(
            thrown_code(
                [&]
                {
                    (void)input.advance_while(
                        [](const char8_t)
                        {
                            return true;
                        }
                    );
                }
            )
            == error_code::invalid_utf8
        );
        CHECK(input.position() == 0);
    }
}

TEST_CASE(
    "parser_input: advance_chars_while",
    "[parser_input][advance]"
)
{
    const auto is_e_acute = [](const char32_t c)
        {
            return c == U'é';
        };
    // Counts code units, not characters.
    {
        pnp::parser_input input(std::u8string_view(u8"ééx"));
        CHECK(input.advance_chars_while(is_e_acute) == 4);
        CHECK(input.position() == 4);
        CHECK(input.starts_with(u8"x"));
    }
    // Nothing accepted: 0, and nothing moves.
    {
        pnp::parser_input input(std::u8string_view(u8"xé"));
        CHECK(input.advance_chars_while(is_e_acute) == 0);
        CHECK(input.position() == 0);
    }
    // Everything accepted: to the end.
    {
        pnp::parser_input input(std::u8string_view(u8"éé"));
        CHECK(input.advance_chars_while(is_e_acute) == 4);
        CHECK(input.at_end());
    }
    // Once per character, in order, including the one it rejects.
    {
        pnp::parser_input input(std::u8string_view(u8"aéb!c"));
        std::u32string    seen;
        CHECK(
            input.advance_chars_while(
                [&](const char32_t c)
                {
                    seen.push_back(c);
                    return c != U'!';
                }
            )
            == 4
        );
        CHECK(seen == U"aéb!");
    }
    // Horizontal whitespace only, keeping the newline.
    {
        pnp::parser_input input(std::u8string_view(u8" \t \nx"));
        (void)input.advance_chars_while(
            [](const char32_t c)
            {
                return c == U' ' || c == U'\t';
            }
        );
        CHECK(input.starts_with(u8"\n"));
    }
    // The same, a byte at a time.
    {
        pnp::testing::piece_source source(std::u8string(u8"ééx"), 1);
        pnp::parser_input input(source);
        CHECK(input.advance_chars_while(is_e_acute) == 4);
        CHECK(input.starts_with(u8"x"));
    }
}

TEST_CASE(
    "parser_input: advance_chars_while's errors leave the position alone",
    "[parser_input][advance][errors]"
)
{
    const std::u8string text = u8"ab" + byte(0xFF);
    pnp::parser_input   input{ std::u8string_view(text) };
    CHECK(
        thrown_code(
            [&]
            {
                (void)input.advance_chars_while(
                    [](const char32_t)
                    {
                        return true;
                    }
                );
            }
        )
        == error_code::invalid_utf8
    );
    CHECK(input.position() == 0);
}

TEST_CASE(
    "parser_input: advance_until",
    "[parser_input][advance]"
)
{
    // To the token, not past it, and says how far.
    {
        pnp::parser_input input(std::u8string_view(u8"abc\"def"));
        const auto        moved = input.advance_until(u8"\"");
        REQUIRE(moved.has_value());
        CHECK(*moved == 3);
        CHECK(input.position() == 3);
        CHECK(input.starts_with(u8"\""));
    }
    // The examples from the docs.
    {
        pnp::parser_input input(std::u8string_view(u8"hello#_x_"));
        CHECK(input.advance_until(u8"_", u8"#") == 8);
        CHECK(input.position() == 8);
    }
    {
        pnp::parser_input input(std::u8string_view(u8"hello##_x_"));
        CHECK(input.advance_until(u8"_", u8"#") == 7);
        CHECK(input.position() == 7);
    }
    // Partway through: the count is from where it started.
    {
        pnp::parser_input input(std::u8string_view(u8"skip:ab\"cd"));
        input.advance(5);
        CHECK(input.advance_until(u8"\"") == 2);
        CHECK(input.position() == 7);
    }
    // A block comment, skipped.
    {
        pnp::parser_input input(std::u8string_view(u8"/* a * b */rest"));
        CHECK(input.advance_if_starts_with(u8"/*"));
        REQUIRE(input.advance_until(u8"*/").has_value());
        CHECK(input.advance_if_starts_with(u8"*/"));
        CHECK(input.starts_with(u8"rest"));
    }
    // An empty token: 0, and nothing moves.
    {
        pnp::parser_input input(std::u8string_view(u8"abc"));
        CHECK(input.advance_until(u8"") == 0);
        CHECK(input.position() == 0);
    }
    // Not found: std::nullopt, and nothing moves.
    {
        pnp::parser_input input(std::u8string_view(u8"abc"));
        input.advance(1);
        CHECK(input.advance_until(u8"\"") == std::nullopt);
        CHECK(input.position() == 1);
    }
    // The same, a byte at a time.
    {
        pnp::testing::piece_source source(std::u8string(u8R"(ab\"c]]d)"), 1);
        pnp::parser_input input(source);
        CHECK(input.advance_until(u8"]]", u8"\\") == 5);
        CHECK(input.starts_with(u8"]]d"));
    }
    {
        pnp::testing::piece_source source(std::u8string(u8"ab]"), 1);
        pnp::parser_input input(source);
        CHECK(input.advance_until(u8"]]") == std::nullopt);
        CHECK(input.position() == 0);
    }
}

TEST_CASE(
    "parser_input: advance_until's errors leave the position alone",
    "[parser_input][advance][errors]"
)
{
    {
        pnp::parser_input input(std::u8string_view(u8"abc"));
        CHECK(
            thrown_code(
                [&]
                {
                    (void)input.advance_until(u8"\"", u8"\"x");
                }
            )
            == error_code::invalid_parser_escape_matches_token
        );
        CHECK(input.position() == 0);
    }
    {
        const std::u8string text = u8"a" + byte(0x80) + u8"\"";
        pnp::parser_input   input{ std::u8string_view(text) };
        CHECK(
            thrown_code(
                [&]
                {
                    (void)input.advance_until(u8"\"");
                }
            )
            == error_code::invalid_utf8
        );
        CHECK(input.position() == 0);
    }
}

// ============================================================ fail, location
// and context

TEST_CASE(
    "parser_input: fail, location_at and get_context on a fixed string",
    "[parser_input]"
)
{
    {
        pnp::parser_input input(std::u8string_view(u8"ab\ncd"));
        input.advance(4);
        CHECK(input.fail(u8"here").position() == 4);
        CHECK(input.fail(u8"there", 1).position() == 1);
        CHECK(input.position() == 4); // failing doesn't move
        CHECK(input.location_at(0).line == 1);
        CHECK(input.location_at(0).column == 1);
        CHECK(input.location_at(4).line == 2);
        CHECK(input.location_at(4).column == 2);
        CHECK(input.location().line == 2);
        CHECK(input.location().column == 2);
        // Past the end: the end.
        CHECK(input.location_at(100).line == 2);
        CHECK(input.location_at(100).column == 3);
    }
    {
        pnp::parser_input input(std::u8string_view(u8"abcdefg"), 2);
        const auto        middle = input.get_context(3);
        CHECK(middle.before_pos_text == u8"bc");
        CHECK(middle.current_pos_text == u8"d");
        CHECK(middle.after_pos_text == u8"ef");
        CHECK(middle.before_text_cut_off);
        CHECK(middle.after_text_cut_off);

        const auto start = input.get_context(0);
        CHECK(start.before_pos_text.empty());
        CHECK(not start.before_text_cut_off);

        const auto end = input.get_context(7);
        CHECK(end.current_pos_text.empty());
        CHECK(end.before_pos_text == u8"fg");
        CHECK(end.after_pos_text.empty());
        CHECK(not end.after_text_cut_off);
    }
    // get_context() is get_context at the current position.
    {
        pnp::parser_input input(std::u8string_view(u8"abcdefg"), 2);
        input.advance(3);
        const auto here = input.get_context();
        const auto at_3 = input.get_context(3);
        CHECK(here.before_pos_text == at_3.before_pos_text);
        CHECK(here.current_pos_text == at_3.current_pos_text);
        CHECK(here.after_pos_text == at_3.after_pos_text);
    }
    // A context_chars of 0 shows only the focus.
    {
        pnp::parser_input input(std::u8string_view(u8"abc"), 0);
        const auto        context = input.get_context(1);
        CHECK(context.before_pos_text.empty());
        CHECK(context.current_pos_text == u8"b");
        CHECK(context.after_pos_text.empty());
    }
}

// ============================================================ the errors it
// throws

TEST_CASE(
    "parser_input: moving past what's available is the parser's mistake",
    "[parser_input][errors]"
)
{
    pnp::parser_input input(std::u8string_view(u8"ab"));
    CHECK(
        thrown_code(
            [&]
            {
                input.advance(3);
            }
        )
        == error_code::invalid_parser_advanced_past_end
    );
    CHECK(input.position() == 0); // nothing moved
}

TEST_CASE(
    "parser_input: invalid UTF-8 is the input's fault",
    "[parser_input][errors]"
)
{
    const std::u8string stray = u8"a" + byte(0x80) + u8"b";
    {
        pnp::parser_input input{ std::u8string_view(stray) };
        CHECK_NOTHROW(input.advance(1));
        CHECK(
            thrown_code(
                [&]
                {
                    input.advance(1);
                }
            )
            == error_code::invalid_utf8
        );
        CHECK(
            thrown_position(
                [&]
                {
                    input.advance(1);
                }
            )
            == 1
        );
    }

    // Decoding a character that isn't valid, at its position.
    {
        pnp::parser_input input{ std::u8string_view(stray) };
        CHECK(input.advance_if_starts_with(u8"a"));
        CHECK(
            thrown_code(
                [&]
                {
                    (void)input.peek_char();
                }
            )
            == error_code::invalid_utf8
        );
        CHECK(
            thrown_position(
                [&]
                {
                    (void)input.peek_char();
                }
            )
            == 1
        );
    }

    // Skipping whitespace into invalid input reports it, rather than stopping
    // silently.
    const std::u8string spaced = u8"  " + byte(0xFF);
    {
        pnp::parser_input input{ std::u8string_view(spaced) };
        CHECK(
            thrown_code(
                [&]
                {
                    input.skip_whitespace();
                }
            )
            == error_code::invalid_utf8
        );
    }

    // A mismatch before the invalid text decides without reading it.
    {
        pnp::parser_input input{ std::u8string_view(stray) };
        CHECK_NOTHROW((void)input.starts_with(u8"x"));
        CHECK(not input.starts_with(u8"x"));
    }
}

TEST_CASE(
    "parser_input: at_end never checks validity",
    "[parser_input][errors]"
)
{
    const std::u8string stray = u8"a" + byte(0x80);
    pnp::parser_input   input{ std::u8string_view(stray) };
    input.advance(1);
    CHECK_NOTHROW((void)input.at_end());
    CHECK(not input.at_end()); // a byte is there, even if it's invalid
}

TEST_CASE(
    "parser_input: invalid UTF-8 from a source",
    "[parser_input][errors][io]"
)
{
    // The input ending partway through a character counts as invalid.
    {
        pnp::testing::piece_source source(u8"a" + byte(0xC3), 1);
        pnp::parser_input input(source);
        CHECK(input.advance_if_starts_with(u8"a"));
        CHECK(
            thrown_code(
                [&]
                {
                    (void)input.peek_char();
                }
            )
            == error_code::invalid_utf8
        );
        CHECK(
            thrown_position(
                [&]
                {
                    (void)input.peek_char();
                }
            )
            == 1
        );
    }
    // Once a source has found invalid text, nothing more is read: reading the
    // rest first would only delay the error.
    {
        trap_source       source({ u8"a" + byte(0x80) });
        pnp::parser_input input(source);
        CHECK(input.advance_if_starts_with(u8"a"));
        CHECK(
            thrown_code(
                [&]
                {
                    (void)input.peek_char();
                }
            )
            == error_code::invalid_utf8
        );
    }
}

// ============================================================ sources

TEST_CASE(
    "string_view_source: viewing text without copying it",
    "[io][sources]"
)
{
    const std::u8string     text = u8"skip:value";
    pnp::string_view_source source(text);
    CHECK(source.buffered().data() == text.data()); // the same text, not a copy
    CHECK(not source.read_more());

    source.release_up_to(5);
    CHECK(source.buffered() == u8"value");
    CHECK(source.offset() == 5);
}

TEST_CASE(
    "string_source: owning text handed over",
    "[io][sources]"
)
{
    std::u8string      text = u8"skip:value";
    pnp::string_source source(std::move(text));
    CHECK(source.buffered() == u8"skip:value");
    CHECK(not source.read_more());
    source.release_up_to(5);
    CHECK(source.buffered() == u8"value");
    CHECK(source.offset() == 5);
}

TEST_CASE(
    "piece_source: a piece at a time",
    "[io][sources]"
)
{
    pnp::testing::piece_source source(std::u8string(u8"abc"), 1);
    CHECK(source.buffered().empty());
    CHECK(source.read_more());
    CHECK(source.buffered() == u8"a");
    CHECK(source.read_more());
    CHECK(source.read_more());
    CHECK(source.buffered() == u8"abc");
    CHECK(not source.read_more());
    CHECK(not source.read_more()); // and stays finished

    CHECK_THROWS_AS(
        pnp::testing::piece_source(std::u8string(u8"x"), 0), std::invalid_argument
    );
}

TEST_CASE(
    "buffered_source: every successful read makes progress",
    "[io][sources]"
)
{
    stalling_source source;
    CHECK(source.read_more()); // asks past the two empty reads
    CHECK(source.buffered() == u8"ok");
    CHECK(not source.read_more());
}

// ============================================================ parser_input
// over a source

TEST_CASE(
    "parser_input: reading more from a source, one byte at a time",
    "[parser_input][io]"
)
{
    // Tokens split across pieces still match; a mismatch stays put.
    {
        pnp::testing::piece_source source(std::u8string(u8"nullptr,rest"), 1);
        pnp::parser_input input(source);
        CHECK(input.advance_if_starts_with(u8"nullptr"));
        CHECK(input.advance_if_starts_with(u8","));
        CHECK(not input.advance_if_starts_with(u8"other"));
        CHECK(input.position() == 8);
    }

    // A token longer than the whole input fails, rather than waiting forever.
    {
        pnp::testing::piece_source source(std::u8string(u8"nul"), 1);
        pnp::parser_input input(source);
        CHECK(not input.advance_if_starts_with(u8"nullptr"));
        CHECK(input.position() == 0);
    }

    // A multi-byte character split across pieces is read whole.
    {
        // A three-byte space.
        pnp::testing::piece_source source(std::u8string(u8"\u3000x"), 1);
        pnp::parser_input input(source);
        const auto        space = input.peek_char();
        REQUIRE(space.has_value());
        CHECK(space->first == U'\u3000');
        CHECK(space->second == 3);
        input.skip_whitespace();
        CHECK(input.advance_if_starts_with(u8"x"));
        CHECK(input.at_end());
    }

    // peek_chars across pieces counts whole characters.
    {
        pnp::testing::piece_source source(std::u8string(u8"é€x"), 1);
        pnp::parser_input input(source);
        CHECK(input.peek_chars(2).value().get() == u8"é€");
        CHECK(input.peek_chars(4) == std::nullopt);
    }

    // advance reads what it needs.
    {
        pnp::testing::piece_source source(std::u8string(u8"é!x"), 1);
        pnp::parser_input input(source);
        input.advance(3);
        CHECK(input.starts_with(u8"x"));
    }

    // The end is only the end once the source has nothing left.
    {
        pnp::testing::piece_source source(std::u8string(u8"ab"), 1);
        pnp::parser_input input(source);
        CHECK(not input.at_end());
        CHECK(input.advance_if_starts_with(u8"ab"));
        CHECK(input.at_end());
    }

    // Positions count from the start of the whole input, after earlier text was
    // discarded.
    {
        pnp::string_source source(std::u8string(u8"skip:value"));
        source.release_up_to(5);
        pnp::parser_input input(source);
        CHECK(input.position() == 5);
        CHECK(input.advance_if_starts_with(u8"value"));
        CHECK(input.position() == 10);
    }
}

TEST_CASE(
    "parser_input: reads no further than it needs to decide",
    "[parser_input][io]"
)
{
    // Each source below holds exactly what the answer needs; any further read
    // throws. On std::cin, a further read would wait for the user.

    // A mismatch decides at once, however long the token.
    {
        trap_source       source({ u8"1" });
        pnp::parser_input input(source);
        CHECK(not input.starts_with(u8"null"));
    }
    // A token ending where the input does needs nothing after it.
    {
        trap_source       source({ u8"nu", u8"ll" });
        pnp::parser_input input(source);
        CHECK(input.advance_if_starts_with(u8"null"));
    }
    {
        trap_source       source({ u8"a", u8"b" });
        pnp::parser_input input(source);
        CHECK(input.starts_with({ u8"a", u8"b" }));
    }
    {
        trap_source       source({ u8"12", u8" " });
        pnp::parser_input input(source);
        CHECK(input.peek_while(is_digit).get() == u8"12");
        CHECK(
            input
            .peek_chars_while(
                [](const char32_t c)
                {
                    return c >= U'0' && c <= U'9';
                }
            )
            .get()
            == u8"12"
        );
    }
    {
        trap_source       source({ u8"12", u8" " });
        pnp::parser_input input(source);
        CHECK(input.advance_while(is_digit) == 2);
    }
    {
        trap_source       source({ u8"12", u8" " });
        pnp::parser_input input(source);
        CHECK(
            input.advance_chars_while(
                [](const char32_t c)
                {
                    return c >= U'0' && c <= U'9';
                }
            )
            == 2
        );
    }
    {
        trap_source       source({ u8"ab", u8"\"c" });
        pnp::parser_input input(source);
        const auto        found = input.peek_until(u8"\"");
        REQUIRE(found.has_value());
        CHECK(found->get() == u8"ab");
    }
    {
        trap_source       source({ u8"ab", u8"\"c" });
        pnp::parser_input input(source);
        CHECK(input.advance_until(u8"\"") == 2);
    }
    // A character split across pieces needs its own bytes, and nothing more.
    {
        trap_source       source({ byte(0xC3), byte(0xA9) });
        pnp::parser_input input(source);
        CHECK(input.peek_char().value().first == U'é');
    }
    {
        trap_source       source({ u8"a", u8"b" });
        pnp::parser_input input(source);
        CHECK(input.peek_chars(2).value().get() == u8"ab");
    }
    {
        trap_source       source({ u8"ab" });
        pnp::parser_input input(source);
        input.advance(2);
        CHECK(input.position() == 2);
    }
    {
        trap_source       source({ u8"  ", u8" x" });
        pnp::parser_input input(source);
        input.skip_whitespace();
        CHECK(input.position() == 3);
    }
    // at_end only reads when nothing is waiting.
    {
        trap_source       source({ u8"a" });
        pnp::parser_input input(source);
        CHECK(not input.at_end()); // reads "a"
        CHECK(not input.at_end()); // "a" is waiting: no read
    }
    // get_context never reads from a source that may wait: error reporting
    // mustn't hang.
    {
        trap_source       source({ u8"ab" });
        pnp::parser_input input(source);
        CHECK(input.peek_char().value().first == U'a');
        CHECK_NOTHROW((void)input.get_context());
    }
}

// ============================================================ constructing
// over a source

TEST_CASE(
    "A source has one reader at a time",
    "[parser_input][source]"
)
{
    pnp::string_source source(u8"abc def");
    {
        pnp::parser_input input(source);
        CHECK(source.in_use());
        CHECK_THROWS_AS(source.release_up_to(1), std::logic_error);
        CHECK_THROWS_AS(pnp::parser_input(source), std::logic_error);
        // A failed second reader leaves the first in charge.
        CHECK(source.in_use());
    }
    CHECK_FALSE(source.in_use());
    CHECK_NOTHROW(source.release_up_to(4));
    CHECK(source.offset() == 4);
    {
        // Starts where the source now starts.
        pnp::parser_input input(source);
        CHECK(input.position() == 4);
        CHECK(input.starts_with(u8"def"));
    }
}

// ============================================================ marks

TEST_CASE(
    "Marks: going back reads nothing again",
    "[parser_input][marks]"
)
{
    counting_source   source({ u8"ab", u8"cd", u8"ef" });
    pnp::parser_input input(source);
    const auto        start = input.mark();
    CHECK(input.advance_if_starts_with(u8"abc"));
    CHECK(source.given == 2);
    input.reset(start);
    CHECK(input.position() == 0);
    CHECK(input.starts_with(u8"abcd"));
    CHECK(source.given == 2); // all of it was already read
}

TEST_CASE(
    "Marks: report a failure where something started",
    "[parser_input][marks]"
)
{
    pnp::parser_input input(std::u8string_view(u8"  [1, x]"));
    input.skip_whitespace();
    const auto start = input.mark();
    CHECK(input.advance_if_starts_with(u8"[1, "));
    // Go back to where the list started, and fail there.
    input.reset(start);
    const auto failure = input.fail(u8"bad list");
    CHECK(failure.position() == 2);
}

TEST_CASE(
    "Marks: going forward again, and nesting",
    "[parser_input][marks]"
)
{
    pnp::parser_input input(std::u8string_view(u8"one two three"));
    const auto        first = input.mark();
    input.advance(4);
    const auto second = input.mark();
    input.advance(4);
    input.reset(first);
    CHECK(input.starts_with(u8"one"));
    input.reset(second);
    CHECK(input.starts_with(u8"two"));
}

TEST_CASE(
    "Marks: destroyed in any order, or moved",
    "[parser_input][marks]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abcdef"));
    {
        std::optional<pnp::input_mark> earlier(input.mark());
        input.advance(2);
        const auto later = input.mark();
        // The earlier one goes first: out of order.
        earlier.reset();
        input.reset(later);
        CHECK(input.position() == 2);
    }
    {
        auto moved_from = input.mark();
        auto moved_to = std::move(moved_from);
        CHECK(
            thrown_code(
                [&]()
                {
                    input.reset(moved_from);
                }
            )
            == error_code::invalid_parser_foreign_mark
        );
        CHECK_NOTHROW(input.reset(moved_to));
    }
    // Every mark is gone: input's destructor checks no holds are left.
}

TEST_CASE(
    "Marks: move assignment gives up the old position",
    "[parser_input][marks][release]"
)
{
    repeating_source  source(u8"12345 ", 10'000);
    pnp::parser_input input(source);
    {
        auto held = input.mark(); // holds 0
        for (int i = 0; i < 2'000; ++i)
        {
            (void)read_one(input, source);
        }
        CHECK(source.offset() == 0); // 0 is held
        const std::size_t later_position = input.position();
        auto              later = input.mark();
        // held now holds later_position; 0 is no longer held.
        held = std::move(later);
        (void)read_one(input, source);
        CHECK(source.offset() > 0);
        input.reset(held);
        CHECK(input.position() == later_position);
        // Assigning a moved-from mark leaves nothing held.
        auto empty = std::move(held);
        held = std::move(later); // later was moved from: holds nothing
        CHECK(
            thrown_code(
                [&]
                {
                    input.reset(held);
                }
            )
            == error_code::invalid_parser_foreign_mark
        );
        CHECK_NOTHROW(input.reset(empty));
    }
    // Every mark is gone: input's destructor checks no holds are left.
}

TEST_CASE(
    "Marks: only for the parser_input that made them",
    "[parser_input][marks]"
)
{
    pnp::parser_input a(std::u8string_view(u8"abc"));
    pnp::parser_input b(std::u8string_view(u8"abc"));
    const auto        mark_a = a.mark();
    CHECK(
        thrown_code(
            [&]()
            {
                b.reset(mark_a);
            }
        )
        == error_code::invalid_parser_foreign_mark
    );
}

// ============================================================ releasing text

TEST_CASE(
    "release: a long input needs memory for only a little of it",
    "[parser_input][release]"
)
{
    repeating_source  source(u8"12345 ", 100'000); // 600 KB in all
    pnp::parser_input input(source);
    std::size_t       largest = 0;
    for (int i = 0; i < 100'000; ++i)
    {
        largest = std::max(largest, read_one(input, source));
    }
    CHECK(largest < 16 * 1'024);
    CHECK(input.position() == 600'000);
}

TEST_CASE(
    "release: does nothing on a fixed string",
    "[parser_input][release]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abc"));
    input.advance(2);
    input.release_consumed();
    CHECK(input.position() == 2);
    CHECK(input.starts_with(u8"c"));
}

TEST_CASE(
    "release: nothing a mark holds is released, until it's gone",
    "[parser_input][release]"
)
{
    repeating_source  source(u8"12345 ", 10'000);
    pnp::parser_input input(source);
    {
        const auto start = input.mark();
        for (int i = 0; i < 2'000; ++i)
        {
            (void)read_one(input, source);
        }
        // Everything since the mark is still here.
        CHECK(source.offset() == 0);
        input.reset(start);
        CHECK(input.position() == 0);
        CHECK(input.starts_with(u8"12345 12345"));
        input.advance(12'000);
    }
    // The mark is gone, so the text before the position can go.
    input.release_consumed();
    CHECK(source.offset() > 0);
    CHECK(source.buffered().size() < 4'096);
}

TEST_CASE(
    "release: after going back to a mark, nothing ahead is released",
    "[parser_input][release]"
)
{
    repeating_source  source(u8"12345 ", 10'000);
    pnp::parser_input input(source);
    {
        const auto start = input.mark();
        // Each of these asks to release up to where it's reached; the mark
        // blocks every one.
        for (int i = 0; i < 2'000; ++i)
        {
            (void)read_one(input, source);
        }
        input.reset(start);
    } // the mark is gone, and the position is back at 0
    // Releasing now must stop at the position, not carry out the earlier
    // requests: the text after it hasn't been parsed yet.
    for (int i = 0; i < 10; ++i)
    {
        (void)read_one(input, source);
    }
    CHECK(input.position() == 60);
    CHECK(source.offset() == 0);
    CHECK(input.starts_with(u8"12345 12345"));
}

TEST_CASE(
    "release: an input_text doesn't stop release, and its text stays valid",
    "[parser_input][release][pins]"
)
{
    // Reading on moves the buffer many times, and releases what's been
    // consumed; first's storage must be kept regardless. Run under
    // AddressSanitizer: a dangling view may otherwise still read correctly.
    {
        repeating_source  source(u8"12345 ", 10'000);
        pnp::parser_input input(source);
        const auto        first = input.peek_while(is_digit);
        input.advance(first.get().size());
        input.skip_whitespace();
        for (int i = 0; i < 5'000; ++i)
        {
            (void)read_one(input, source);
        }
        CHECK(source.offset() > 0); // text really was released
        CHECK(first.get() == u8"12345");
    }
    // Several, taken from different points, all stay valid.
    {
        repeating_source  source(u8"12345 ", 10'000);
        pnp::parser_input input(source);
        const auto        first = input.peek_while(is_digit);
        for (int i = 0; i < 2'000; ++i)
        {
            (void)read_one(input, source);
        }
        const auto second = input.peek_chars(6).value();
        for (int i = 0; i < 2'000; ++i)
        {
            (void)read_one(input, source);
        }
        const auto third = input.peek_until(u8" ").value();
        for (int i = 0; i < 2'000; ++i)
        {
            (void)read_one(input, source);
        }
        CHECK(source.offset() > 0);
        CHECK(first.get() == u8"12345");
        CHECK(second.get() == u8"12345 ");
        CHECK(third.get() == u8"12345");
    }
    // Moving an input_text moves its pin with it.
    {
        repeating_source  source(u8"12345 ", 10'000);
        pnp::parser_input input(source);
        auto              first = input.peek_while(is_digit);
        input.advance(first.get().size());
        input.skip_whitespace();
        const auto moved = std::move(first);
        for (int i = 0; i < 5'000; ++i)
        {
            (void)read_one(input, source);
        }
        CHECK(source.offset() > 0);
        CHECK(first.get().empty());
        CHECK(moved.get() == u8"12345");
    }
    // Assigning over an input_text gives up its old pin, and keeps the new.
    {
        repeating_source  source(u8"12345 ", 10'000);
        pnp::parser_input input(source);
        auto              held = input.peek_while(is_digit);
        for (int i = 0; i < 2'000; ++i)
        {
            (void)read_one(input, source);
        }
        held = input.peek_chars(3).value();
        for (int i = 0; i < 2'000; ++i)
        {
            (void)read_one(input, source);
        }
        CHECK(held.get() == u8"123");
    }
    // Destroyed while reading goes on: nothing is left pinned.
    {
        repeating_source  source(u8"12345 ", 10'000);
        pnp::parser_input input(source);
        {
            const auto first = input.peek_while(is_digit);
            for (int i = 0; i < 2'000; ++i)
            {
                (void)read_one(input, source);
            }
            CHECK(first.get() == u8"12345");
        }
        for (int i = 0; i < 2'000; ++i)
        {
            (void)read_one(input, source);
        }
        CHECK(source.buffered().size() < 16 * 1'024);
    }
}

TEST_CASE(
    "release: lines, columns and context survive",
    "[parser_input][release]"
)
{
    repeating_source  source(u8"12345\n", 10'000);
    pnp::parser_input input(source, 10);
    for (int i = 0; i < 5'000; ++i)
    {
        (void)read_one(input, source);
    }
    CHECK(source.offset() > 0); // text really was released
    CHECK(input.location().line == 5'001);
    CHECK(input.location().column == 1);
    const pnp::parse_error_context context = input.get_context();
    CHECK(context.before_pos_text == u8"345\n12345\n");
    CHECK(context.before_text_cut_off);
}

TEST_CASE(
    "release: context for a position that's already been released",
    "[parser_input][release]"
)
{
    repeating_source  source(u8"12345\n", 10'000);
    pnp::parser_input input(source, 3);
    for (int i = 0; i < 5'000; ++i)
    {
        (void)read_one(input, source);
    }
    REQUIRE(source.offset() > 0);
    const auto context = input.get_context(0);
    CHECK(context.before_text_cut_off);
    CHECK(context.location.line == input.location_at(source.offset()).line);
}

// ============================================================ run_parser

TEST_CASE(
    "run_parser: releases after a success, not after a failure",
    "[parser_input][release]"
)
{
    repeating_source  source(u8"12345 ", 10'000);
    pnp::parser_input input(source);
    int               value = 0;

    // A failure releases nothing: whoever ran it may go back.
    const auto failed = input.run_parser(consume_then_fail{}, value);
    CHECK(not failed.succeeded());
    CHECK(input.position() == 12'000);
    CHECK(source.offset() == 0);

    // A success releases what's behind it.
    const auto succeeded_result = input.run_parser(digits_parser{}, value);
    CHECK(succeeded_result.succeeded());
    CHECK(value == 5);
    CHECK(input.position() == 12'006);
    CHECK(source.offset() > 0);
}

TEST_CASE(
    "run_key: only parser_input runs pnps",
    "[parser_input][run_key]"
)
{
    STATIC_CHECK(not std::is_default_constructible_v<pnp::parser_run_key>);
    STATIC_CHECK(not std::is_copy_constructible_v<pnp::parser_run_key>);
    STATIC_CHECK(not std::is_move_constructible_v<pnp::parser_run_key>);
    STATIC_CHECK(pnp::parser_c<pnp::default_parser<int>, int>);

    // A type with the old signature is no longer a parser.
    struct old_style
    {
        pnp::basic_parser_return_value
            run_parser(pnp::parser_input&, int&) const
        {
            return {};
        }
    };

    STATIC_CHECK(not pnp::parser_c<old_style, int>);

    struct takes_key_by_reference
    {
        pnp::basic_parser_return_value
            run_parser(pnp::parser_input&, int&, const pnp::parser_run_key&) const
        {
            return {};
        }
    };

    struct takes_key_by_rvalue
    {
        pnp::basic_parser_return_value
            run_parser(pnp::parser_input&, int&, pnp::parser_run_key&&) const
        {
            return {};
        }
    };

    STATIC_CHECK(not pnp::parser_c<takes_key_by_reference, int>);
    STATIC_CHECK(not pnp::parser_c<takes_key_by_rvalue, int>);
}

// ============================================================ error details

TEST_CASE(
    "parser_input: advance reads more before deciding it's past the end",
    "[parser_input][io][errors]"
)
{
    // Enough arrives, a byte at a time: no error.
    {
        pnp::testing::piece_source source(std::u8string(u8"abcde"), 1);
        pnp::parser_input input(source);
        input.advance(5);
        CHECK(input.position() == 5);
        CHECK(input.at_end());
    }
    // Not enough ever arrives: available is everything there was.
    {
        pnp::testing::piece_source source(std::u8string(u8"abc"), 1);
        pnp::parser_input input(source);
        const auto        error = thrown_error(
            [&]
            {
                input.advance(5);
            }
        );
        REQUIRE(error.has_value());
        CHECK(error->code() == error_code::invalid_parser_advanced_past_end);
        CHECK(error->position() == 0);
        CHECK(error->details().requested == 5);
        CHECK(error->details().available == 3);
        CHECK(input.position() == 0);
    }
    // From partway through a fixed string.
    {
        pnp::parser_input input(std::u8string_view(u8"ab"));
        input.advance(1);
        const auto error = thrown_error(
            [&]
            {
                input.advance(3);
            }
        );
        REQUIRE(error.has_value());
        CHECK(error->position() == 1);
        CHECK(error->details().requested == 3);
        CHECK(error->details().available == 1);
    }
}

TEST_CASE(
    "parser_input: advancing into a character, in detail",
    "[parser_input][errors]"
)
{
    // The position is where it would have landed.
    {
        pnp::parser_input input(std::u8string_view(u8"aé!"));
        input.advance(1);
        const auto error = thrown_error(
            [&]
            {
                input.advance(1);
            }
        );
        REQUIRE(error.has_value());
        CHECK(
            error->code() == error_code::invalid_parser_advanced_into_character
        );
        CHECK(error->position() == 2);
        CHECK(error->details().requested == 1);
        CHECK(error->details().char8 == static_cast<char8_t>(0xA9));
        CHECK(input.position() == 1);
    }
    // The character hadn't arrived yet when advance was called.
    {
        pnp::testing::piece_source source(std::u8string(u8"aé!"), 1);
        pnp::parser_input input(source);
        const auto        error = thrown_error(
            [&]
            {
                input.advance(2);
            }
        );
        REQUIRE(error.has_value());
        CHECK(
            error->code() == error_code::invalid_parser_advanced_into_character
        );
        CHECK(error->position() == 2);
        CHECK(input.position() == 0);
    }
    // A token that ends partway through a character.
    {
        pnp::parser_input   input(std::u8string_view(u8"é"));
        const std::u8string half = byte(0xC3);
        const auto          error = thrown_error(
            [&]
            {
                (void)input.advance_if_starts_with(half);
            }
        );
        REQUIRE(error.has_value());
        CHECK(
            error->code() == error_code::invalid_parser_advanced_into_character
        );
        CHECK(error->position() == 1);
        CHECK(input.position() == 0);
    }
}

TEST_CASE(
    "parser_input: a predicate that splits a character, in detail",
    "[parser_input][errors]"
)
{
    const auto splits_e_acute = [](const char8_t c)
        {
            return c != static_cast<char8_t>(0xA9);
        };
    {
        pnp::parser_input input(std::u8string_view(u8"aé"));
        const auto        error = thrown_error(
            [&]
            {
                (void)input.peek_while(splits_e_acute);
            }
        );
        REQUIRE(error.has_value());
        CHECK(error->code() == error_code::invalid_parser_split_character);
        CHECK(error->position() == 2); // the rejected continuation byte
        CHECK(error->details().char8 == static_cast<char8_t>(0xA9));
    }
    // advance_while reports the same.
    {
        pnp::parser_input input(std::u8string_view(u8"aé"));
        const auto        error = thrown_error(
            [&]
            {
                (void)input.advance_while(splits_e_acute);
            }
        );
        REQUIRE(error.has_value());
        CHECK(error->code() == error_code::invalid_parser_split_character);
        CHECK(error->position() == 2);
    }
}

TEST_CASE(
    "parser_input: an escape matching the token, in detail",
    "[parser_input][errors]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abc"));
    input.advance(1);
    const auto error = thrown_error(
        [&]
        {
            (void)input.peek_until(u8"\"", u8"\"x");
        }
    );
    REQUIRE(error.has_value());
    CHECK(error->position() == 1);
    CHECK(error->details().token == u8"\"");
    CHECK(error->details().escape == u8"\"x");
}

TEST_CASE(
    "Marks: a foreign mark says where it came from",
    "[parser_input][marks][errors]"
)
{
    const std::string_view this_file
        = std::source_location::current().file_name();
    // From a different parser_input.
    {
        pnp::parser_input a(std::u8string_view(u8"abc"));
        pnp::parser_input b(std::u8string_view(u8"abc"));
        const auto mark_line = std::source_location::current().line() + 1;
        const auto mark_a = a.mark();
        const auto error = thrown_error(
            [&]
            {
                b.reset(mark_a);
            }
        );
        REQUIRE(error.has_value());
        const auto& details = error->details();
        CHECK(details.mark_problem == pnp::mark_problem_enum::other_input);
        // Where mark() was called, not somewhere inside parser_input.
        CHECK(details.mark_location.line() == mark_line);
        CHECK(std::string_view(details.mark_location.file_name()) == this_file);
        CHECK(details.reset_location.line() > mark_line);
        CHECK(
            std::string_view(details.reset_location.file_name()) == this_file
        );
    }
    // Moved from: still knows where it was made.
    {
        pnp::parser_input input(std::u8string_view(u8"abc"));
        const auto mark_line = std::source_location::current().line() + 1;
        auto       moved_from = input.mark();
        auto       moved_to = std::move(moved_from);
        const auto error = thrown_error(
            [&]
            {
                input.reset(moved_from);
            }
        );
        REQUIRE(error.has_value());
        CHECK(
            error->details().mark_problem == pnp::mark_problem_enum::moved_from
        );
        CHECK(error->details().mark_location.line() == mark_line);
    }
    // An explicit location is passed straight through.
    {
        pnp::parser_input a(std::u8string_view(u8"abc"));
        pnp::parser_input b(std::u8string_view(u8"abc"));
        const auto        mark_a = a.mark();
        const auto        here = std::source_location::current();
        const auto        error = thrown_error(
            [&]
            {
                b.reset(mark_a, here);
            }
        );
        REQUIRE(error.has_value());
        CHECK(error->details().reset_location.line() == here.line());
    }
}

TEST_CASE(
    "parser_input: invalid UTF-8 carries the decoder's reason",
    "[parser_input][errors]"
)
{
    // Each starts with a valid "a", so the error is at 1.
    const auto reason_for = [](const std::u8string& text_arg)
        {
            pnp::parser_input input{ std::u8string_view(text_arg) };
            const auto        error = thrown_error(
                [&]
                {
                    (void)input.peek_chars(text_arg.size());
                }
            );
            REQUIRE(error.has_value());
            CHECK(error->code() == error_code::invalid_utf8);
            CHECK(error->position() == 1);
            REQUIRE(error->details().utf8_error.has_value());
            return *error->details().utf8_error;
        };
    // Fill in the expected unicode_bridge::next_char32_error for each.
    const auto stray = reason_for(u8"a" + byte(0x80));
    const auto truncated = reason_for(u8"a" + byte(0xC3));
    const auto overlong = reason_for(u8"a" + byte(0xC0) + byte(0x80));
    const auto surrogate
        = reason_for(u8"a" + byte(0xED) + byte(0xA0) + byte(0x80));
    const auto too_large
        = reason_for(u8"a" + byte(0xF4) + byte(0x90) + byte(0x80) + byte(0x80));
    // CHECK(stray == unicode_bridge::next_char32_error::...); and so on.
    (void)stray;
    (void)truncated;
    (void)overlong;
    (void)surrogate;
    (void)too_large;

    // The same reason for a truncated character arriving a byte at a time.
    {
        pnp::testing::piece_source source(u8"a" + byte(0xC3), 1);
        pnp::parser_input input(source);
        const auto        error = thrown_error(
            [&]
            {
                (void)input.peek_chars(2);
            }
        );
        REQUIRE(error.has_value());
        CHECK(error->position() == 1);
        CHECK(
            error->details().utf8_error.value().message() == truncated.message()
        );
    }
}

TEST_CASE(
    "parser_input: context around a position inside a character",
    "[parser_input]"
)
{
    // The context is centred on the exact code unit, even partway through a
    // character; the context may then hold invalid UTF-8.
    pnp::parser_input input(std::u8string_view(u8"aé!"), 2);
    const auto        context = input.get_context(2);
    CHECK(context.before_pos_text == u8"a" + byte(0xC3));
    CHECK(context.current_pos_text == byte(0xA9));
    CHECK(context.after_pos_text == u8"!");

    // Invalid UTF-8 is the focus when it's what's being reported.
    const std::u8string stray = u8"a" + byte(0x80) + u8"b";
    pnp::parser_input   bad{ std::u8string_view(stray), 2 };
    CHECK(bad.get_context(1).current_pos_text == byte(0x80));
}