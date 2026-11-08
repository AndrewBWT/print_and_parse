#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace pnp_tests
{
    using default_nullptr_pnp = pnp::support::nullptr_pnp<>;

    // Prints nullptr with the given pnp, checks the text, and parses it back.
    template <typename P>
    void
        check_nullptr_round_trip(const P& pnp_obj, std::u8string_view expected)
    {
        const auto text = pnp::to_u8string(pnp_obj, nullptr);
        CHECK(text == expected);
        const auto parsed = pnp::try_parse<std::nullptr_t>(text, pnp_obj);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == nullptr);
    }

    // A user policy.
    struct null_word_policy
    {
        static constexpr std::u8string_view null_id = u8"null";
    };

    // Rejected policies.
    struct empty_null_id_policy
    {
        static constexpr std::u8string_view null_id = u8"";
    };
    struct leading_space_null_id_policy
    {
        static constexpr std::u8string_view null_id = u8" null";
    };
    struct trailing_space_null_id_policy
    {
        static constexpr std::u8string_view null_id = u8"null ";
    };
    struct missing_null_id_policy
    {
    };
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("nullptr: default round trip", "[nullptr]")
{
    CHECK(pnp::to_u8string(nullptr) == u8"nullptr");
    const auto parsed = pnp::try_parse<std::nullptr_t>(u8"nullptr");
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == nullptr);

    check_nullptr_round_trip(default_nullptr_pnp{}, u8"nullptr");
}

TEST_CASE("nullptr: parsing is exact", "[nullptr]")
{
    // The wrong word, with the expected one named in the message.
    const auto wrong_word = pnp::try_parse<std::nullptr_t>(u8"null");
    CHECK(not succeeded(wrong_word));
    CHECK(first_message(wrong_word) == u8"Expected \"nullptr\".");

    CHECK(not succeeded(pnp::try_parse<std::nullptr_t>(u8"")));
    CHECK(not succeeded(pnp::try_parse<std::nullptr_t>(u8"NULLPTR")));    // case-sensitive
    CHECK(not succeeded(pnp::try_parse<std::nullptr_t>(u8"0")));
    CHECK(succeeded(pnp::try_parse<std::nullptr_t>(u8" nullptr")));   // no whitespace skipping
    CHECK(not succeeded(pnp::try_parse<std::nullptr_t>(u8"nullptrx")));   // trailing input: not a full parse
    CHECK(not succeeded(pnp::try_parse<std::nullptr_t>(u8"nullpt")));     // a truncated word
}

TEST_CASE("nullptr: configuring through options", "[nullptr]")
{
    // A custom word.
    const default_nullptr_pnp null_word({ .null_id = u8"null" });
    CHECK(null_word.options().null_id == u8"null");
    check_nullptr_round_trip(null_word, u8"null");

    // The default word is no longer accepted, and the message names the custom one.
    const auto other_word = pnp::try_parse<std::nullptr_t>(u8"nil", null_word);
    CHECK(not succeeded(other_word));
    CHECK(first_message(other_word) == u8"Expected \"null\".");
    CHECK(not succeeded(pnp::try_parse<std::nullptr_t>(u8"nullptr", null_word)));
    // Non-ASCII words, and words with spaces inside.
    check_nullptr_round_trip(default_nullptr_pnp({ .null_id = u8"∅" }), u8"∅");
    check_nullptr_round_trip(default_nullptr_pnp({ .null_id = u8"no value" }), u8"no value");
    check_nullptr_round_trip(default_nullptr_pnp({ .null_id = u8"NULL" }), u8"NULL");

    // A user policy works the same way.
    check_nullptr_round_trip(pnp::support::nullptr_pnp<null_word_policy>{}, u8"null");

    // A modified copy of an existing configuration.
    auto modified = null_word.options();
    modified.null_id = u8"nil";
    check_nullptr_round_trip(default_nullptr_pnp(modified), u8"nil");

    // The defaults inherit the options constructor.
    const pnp::default_printer<std::nullptr_t> default_null({ .null_id = u8"null" });
    CHECK(pnp::to_u8string(default_null, nullptr) == u8"null");
    const pnp::default_parser<std::nullptr_t> default_null_parser({ .null_id = u8"null" });
    CHECK(succeeded(pnp::try_parse<std::nullptr_t>(u8"null", default_null_parser)));
}

TEST_CASE("nullptr: invalid options throw on construction", "[nullptr]")
{
    // Empty.
    CHECK_THROWS_AS(default_nullptr_pnp({ .null_id = u8"" }), std::invalid_argument);

    // Whitespace at either end.
    CHECK_THROWS_AS(default_nullptr_pnp({ .null_id = u8" null" }), std::invalid_argument);
    CHECK_THROWS_AS(default_nullptr_pnp({ .null_id = u8"null " }), std::invalid_argument);
    CHECK_THROWS_AS(default_nullptr_pnp({ .null_id = u8"\tnull" }), std::invalid_argument);
    CHECK_THROWS_AS(default_nullptr_pnp({ .null_id = u8"null\n" }), std::invalid_argument);
    CHECK_THROWS_AS(default_nullptr_pnp({ .null_id = u8" " }), std::invalid_argument);

    // Invalid UTF-8.
    CHECK_THROWS_AS(default_nullptr_pnp({ .null_id = half_a_character }), std::invalid_argument);
    CHECK_THROWS_AS(default_nullptr_pnp({ .null_id = u8"null" + half_a_character }), std::invalid_argument);

    // The messages name the options type and the rule.
    CHECK(invalid_argument_message([] { default_nullptr_pnp p({ .null_id = half_a_character }); })
        == "nullptr_pnp_options null id must be valid UTF-8");
    CHECK(invalid_argument_message([] { default_nullptr_pnp p({ .null_id = u8" null" }); })
        == "nullptr_pnp_options null id must not start or end with whitespace");
    CHECK(invalid_argument_message([] { default_nullptr_pnp p({ .null_id = u8"" }); })
        == "nullptr_pnp_options null id must be non-empty");

    CHECK_NOTHROW(default_nullptr_pnp({ .null_id = u8"null" }));
}

TEST_CASE("nullptr: inside containers", "[nullptr]")
{
    const std::vector<std::nullptr_t> nulls{ nullptr, nullptr, nullptr };
    const auto text = pnp::to_u8string(nulls);
    CHECK(text == u8"[nullptr,nullptr,nullptr]");
    const auto parsed = pnp::try_parse<std::vector<std::nullptr_t>>(text);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value.size() == 3);

    // A bad element fails the whole parse.
    CHECK(not succeeded(pnp::try_parse<std::vector<std::nullptr_t>>(u8"[nullptr,null]")));
}

TEST_CASE("nullptr: always a fixed output width", "[nullptr]")
{
    using null_pnp = pnp::support::nullptr_pnp<>;

    // The null token's length, in code units.
    CHECK(null_pnp{}.fixed_output_width() == 7);   // nullptr
    CHECK(null_pnp({ .null_id = u8"null" }).fixed_output_width() == 4);
    CHECK(null_pnp({ .null_id = u8"∅" }).fixed_output_width() == 3);   // one character, three code units

    // Read through the options too, and kept by copies and the default.
    const null_pnp word({ .null_id = u8"null" });
    CHECK(word.options().fixed_output_width() == 4);
    const null_pnp copy = word;
    CHECK(copy.fixed_output_width() == 4);
    CHECK(pnp::default_printer<std::nullptr_t>{}.fixed_output_width() == 7);

    // What's printed really has that width.
    CHECK(pnp::to_u8string(word, nullptr).size() == 4);
}

TEST_CASE("nullptr: always a fixed output width", "[nullptr][fixed_width]")
{
    using null_pnp = pnp::support::nullptr_pnp<>;

    // The null token's length, in code units.
    CHECK(null_pnp{}.fixed_output_width() == 7);   // nullptr
    CHECK(null_pnp({ .null_id = u8"null" }).fixed_output_width() == 4);
    CHECK(null_pnp({ .null_id = u8"∅" }).fixed_output_width() == 3);   // one character, three code units

    // Read through the options too, and kept by copies and the default.
    const null_pnp word({ .null_id = u8"null" });
    CHECK(word.options().fixed_output_width() == 4);
    const null_pnp copy = word;
    CHECK(copy.fixed_output_width() == 4);
    CHECK(pnp::default_printer<std::nullptr_t>{}.fixed_output_width() == 7);

    // What's printed really has that width.
    CHECK(pnp::to_u8string(word, nullptr).size() == 4);
}

TEST_CASE("nullptr: the token it prints", "[nullptr]")
{
    using null_pnp = pnp::support::nullptr_pnp<>;

    // The text it always prints, read back through token().
    CHECK(null_pnp{}.token() == u8"nullptr");
    CHECK(null_pnp({ .null_id = u8"null" }).token() == u8"null");

    // And it is exactly what's printed.
    const null_pnp word({ .null_id = u8"∅" });
    CHECK(pnp::to_u8string(word, nullptr) == word.token());
}

// A pnp that always prints the same text satisfies token_pnp_c; one whose output varies doesn't.
static_assert(pnp::support::internal::token_pnp_c<pnp::support::nullptr_pnp<>>);
static_assert(pnp::support::internal::token_pnp_c<pnp::default_printer<std::nullptr_t>>);
static_assert(not pnp::support::internal::token_pnp_c<pnp::support::integer_pnp<int>>);

static_assert(pnp::support::fixed_output_width_c<pnp::support::nullptr_pnp<>>);

static_assert(pnp::support::fixed_output_width_c<pnp::support::nullptr_pnp<>>);

// --- Policy validation at compile time ---------------------------------------
static_assert(pnp::support::valid_nullptr_pnp_c<pnp::support::nullptr_pnp_default>);
static_assert(pnp::support::valid_nullptr_pnp_c<null_word_policy>);
static_assert(not pnp::support::valid_nullptr_pnp_c<empty_null_id_policy>);
static_assert(not pnp::support::valid_nullptr_pnp_c<leading_space_null_id_policy>);
static_assert(not pnp::support::valid_nullptr_pnp_c<trailing_space_null_id_policy>);
static_assert(not pnp::support::valid_nullptr_pnp_c<missing_null_id_policy>);

// Validation stays wired up.
static_assert(pnp::support::checkable_options_c<pnp::support::nullptr_pnp_options<pnp::support::nullptr_pnp_default>>);

// The shared null-token rules, used by the pointer printers too.
static_assert(pnp::support::internal::valid_null_id(u8"nullptr"));
static_assert(pnp::support::internal::valid_null_id(u8"no value"));
static_assert(not pnp::support::internal::valid_null_id(u8""));
static_assert(not pnp::support::internal::valid_null_id(u8" nullptr"));

// --- Defaults ------------------------------------------------------------------
static_assert(pnp::default_printable_c<std::nullptr_t>);
static_assert(pnp::default_parseable_c<std::nullptr_t>);

// --- The exact-type guard ----------------------------------------------------
static_assert(pnp::printer_c<default_nullptr_pnp, std::nullptr_t>);
static_assert(not pnp::printer_c<default_nullptr_pnp, int*>);
static_assert(not pnp::printer_c<default_nullptr_pnp, void*>);
static_assert(not pnp::printer_c<default_nullptr_pnp, int>);
static_assert(not pnp::printer_c<default_nullptr_pnp, bool>);

// Parsers were never affected: a std::nullptr_t& only binds to a std::nullptr_t.
static_assert(pnp::parser_c<default_nullptr_pnp, std::nullptr_t>);
static_assert(not pnp::parser_c<default_nullptr_pnp, int*>);