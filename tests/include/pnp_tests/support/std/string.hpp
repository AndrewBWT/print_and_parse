#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <memory_resource>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pnp_tests
{
    template <typename String>
    using string_pnp_t = pnp::support::generic_string_pnp<String, pnp::support::string_pnp_default>;

    // Prints with the default pnp, parses the text back, and checks the value.
    template <typename String>
    void
        check_string_round_trip(const String& value)
    {
        const auto text = pnp::to_u8string(value);
        INFO(unicode_bridge::cast_string_view<char>(unicode_bridge::to_formatted_unicode_string<char8_t>(text)));
        const auto parsed = pnp::try_parse<String>(text);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value);
    }

    // Prints with the given pnp, parses it back, and checks the value. The text is checked
    // only when an expectation is given.
    template <typename P, typename String>
    void
        check_string_round_trip(const P& pnp_obj, const String& value, std::u8string_view expected = {})
    {
        const auto text = pnp::to_u8string(pnp_obj, value);
        INFO(unicode_bridge::cast_string_view<char>(text));
        INFO(unicode_bridge::cast_string_view<char>(expected));
        if (not expected.empty())
        {
            if (text != expected)
            {
                int x = 4;
            }
            CHECK(text == expected);
        }
        const auto parsed = pnp::try_parse<String>(text, pnp_obj);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value);
    }

    // A user policy.
    struct single_quoted_strings
    {
        static constexpr std::u8string_view open_delimiter = u8"'";
        static constexpr std::u8string_view close_delimiter = u8"'";
    };

    // Rejected policies.
    struct bare_strings
    {
        static constexpr std::u8string_view open_delimiter = u8"";
        static constexpr std::u8string_view close_delimiter = u8"";
    };
    struct unescaped_close
    {
        static constexpr std::u8string_view open_delimiter = u8"<";
        static constexpr std::u8string_view close_delimiter = u8">";
    };
    struct backslash_close
    {
        static constexpr std::u8string_view open_delimiter = u8"\"";
        static constexpr std::u8string_view close_delimiter = u8"\\";
    };
    struct spaced_open
    {
        static constexpr std::u8string_view open_delimiter = u8" \"";
        static constexpr std::u8string_view close_delimiter = u8"\"";
    };
    struct missing_close_member
    {
        static constexpr std::u8string_view open_delimiter = u8"\"";
    };
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("strings: plain ASCII prints quoted", "[strings]")
{
    CHECK(pnp::to_u8string(std::string("abc")) == u8"\"abc\"");
    CHECK(pnp::to_u8string(std::u8string(u8"abc")) == u8"\"abc\"");
    CHECK(pnp::to_u8string(std::u16string(u"abc")) == u8"\"abc\"");
    CHECK(pnp::to_u8string(std::u32string(U"abc")) == u8"\"abc\"");
    CHECK(pnp::to_u8string(std::wstring(L"abc")) == u8"\"abc\"");

    // The empty string.
    CHECK(pnp::to_u8string(std::string()) == u8"\"\"");
    check_string_round_trip(std::string());
    check_string_round_trip(std::u32string());

    check_string_round_trip(std::string("hello world"));
    check_string_round_trip(std::u8string(u8"hello world"));
    check_string_round_trip(std::u16string(u"hello world"));
    check_string_round_trip(std::u32string(U"hello world"));
    check_string_round_trip(std::wstring(L"hello world"));
}

TEST_CASE("strings: special and escaped characters round-trip", "[strings]")
{
    // Whatever escapes ub chooses, the parser must read them back.
    check_string_round_trip(std::string("say \"hi\""));   // the delimiter itself
    check_string_round_trip(std::string("back\\slash"));
    check_string_round_trip(std::string("it's"));
    check_string_round_trip(std::string("tab\there"));
    check_string_round_trip(std::string("line\nbreak"));
    check_string_round_trip(std::string("nul\0inside", 10));   // an embedded null
    check_string_round_trip(std::string("\"\"\""));             // nothing but delimiters
    check_string_round_trip(std::string("\\\""));               // a backslash before a quote
}

TEST_CASE("strings: non-ASCII text round-trips", "[strings]")
{
    check_string_round_trip(std::u8string(u8"héllo wörld"));
    check_string_round_trip(std::u8string(u8"日本語"));
    check_string_round_trip(std::u8string(u8"😀 outside the BMP"));
    check_string_round_trip(std::u16string(u"😀 surrogate pair"));
    check_string_round_trip(std::u32string(U"😀 one code unit"));
    check_string_round_trip(std::wstring(L"héllo"));

    // Raw UTF-8 bytes in a plain std::string.
    check_string_round_trip(std::string("caf\xC3\xA9"));

    // Lone code units that aren't characters on their own.
    check_string_round_trip(std::u8string(1, static_cast<char8_t>(0xE9)));
    check_string_round_trip(std::u16string(1, static_cast<char16_t>(0xD83D)));
}

TEST_CASE("strings: other allocators and traits", "[strings]")
{
    // A pmr string uses the same default, with the allocator carried through.
    check_string_round_trip(std::pmr::string("polymorphic"));
    check_string_round_trip(std::pmr::u8string(u8"polymorphic"));
}

TEST_CASE("strings: custom delimiters through options", "[strings]")
{
    using u8_pnp = string_pnp_t<std::u8string>;

    const u8_pnp angled({ .open_delimiter = u8"<'", .close_delimiter = u8"'>" });
    CHECK(angled.options().open_delimiter == u8"<'");
    CHECK(angled.options().close_delimiter == u8"'>");
    check_string_round_trip(angled, std::u8string(u8"abc"), u8"<'abc'>");

    // The close delimiter starts with an escaped character, so content containing the
    // delimiter's pieces still round-trips.
    check_string_round_trip(angled, std::u8string(u8"a'>b"));
    check_string_round_trip(angled, std::u8string(u8"'>'>"));
    check_string_round_trip(angled, std::u8string(u8"<'nested'>"));

    // The open delimiter only appears once, so the content may contain it freely.
    check_string_round_trip(angled, std::u8string(u8"<'<'"));

    // Only the named field changes; the other keeps its policy value.
    const u8_pnp open_only({ .open_delimiter = u8"«" });
    check_string_round_trip(open_only, std::u8string(u8"abc"), u8"«abc\"");

    // Non-ASCII and multi-character delimiters.
    const u8_pnp guillemets({ .open_delimiter = u8"«", .close_delimiter = u8"\"»" });
    check_string_round_trip(guillemets, std::u8string(u8"abc"), u8"«abc\"»");

    // Whitespace inside a delimiter is fine: only the ends are restricted.
    const u8_pnp spaced({ .open_delimiter = u8"[ \"", .close_delimiter = u8"\" ]" });
    check_string_round_trip(spaced, std::u8string(u8"abc"), u8"[ \"abc\" ]");

    // A user policy works the same way.
    const pnp::support::generic_string_pnp<std::u8string, single_quoted_strings> single;
    check_string_round_trip(single, std::u8string(u8"abc"), u8"'abc'");
    check_string_round_trip(single, std::u8string(u8"it's \"quoted\""));

    // A modified copy of an existing configuration.
    auto modified = angled.options();
    modified.open_delimiter = u8"(";
    const u8_pnp copy(modified);
    check_string_round_trip(copy, std::u8string(u8"abc"), u8"(abc'>");

    // Custom delimiters appear in the error messages.
    const auto no_close = pnp::try_parse<std::u8string>(u8"<'abc", angled);
    CHECK(first_message(no_close) == u8"Expected closing delimiter \"'>\", but the input ended.");

    // The defaults inherit the options constructor.
    const pnp::default_printer<std::string> default_single({ .open_delimiter = u8"'", .close_delimiter = u8"'" });
    CHECK(pnp::to_u8string(default_single, std::string("abc")) == u8"'abc'");
}

TEST_CASE("strings: invalid options throw on construction", "[strings]")
{
    using u8_pnp = string_pnp_t<std::u8string>;

    // Empty delimiters.
    CHECK_THROWS_AS(u8_pnp({ .open_delimiter = u8"" }), std::invalid_argument);
    CHECK_THROWS_AS(u8_pnp({ .close_delimiter = u8"" }), std::invalid_argument);

    // Whitespace at either end.
    CHECK_THROWS_AS(u8_pnp({ .open_delimiter = u8" \"" }), std::invalid_argument);
    CHECK_THROWS_AS(u8_pnp({ .open_delimiter = u8"\" " }), std::invalid_argument);
    CHECK_THROWS_AS(u8_pnp({ .close_delimiter = u8"\t\"" }), std::invalid_argument);
    CHECK_THROWS_AS(u8_pnp({ .close_delimiter = u8"\"\n" }), std::invalid_argument);

    // Invalid UTF-8.
    CHECK_THROWS_AS(u8_pnp({ .open_delimiter = half_a_character }), std::invalid_argument);
    CHECK_THROWS_AS(u8_pnp({ .close_delimiter = u8"\"" + half_a_character }), std::invalid_argument);

    // The close delimiter must start with a character ub always escapes.
    CHECK_THROWS_AS(u8_pnp({ .close_delimiter = u8">" }), std::invalid_argument);
    CHECK_THROWS_AS(u8_pnp({ .close_delimiter = u8"end" }), std::invalid_argument);
    CHECK_THROWS_AS(u8_pnp({ .close_delimiter = u8"\\" }), std::invalid_argument);   // escapes start with one
    CHECK_THROWS_AS(u8_pnp({ .close_delimiter = u8">\"" }), std::invalid_argument);  // only the first matters
    CHECK_NOTHROW(u8_pnp({ .close_delimiter = u8"'" }));
    CHECK_NOTHROW(u8_pnp({ .close_delimiter = u8"\">" }));

    // The open delimiter has no such restriction.
    CHECK_NOTHROW(u8_pnp({ .open_delimiter = u8">" }));
}

TEST_CASE("strings: construction errors name the rule that was broken", "[strings]")
{
    using u8_pnp = string_pnp_t<std::u8string>;

    CHECK(invalid_argument_message([] { u8_pnp p({ .open_delimiter = half_a_character }); })
        == "string_pnp_options open delimiter must be valid UTF-8");
    CHECK(invalid_argument_message([] { u8_pnp p({ .close_delimiter = u8"\" " }); })
        == "string_pnp_options close delimiter must not start or end with whitespace");
    CHECK(invalid_argument_message([] { u8_pnp p({ .open_delimiter = u8"" }); })
        == "string_pnp_options open delimiter must be non-empty");
    CHECK(invalid_argument_message([] { u8_pnp p({ .close_delimiter = u8"" }); })
        == "string_pnp_options close delimiter must be non-empty");
    CHECK(invalid_argument_message([] { u8_pnp p({ .close_delimiter = u8">" }); })
        == "string_pnp_options close delimiter must start with '\"' or '\\'', the characters always escaped "
        "inside strings");
}

TEST_CASE("strings: parse errors", "[strings]")
{
    // A missing opening delimiter.
    const auto no_open = pnp::try_parse<std::string>(u8"abc\"");
    CHECK(not succeeded(no_open));
    CHECK(first_message(no_open) == u8"Expected opening delimiter \"\"\".");

    // A missing closing delimiter, including the case of no content at all.
    const auto no_close = pnp::try_parse<std::string>(u8"\"abc");
    CHECK(not succeeded(no_close));
    CHECK(first_message(no_close) == u8"Expected closing delimiter \"\"\", but the input ended.");
    CHECK(not succeeded(pnp::try_parse<std::string>(u8"\"")));

    // An escaped delimiter doesn't close the string.
    CHECK(not succeeded(pnp::try_parse<std::string>(u8"\"abc\\\"")));

    // An invalid escape mid-string reports ub's reason.
    const auto bad_escape = pnp::try_parse<std::string>(u8"\"ab\\qcd\"");
    CHECK(not succeeded(bad_escape));
    CHECK(first_message(bad_escape)
        .starts_with(u8"Could not read a character in the string. "
            u8"The Unicode parser returned the following error: "));

    // Text after a complete string is not a full parse.
    CHECK(not succeeded(pnp::try_parse<std::string>(u8"\"abc\"x")));

    // Empty input.
    CHECK(not succeeded(pnp::try_parse<std::string>(u8"")));
}

TEST_CASE("strings: characters that don't fit the target type", "[strings]")
{
    // é can't be represented in a char16_t string without its own escape problems, but it
    // fits; 😀 needs a surrogate pair in UTF-16 and one code unit in UTF-32. Both fit
    // because a string can hold several code units per character.
    const auto as_u16 = pnp::try_parse<std::u16string>(u8"\"😀\"");
    REQUIRE(succeeded(as_u16));
    CHECK(as_u16.value == u"😀");
    CHECK(as_u16.value.size() == 2);

    const auto as_u32 = pnp::try_parse<std::u32string>(u8"\"😀\"");
    REQUIRE(succeeded(as_u32));
    CHECK(as_u32.value == U"😀");
    CHECK(as_u32.value.size() == 1);
}

TEST_CASE("strings: inside containers", "[strings]")
{
    // Elements containing the container's own punctuation.
    const std::vector<std::string> words{ "a", "b,c", "[x]", "" };
    const auto text = pnp::to_u8string(words);
    CHECK(text == u8"[\"a\",\"b,c\",\"[x]\",\"\"]");
    const auto parsed = pnp::try_parse<std::vector<std::string>>(text);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == words);

    // Elements containing the string delimiter, which ub escapes.
    const std::vector<std::string> quoted{ "say \"hi\"", "\"" };
    const auto parsed_quoted = pnp::try_parse<std::vector<std::string>>(pnp::to_u8string(quoted));
    REQUIRE(succeeded(parsed_quoted));
    CHECK(parsed_quoted.value == quoted);
}

// --- Policy validation at compile time ---------------------------------------
static_assert(pnp::support::valid_string_pnp_c<pnp::support::string_pnp_default>);
static_assert(pnp::support::valid_string_pnp_c<single_quoted_strings>);
static_assert(not pnp::support::valid_string_pnp_c<bare_strings>);
static_assert(not pnp::support::valid_string_pnp_c<unescaped_close>);
static_assert(not pnp::support::valid_string_pnp_c<backslash_close>);
static_assert(not pnp::support::valid_string_pnp_c<spaced_open>);
static_assert(not pnp::support::valid_string_pnp_c<missing_close_member>);

// Validation stays wired up.
static_assert(pnp::support::checkable_options_c<pnp::support::string_pnp_options<pnp::support::string_pnp_default>>);

// Options for the same policy are shared across string types.
static_assert(std::same_as<string_pnp_t<std::string>::options_type, string_pnp_t<std::u32string>::options_type>);

// --- Which types have defaults -------------------------------------------------
static_assert(pnp::default_printable_c<std::string>);
static_assert(pnp::default_printable_c<std::u8string>);
static_assert(pnp::default_printable_c<std::pmr::string>);
static_assert(pnp::default_parseable_c<std::wstring>);

// --- The exact-type guard ----------------------------------------------------
static_assert(pnp::printer_c<pnp::default_printer<std::string>, std::string>);
static_assert(not pnp::printer_c<pnp::default_printer<std::string>, const char*>);
static_assert(not pnp::printer_c<pnp::default_printer<std::string>, std::string_view>);
static_assert(not pnp::printer_c<pnp::default_printer<std::string>, std::u8string>);
static_assert(not pnp::printer_c<pnp::default_printer<std::string>, std::pmr::string>,
    "a different allocator is a different type");