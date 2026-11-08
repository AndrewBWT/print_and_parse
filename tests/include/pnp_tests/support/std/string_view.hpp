#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pnp_tests
{
    template <typename View>
    using view_printer_t = pnp::support::generic_string_view_printer<View, pnp::support::string_pnp_default>;

    // A view prints exactly as the equivalent string does.
    template <typename CharT, typename Traits>
    void
        check_view_matches_string(const std::basic_string_view<CharT, Traits> view_arg)
    {
        const std::basic_string<CharT, Traits> as_string(view_arg);
        CHECK(pnp::to_u8string(view_arg) == pnp::to_u8string(as_string));
    }

    // A user policy, with a different name from the string tests' to avoid a clash.
    struct view_single_quoted
    {
        static constexpr std::u8string_view open_delimiter = u8"'";
        static constexpr std::u8string_view close_delimiter = u8"'";
    };

    // Rejected: the close delimiter doesn't start with an escaped character.
    struct view_unescaped_close
    {
        static constexpr std::u8string_view open_delimiter = u8"<";
        static constexpr std::u8string_view close_delimiter = u8">";
    };
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("string_view: plain ASCII prints quoted", "[string_view]")
{
    CHECK(pnp::to_u8string(std::string_view("abc")) == u8"\"abc\"");
    CHECK(pnp::to_u8string(std::u8string_view(u8"abc")) == u8"\"abc\"");
    CHECK(pnp::to_u8string(std::u16string_view(u"abc")) == u8"\"abc\"");
    CHECK(pnp::to_u8string(std::u32string_view(U"abc")) == u8"\"abc\"");
    CHECK(pnp::to_u8string(std::wstring_view(L"abc")) == u8"\"abc\"");

    // The empty view, both default-constructed and of zero length.
    CHECK(pnp::to_u8string(std::string_view()) == u8"\"\"");
    CHECK(pnp::to_u8string(std::string_view("abc", 0)) == u8"\"\"");
}

TEST_CASE("string_view: prints exactly as the equivalent string", "[string_view]")
{
    // Escaped and special characters.
    check_view_matches_string(std::string_view("say \"hi\""));
    check_view_matches_string(std::string_view("back\\slash"));
    check_view_matches_string(std::string_view("it's"));
    check_view_matches_string(std::string_view("tab\there"));
    check_view_matches_string(std::string_view("line\nbreak"));
    check_view_matches_string(std::string_view("nul\0inside", 10));   // an embedded null

    // Non-ASCII text in every character type.
    check_view_matches_string(std::u8string_view(u8"héllo wörld"));
    check_view_matches_string(std::u8string_view(u8"日本語"));
    check_view_matches_string(std::u16string_view(u"😀 surrogate pair"));
    check_view_matches_string(std::u32string_view(U"😀 one code unit"));
    check_view_matches_string(std::wstring_view(L"héllo"));

    // Lone code units.
    const char8_t  lead_byte[] = { static_cast<char8_t>(0xE9) };
    const char16_t high_surrogate[] = { static_cast<char16_t>(0xD83D) };
    check_view_matches_string(std::u8string_view(lead_byte, 1));
    check_view_matches_string(std::u16string_view(high_surrogate, 1));
}

TEST_CASE("string_view: only the view's range is printed", "[string_view]")
{
    // A view into the middle of a larger string.
    const std::string_view whole("abcdef");
    CHECK(pnp::to_u8string(whole.substr(2, 2)) == u8"\"cd\"");
    CHECK(pnp::to_u8string(whole.substr(0, 1)) == u8"\"a\"");
    CHECK(pnp::to_u8string(whole.substr(6)) == u8"\"\"");

    // A view of characters with no null terminator after them.
    const char no_terminator[] = { 'x', 'y', 'z' };
    CHECK(pnp::to_u8string(std::string_view(no_terminator, 3)) == u8"\"xyz\"");
    CHECK(pnp::to_u8string(std::string_view(no_terminator, 2)) == u8"\"xy\"");

    // A view into a std::string.
    const std::u8string backing(u8"prefix:value:suffix");
    CHECK(pnp::to_u8string(std::u8string_view(backing).substr(7, 5)) == u8"\"value\"");
}

TEST_CASE("string_view: configuring through options", "[string_view]")
{
    using u8_view_printer = view_printer_t<std::u8string_view>;
    using u8_string_pnp = pnp::support::generic_string_pnp<std::u8string, pnp::support::string_pnp_default>;

    const u8_view_printer angled({ .open_delimiter = u8"<'", .close_delimiter = u8"'>" });
    CHECK(angled.options().open_delimiter == u8"<'");
    CHECK(angled.options().close_delimiter == u8"'>");
    CHECK(pnp::to_u8string(angled, std::u8string_view(u8"abc")) == u8"<'abc'>");

    // The same options value on a string pnp produces identical output.
    const u8_string_pnp::options_type shared{ .open_delimiter = u8"«", .close_delimiter = u8"\"»" };
    const u8_view_printer view_with_shared(shared);
    const u8_string_pnp   string_with_shared(shared);
    for (const std::u8string_view text : {u8"abc", u8"", u8"say \"hi\"", u8"日本語"})
    {
        CHECK(pnp::to_u8string(view_with_shared, text) == pnp::to_u8string(string_with_shared, std::u8string(text)));
    }

    // Only the named field changes; the other keeps its policy value.
    const u8_view_printer open_only({ .open_delimiter = u8"«" });
    CHECK(pnp::to_u8string(open_only, std::u8string_view(u8"abc")) == u8"«abc\"");

    // A user policy works the same way.
    const pnp::support::generic_string_view_printer<std::u8string_view, view_single_quoted> single;
    CHECK(pnp::to_u8string(single, std::u8string_view(u8"abc")) == u8"'abc'");

    // The defaults inherit the options constructor.
    const pnp::default_printer<std::string_view> default_single({ .open_delimiter = u8"'", .close_delimiter = u8"'" });
    CHECK(pnp::to_u8string(default_single, std::string_view("abc")) == u8"'abc'");
}

TEST_CASE("string_view: invalid options throw on construction", "[string_view]")
{
    using u8_view_printer = view_printer_t<std::u8string_view>;

    // The same rules as the string pnp, because the options are the same type.
    CHECK_THROWS_AS(u8_view_printer({ .open_delimiter = u8"" }), std::invalid_argument);
    CHECK_THROWS_AS(u8_view_printer({ .close_delimiter = u8"\" " }), std::invalid_argument);
    CHECK_THROWS_AS(u8_view_printer({ .open_delimiter = half_a_character }), std::invalid_argument);
    CHECK_THROWS_AS(u8_view_printer({ .close_delimiter = u8">" }), std::invalid_argument);

    CHECK(invalid_argument_message([] { u8_view_printer p({ .close_delimiter = u8"" }); })
        == "string_pnp_options close delimiter must be non-empty");
}

TEST_CASE("string_view: inside containers", "[string_view]")
{
    // Elements containing the container's own punctuation.
    const std::vector<std::string_view> words{ "a", "b,c", "[x]", "" };
    CHECK(pnp::to_u8string(words) == u8"[\"a\",\"b,c\",\"[x]\",\"\"]");

    // A vector of views prints the same as a vector of the equivalent strings.
    const std::vector<std::string> as_strings(words.begin(), words.end());
    CHECK(pnp::to_u8string(words) == pnp::to_u8string(as_strings));
}

// --- Print-only ----------------------------------------------------------------
// A view can't own the text a parser would produce.
static_assert(pnp::default_printable_c<std::string_view>);
static_assert(pnp::default_printable_c<std::u8string_view>);
static_assert(pnp::default_printable_c<std::u16string_view>);
static_assert(pnp::default_printable_c<std::u32string_view>);
static_assert(pnp::default_printable_c<std::wstring_view>);
static_assert(not pnp::default_parseable_c<std::string_view>);
static_assert(not pnp::default_parseable_c<std::u8string_view>);

// --- Options and policies ------------------------------------------------------
static_assert(pnp::support::valid_string_pnp_c<view_single_quoted>);
static_assert(not pnp::support::valid_string_pnp_c<view_unescaped_close>);

// The view printer and the string pnp share an options type, so one configuration
// can be applied to both.
static_assert(std::same_as<view_printer_t<std::u8string_view>::options_type,
    pnp::support::generic_string_pnp<std::u8string, pnp::support::string_pnp_default>::options_type>);
static_assert(std::same_as<view_printer_t<std::string_view>::options_type, view_printer_t<std::u32string_view>::options_type>);

// --- The exact-type guard ----------------------------------------------------
static_assert(pnp::printer_c<pnp::default_printer<std::string_view>, std::string_view>);
static_assert(not pnp::printer_c<pnp::default_printer<std::string_view>, std::string>,
    "a string must be converted to a view explicitly");
static_assert(not pnp::printer_c<pnp::default_printer<std::string_view>, const char*>);
static_assert(not pnp::printer_c<pnp::default_printer<std::string_view>, std::u8string_view>);
static_assert(not pnp::printer_c<pnp::default_printer<std::u8string_view>, std::string_view>);