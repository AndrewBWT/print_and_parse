#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pnp_tests
{
using default_bool_pnp = pnp::support::bool_pnp<>;

// Prints both values, checks the text, and parses each back.
template <typename P>
void
    check_bool_words(
        const P&           pnp_obj,
        std::u8string_view true_text,
        std::u8string_view false_text
    )
{
    CHECK(pnp::to_u8string(pnp_obj, true) == true_text);
    CHECK(pnp::to_u8string(pnp_obj, false) == false_text);
    const auto parsed_true  = pnp::try_parse<bool, P>(true_text, pnp_obj);
    const auto parsed_false = pnp::try_parse<bool, P>(false_text, pnp_obj);
    REQUIRE(succeeded(parsed_true));
    REQUIRE(succeeded(parsed_false));
    CHECK(parsed_true.value == true);
    CHECK(parsed_false.value == false);
}

// A user policy.
struct on_off_words
{
    static constexpr std::u8string_view true_value  = u8"on";
    static constexpr std::u8string_view false_value = u8"off";
};

// Rejected policies.
struct true_prefix_of_false
{
    static constexpr std::u8string_view true_value  = u8"on";
    static constexpr std::u8string_view false_value = u8"one-off";
};

struct false_prefix_of_true
{
    static constexpr std::u8string_view true_value  = u8"yes-please";
    static constexpr std::u8string_view false_value = u8"yes";
};

struct identical_words
{
    static constexpr std::u8string_view true_value  = u8"x";
    static constexpr std::u8string_view false_value = u8"x";
};

struct empty_true_word
{
    static constexpr std::u8string_view true_value  = u8"";
    static constexpr std::u8string_view false_value = u8"false";
};

struct empty_false_word
{
    static constexpr std::u8string_view true_value  = u8"true";
    static constexpr std::u8string_view false_value = u8"";
};

struct leading_space_word
{
    static constexpr std::u8string_view true_value  = u8" true";
    static constexpr std::u8string_view false_value = u8"false";
};

struct trailing_space_word
{
    static constexpr std::u8string_view true_value  = u8"true";
    static constexpr std::u8string_view false_value = u8"false ";
};

struct missing_false_word
{
    static constexpr std::u8string_view true_value = u8"true";
};
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "bool: default round trips",
    "[bool]"
)
{
    CHECK(pnp::to_u8string(true) == u8"true");
    CHECK(pnp::to_u8string(false) == u8"false");
    for (const bool value : {true, false})
    {
        const auto parsed = pnp::try_parse<bool>(pnp::to_u8string(value));
        REQUIRE(parsed.outcome.succeeded());
        CHECK(parsed.value == value);
    }
}

TEST_CASE(
    "bool: every built-in word set",
    "[bool]"
)
{
    using namespace pnp::support;
    check_bool_words(bool_pnp<bool_pnp_vals_words_lc>{}, u8"true", u8"false");
    check_bool_words(bool_pnp<bool_pnp_vals_words_uc>{}, u8"TRUE", u8"FALSE");
    check_bool_words(
        bool_pnp<bool_pnp_vals_words_uc_first>{}, u8"True", u8"False"
    );
    check_bool_words(bool_pnp<bool_pnp_vals_letter_lc>{}, u8"t", u8"f");
    check_bool_words(bool_pnp<bool_pnp_vals_letter_uc>{}, u8"T", u8"F");
    check_bool_words(bool_pnp<bool_pnp_vals_answer_lc>{}, u8"yes", u8"no");
    check_bool_words(bool_pnp<bool_pnp_vals_answer_uc>{}, u8"YES", u8"NO");
    check_bool_words(
        bool_pnp<bool_pnp_vals_answer_uc_first>{}, u8"Yes", u8"No"
    );

    // A user policy works the same way.
    check_bool_words(bool_pnp<on_off_words>{}, u8"on", u8"off");
}

TEST_CASE(
    "bool: parsing is exact",
    "[bool]"
)
{
    CHECK(not succeeded(pnp::try_parse<bool>(u8"True"))); // case-sensitive
    CHECK(not succeeded(pnp::try_parse<bool>(u8"1")));
    CHECK(not succeeded(pnp::try_parse<bool>(u8"")));
    CHECK(not succeeded(pnp::try_parse<bool>(u8"maybe")));
    CHECK(succeeded(pnp::try_parse<bool>(u8" true"))); // whitespace skipping
    CHECK(not succeeded(pnp::try_parse<bool>(u8"truex"))
    ); // trailing input: not a full parse

    // A word followed directly by the other word is still only a partial parse.
    const default_bool_pnp on_off({.true_value = u8"on", .false_value = u8"off"}
    );
    CHECK(not succeeded(pnp::try_parse<bool>(u8"onoff", on_off)));

    const auto failed = pnp::try_parse<bool>(u8"maybe");
    CHECK(
        first_message(failed)
        == u8"Could not construct bool. Expected either \"true\" representing "
           u8"true, "
           u8"or \"false\" representing false."
    );
}

TEST_CASE(
    "bool: configuring through options",
    "[bool]"
)
{
    // Both words.
    const default_bool_pnp on_off({.true_value = u8"on", .false_value = u8"off"}
    );
    check_bool_words(on_off, u8"on", u8"off");
    CHECK(on_off.options().true_value == u8"on");
    CHECK(on_off.options().false_value == u8"off");

    // Only the named field changes; the other keeps its policy value.
    const default_bool_pnp yes_or_false({.true_value = u8"yes"});
    check_bool_words(yes_or_false, u8"yes", u8"false");

    // Options override a non-default policy too.
    const pnp::support::bool_pnp<pnp::support::bool_pnp_vals_answer_lc> answer(
        {.false_value = u8"nope"}
    );
    check_bool_words(answer, u8"yes", u8"nope");

    // Non-ASCII words.
    const default_bool_pnp ticks({.true_value = u8"✓", .false_value = u8"✗"});
    check_bool_words(ticks, u8"✓", u8"✗");

    // Whitespace inside a word is fine: only the ends are restricted.
    const default_bool_pnp spaced(
        {.true_value = u8"turned on", .false_value = u8"turned off"}
    );
    check_bool_words(spaced, u8"turned on", u8"turned off");

    // A modified copy of an existing configuration.
    auto modified       = on_off.options();
    modified.true_value = u8"enabled";
    const default_bool_pnp copy(modified);
    check_bool_words(copy, u8"enabled", u8"off");

    // Custom words appear in the error message.
    const auto failed = pnp::try_parse<bool>(u8"maybe", on_off);
    CHECK(
        first_message(failed)
        == u8"Could not construct bool. Expected either \"on\" representing "
           u8"true, "
           u8"or \"off\" representing false."
    );

    // The defaults inherit the options constructor.
    const pnp::default_printer<bool> default_on_off(
        {.true_value = u8"on", .false_value = u8"off"}
    );
    CHECK(pnp::to_u8string(default_on_off, true) == u8"on");
}

TEST_CASE(
    "bool: invalid options throw on construction",
    "[bool]"
)
{
    // Empty words.
    CHECK_THROWS_AS(
        default_bool_pnp({.true_value = u8""}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_bool_pnp({.false_value = u8""}), std::invalid_argument
    );

    // Identical words, and words that are prefixes of each other in either
    // direction.
    CHECK_THROWS_AS(
        (default_bool_pnp({.true_value = u8"x", .false_value = u8"x"})),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        (default_bool_pnp({.true_value = u8"on", .false_value = u8"one-off"})),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        (default_bool_pnp({.true_value = u8"yes-please", .false_value = u8"yes"}
        )),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_bool_pnp({.true_value = u8"f"}), std::invalid_argument
    ); // prefix of "false"

    // Whitespace at either end.
    CHECK_THROWS_AS(
        default_bool_pnp({.true_value = u8" true"}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_bool_pnp({.true_value = u8"true "}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_bool_pnp({.false_value = u8"\tfalse"}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_bool_pnp({.false_value = u8"false\n"}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_bool_pnp({.true_value = u8" "}), std::invalid_argument
    );

    // Invalid UTF-8.
    CHECK_THROWS_AS(
        default_bool_pnp({.true_value = half_a_character}),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        default_bool_pnp({.false_value = u8"no" + half_a_character}),
        std::invalid_argument
    );

    // The messages name the options type and the rule.
    try
    {
        default_bool_pnp p({.true_value = u8"x", .false_value = u8"x"});
        FAIL("expected std::invalid_argument");
    }
    catch (const std::invalid_argument& error)
    {
        CHECK(
            std::string(error.what())
            == "bool_pnp_options true and false values must not be prefixes of "
               "each other"
        );
    }

    CHECK_NOTHROW(
        default_bool_pnp({.true_value = u8"on", .false_value = u8"off"})
    );
}

TEST_CASE(
    "bool: inside containers",
    "[bool]"
)
{
    const std::vector<bool> flags{true, false, true};
    CHECK(pnp::to_u8string(flags) == u8"[true,false,true]");
    const auto parsed
        = pnp::try_parse<std::vector<bool>>(u8"[true,false,true]");
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == flags);
}

// --- Policy validation at compile time ---------------------------------------
static_assert(pnp::support::valid_bool_pnp_c<
              pnp::support::bool_pnp_vals_words_lc>);
static_assert(pnp::support::valid_bool_pnp_c<
              pnp::support::bool_pnp_vals_words_uc>);
static_assert(pnp::support::valid_bool_pnp_c<
              pnp::support::bool_pnp_vals_words_uc_first>);
static_assert(pnp::support::valid_bool_pnp_c<
              pnp::support::bool_pnp_vals_letter_lc>);
static_assert(pnp::support::valid_bool_pnp_c<
              pnp::support::bool_pnp_vals_letter_uc>);
static_assert(pnp::support::valid_bool_pnp_c<
              pnp::support::bool_pnp_vals_answer_lc>);
static_assert(pnp::support::valid_bool_pnp_c<
              pnp::support::bool_pnp_vals_answer_uc>);
static_assert(pnp::support::valid_bool_pnp_c<
              pnp::support::bool_pnp_vals_answer_uc_first>);
static_assert(pnp::support::valid_bool_pnp_c<on_off_words>);
static_assert(not pnp::support::valid_bool_pnp_c<true_prefix_of_false>);
static_assert(not pnp::support::valid_bool_pnp_c<false_prefix_of_true>);
static_assert(not pnp::support::valid_bool_pnp_c<identical_words>);
static_assert(not pnp::support::valid_bool_pnp_c<empty_true_word>);
static_assert(not pnp::support::valid_bool_pnp_c<empty_false_word>);
static_assert(not pnp::support::valid_bool_pnp_c<leading_space_word>);
static_assert(not pnp::support::valid_bool_pnp_c<trailing_space_word>);
static_assert(not pnp::support::valid_bool_pnp_c<missing_false_word>);

// Validation stays wired up.
static_assert(pnp::support::checkable_options_c<pnp::support::bool_pnp_options<
                  pnp::support::bool_pnp_vals_words_lc>>);

// --- The exact-type guard ----------------------------------------------------
namespace pnp_tests
{
// An unguarded printer, written the "obvious" way. It exists only to
// demonstrate the leak.
struct unguarded_bool_printer
{
    void
        run_printer(
            const bool&     object_arg,
            pnp::text_sink& sink_obj_arg
        ) const
    {
        sink_obj_arg.write(std::u8string_view(object_arg ? u8"true" : u8"false")
        );
    }
};

// A user type with an implicit conversion to bool.
struct truthy
{
    operator bool() const
    {
        return true;
    }
};

// A user type with an explicit conversion, which should never be accepted.
struct explicit_truthy
{
    explicit
        operator bool() const
    {
        return true;
    }
};
} // namespace pnp_tests

TEST_CASE(
    "bool: a fixed output width when the words have the same length",
    "[bool]"
)
{
    using words_pnp = pnp::support::bool_pnp<>;

    // "true" and "false" differ in length.
    CHECK(not words_pnp{}.fixed_output_width().has_value());

    // Same-length words, including the letter policies.
    CHECK(
        pnp::support::bool_pnp<pnp::support::bool_pnp_vals_letter_uc>{}
            .fixed_output_width()
        == 1
    );
    CHECK(
        words_pnp({.true_value = u8"1", .false_value = u8"0"})
            .fixed_output_width()
        == 1
    );
    CHECK(
        words_pnp({.true_value = u8"on", .false_value = u8"no"})
            .fixed_output_width()
        == 2
    );

    // Code units, not characters: "✓" and "✗" are three each.
    CHECK(
        words_pnp({.true_value = u8"✓", .false_value = u8"✗"})
            .fixed_output_width()
        == 3
    );

    // One character can match two, and the reverse: "é" is two code units.
    CHECK(
        words_pnp({.true_value = u8"é", .false_value = u8"ab"})
            .fixed_output_width()
        == 2
    );
    CHECK(not words_pnp({.true_value = u8"é", .false_value = u8"a"})
                  .fixed_output_width()
                  .has_value());

    // The width comes from the options, so it reads the same through them.
    const words_pnp digits({.true_value = u8"1", .false_value = u8"0"});
    CHECK(digits.options().fixed_output_width() == 1);

    // Copies keep it, and the defaults inherit it.
    const words_pnp copy = digits;
    CHECK(copy.fixed_output_width() == 1);
    CHECK(
        pnp::default_printer<bool>({.true_value = u8"T", .false_value = u8"F"})
            .fixed_output_width()
        == 1
    );

    // Both words print at that width.
    CHECK(pnp::to_u8string(digits, true).size() == 1);
    CHECK(pnp::to_u8string(digits, false).size() == 1);
}

static_assert(pnp::support::fixed_output_width_c<pnp::support::bool_pnp<>>);
static_assert(pnp::support::fixed_output_width_c<pnp::default_printer<bool>>);

// The leak exists for an unguarded printer. These document the language
// behaviour the guard protects against; if they fail, printer_c has started
// rejecting conversions by itself.
static_assert(pnp::printer_c<unguarded_bool_printer, bool>);
static_assert(
    pnp::printer_c<unguarded_bool_printer, int>,
    "int converts to bool"
);
static_assert(
    pnp::printer_c<unguarded_bool_printer, int*>,
    "pointers convert to bool"
);
static_assert(
    pnp::printer_c<unguarded_bool_printer, double>,
    "double converts to bool"
);
static_assert(
    pnp::printer_c<unguarded_bool_printer, truthy>,
    "implicit operator bool"
);
static_assert(
    not pnp::printer_c<unguarded_bool_printer, explicit_truthy>,
    "explicit operator bool is never used implicitly"
);

// pnp's guarded bool printer accepts only bool.
static_assert(pnp::printer_c<default_bool_pnp, bool>);
static_assert(not pnp::printer_c<default_bool_pnp, int>);
static_assert(not pnp::printer_c<default_bool_pnp, int*>);
static_assert(not pnp::printer_c<default_bool_pnp, double>);
static_assert(not pnp::printer_c<default_bool_pnp, char>);
static_assert(not pnp::printer_c<default_bool_pnp, truthy>);
static_assert(not pnp::printer_c<default_bool_pnp, explicit_truthy>);

// Parsers were never affected: a bool& only binds to a bool.
static_assert(pnp::parser_c<default_bool_pnp, bool>);
static_assert(not pnp::parser_c<default_bool_pnp, int>);

// The guard stops the leak propagating into containers.
template <typename Container, typename ElementPrinter>
concept sequence_printer_formable = requires {
    typename pnp::support::generic_sequence_pnp<Container, ElementPrinter>;
};

static_assert(sequence_printer_formable<std::vector<bool>, default_bool_pnp>);
static_assert(
    not sequence_printer_formable<std::vector<int>, default_bool_pnp>,
    "a bool printer must not be usable as the element printer for ints"
);
static_assert(
    sequence_printer_formable<std::vector<int>, unguarded_bool_printer>,
    "the unguarded printer still leaks into containers"
);