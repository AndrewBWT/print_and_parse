#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pnp_tests
{
// Names where one is a prefix of another, and a value that isn't contiguous.
enum class colour : int
{
    red,
    green,
    blue,
    reddish = 10
};

// A signed underlying type, for negative unlisted values.
enum class level : std::int8_t
{
    low  = -1,
    mid  = 0,
    high = 1
};

// Bool-based enums: one listing a single value, one listing both.
enum class lights : bool
{
    off = false,
    on  = true
};
enum class switch_state : bool
{
    down = false,
    up   = true
};

// Non-ASCII names, and a name with a space inside.
enum class greek
{
    alpha,
    beta
};
enum class shade
{
    dark_red,
    light_red
};

// A name starting with the default unlisted prefix.
enum class hashed
{
    tag
};

// Invalid lists.
enum class empty_list_enum
{
    a
};
enum class duplicate_value_enum
{
    a,
    b
};
enum class duplicate_name_enum
{
    a,
    b
};
enum class spaced_name_enum
{
    a
};
enum class empty_name_enum
{
    a
};

// in namespace pnp_tests, with the other enums:
// Names all the same length: fixed width when unlisted values aren't allowed.
enum class direction
{
    north,
    east,
    south,
    west
};

// A policy that doesn't allow unlisted values.
struct strict_enum_policy
{
    static constexpr bool               allow_unlisted  = false;
    static constexpr std::u8string_view unlisted_prefix = u8"#";
};
} // namespace pnp_tests

PNP_NS_BEGIN

template <>
struct enum_list<pnp_tests::colour>
{
    static constexpr enum_element<pnp_tests::colour> values[] = {
        {pnp_tests::colour::red,     u8"red"    },
        {pnp_tests::colour::green,   u8"green"  },
        {pnp_tests::colour::blue,    u8"blue"   },
        {pnp_tests::colour::reddish, u8"reddish"},
    };
};

template <>
inline constexpr bool enable_default_enum_pnp<pnp_tests::colour> = true;

template <>
struct enum_list<pnp_tests::level>
{
    static constexpr enum_element<pnp_tests::level> values[] = {
        {pnp_tests::level::low,  u8"low" },
        {pnp_tests::level::mid,  u8"mid" },
        {pnp_tests::level::high, u8"high"},
    };
};

template <>
inline constexpr bool enable_default_enum_pnp<pnp_tests::level> = true;

template <>
struct enum_list<pnp_tests::lights>
{
    // Only one of the two values is listed.
    static constexpr enum_element<pnp_tests::lights> values[] = {
        {pnp_tests::lights::on, u8"on"}
    };
};

template <>
inline constexpr bool enable_default_enum_pnp<pnp_tests::lights> = true;

template <>
struct enum_list<pnp_tests::switch_state>
{
    static constexpr enum_element<pnp_tests::switch_state> values[] = {
        {pnp_tests::switch_state::down, u8"down"},
        {pnp_tests::switch_state::up,   u8"up"  },
    };
};

template <>
inline constexpr bool enable_default_enum_pnp<pnp_tests::switch_state> = true;

template <>
struct enum_list<pnp_tests::greek>
{
    static constexpr enum_element<pnp_tests::greek> values[] = {
        {pnp_tests::greek::alpha, u8"α"},
        {pnp_tests::greek::beta,  u8"β"},
    };
};

template <>
inline constexpr bool enable_default_enum_pnp<pnp_tests::greek> = true;

template <>
struct enum_list<pnp_tests::shade>
{
    static constexpr enum_element<pnp_tests::shade> values[] = {
        {pnp_tests::shade::dark_red,  u8"dark red" },
        {pnp_tests::shade::light_red, u8"light red"},
    };
};

template <>
inline constexpr bool enable_default_enum_pnp<pnp_tests::shade> = true;

template <>
struct enum_list<pnp_tests::direction>
{
    static constexpr enum_element<pnp_tests::direction> values[] = {
        {pnp_tests::direction::north, u8"N"},
        {pnp_tests::direction::east,  u8"E"},
        {pnp_tests::direction::south, u8"S"},
        {pnp_tests::direction::west,  u8"W"},
    };
};

template <>
inline constexpr bool enable_default_enum_pnp<pnp_tests::direction> = true;

// Listed, but not opted in to the defaults: "#tag" conflicts with the default
// prefix.
template <>
struct enum_list<pnp_tests::hashed>
{
    static constexpr enum_element<pnp_tests::hashed> values[] = {
        {pnp_tests::hashed::tag, u8"#tag"}
    };
};

// Invalid lists: not listed enums, so there's nothing to opt in.
template <>
struct enum_list<pnp_tests::empty_list_enum>
{
    static constexpr std::array<enum_element<pnp_tests::empty_list_enum>, 0>
        values = {};
};

template <>
struct enum_list<pnp_tests::duplicate_value_enum>
{
    static constexpr enum_element<pnp_tests::duplicate_value_enum> values[] = {
        {pnp_tests::duplicate_value_enum::a, u8"a"},
        {pnp_tests::duplicate_value_enum::a, u8"b"},
    };
};

template <>
struct enum_list<pnp_tests::duplicate_name_enum>
{
    static constexpr enum_element<pnp_tests::duplicate_name_enum> values[] = {
        {pnp_tests::duplicate_name_enum::a, u8"x"},
        {pnp_tests::duplicate_name_enum::b, u8"x"},
    };
};

template <>
struct enum_list<pnp_tests::spaced_name_enum>
{
    static constexpr enum_element<pnp_tests::spaced_name_enum> values[] = {
        {pnp_tests::spaced_name_enum::a, u8" a"}
    };
};

template <>
struct enum_list<pnp_tests::empty_name_enum>
{
    static constexpr enum_element<pnp_tests::empty_name_enum> values[] = {
        {pnp_tests::empty_name_enum::a, u8""}
    };
};

PNP_NS_END

namespace pnp_tests
{
using colour_pnp = pnp::support::listed_enum_pnp<colour>;

// Prints with the given pnp, checks the text, parses it back, and checks the
// value.
template <typename P, typename E>
void
    check_enum_round_trip(
        const P&           pnp_obj,
        const E            value,
        std::u8string_view expected
    )
{
    const auto text = pnp::to_u8string(pnp_obj, value);
    CHECK(text == expected);
    const auto parsed = pnp::try_parse<E>(text, pnp_obj);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value);
}
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "enums: listed values round-trip by name",
    "[enums]"
)
{
    CHECK(pnp::to_u8string(colour::red) == u8"red");
    CHECK(pnp::to_u8string(colour::reddish) == u8"reddish");

    const colour_pnp p;
    check_enum_round_trip(p, colour::red, u8"red");
    check_enum_round_trip(p, colour::green, u8"green");
    check_enum_round_trip(p, colour::blue, u8"blue");
    check_enum_round_trip(p, colour::reddish, u8"reddish");

    // Non-ASCII names, and names with a space inside.
    check_enum_round_trip(
        pnp::support::listed_enum_pnp<greek>{}, greek::beta, u8"β"
    );
    check_enum_round_trip(
        pnp::support::listed_enum_pnp<shade>{}, shade::light_red, u8"light red"
    );

    // The defaults, for each opted-in enum.
    CHECK(pnp::to_u8string(level::low) == u8"low");
    CHECK(pnp::to_u8string(greek::alpha) == u8"α");
    CHECK(pnp::to_u8string(shade::dark_red) == u8"dark red");
    CHECK(pnp::to_u8string(switch_state::up) == u8"up");
}

TEST_CASE(
    "enums: the longest matching name wins",
    "[enums]"
)
{
    // "red" is a prefix of "reddish", so the parser must try the longer name
    // first.
    const auto reddish = pnp::try_parse<colour>(u8"reddish");
    REQUIRE(succeeded(reddish));
    CHECK(reddish.value == colour::reddish);

    const auto red = pnp::try_parse<colour>(u8"red");
    REQUIRE(succeeded(red));
    CHECK(red.value == colour::red);

    // "redx" matches "red", leaving "x": not a full parse.
    CHECK(not succeeded(pnp::try_parse<colour>(u8"redx")));
}

TEST_CASE(
    "enums: unlisted values round-trip as their underlying value",
    "[enums]"
)
{
    const colour_pnp p;
    check_enum_round_trip(p, static_cast<colour>(600), u8"#600");
    check_enum_round_trip(
        p, static_cast<colour>(3), u8"#3"
    ); // between listed values

    // A negative underlying value.
    check_enum_round_trip(
        pnp::support::listed_enum_pnp<level>{}, static_cast<level>(-5), u8"#-5"
    );

    // A configured underlying part.
    const colour_pnp plus(
        {.underlying = colour_pnp::underlying_part_type(
             {.use_plus_for_positive_numbers = true}
         )}
    );
    check_enum_round_trip(plus, static_cast<colour>(600), u8"#+600");

    // A custom prefix.
    const colour_pnp tilde({.unlisted_prefix = u8"~"});
    check_enum_round_trip(tilde, static_cast<colour>(600), u8"~600");
    CHECK(tilde.options().unlisted_prefix == u8"~");

    // A bad underlying value reports the underlying part's error.
    CHECK(not succeeded(pnp::try_parse<colour>(u8"#x")));
}

TEST_CASE(
    "enums: unlisted values can be forbidden",
    "[enums]"
)
{
    const colour_pnp strict({.allow_unlisted = false});
    CHECK(not strict.options().allow_unlisted);

    // Listed values still work.
    check_enum_round_trip(strict, colour::blue, u8"blue");

    // Printing an unlisted value throws; parsing one fails.
    CHECK_THROWS_AS(
        pnp::to_u8string(strict, static_cast<colour>(600)), pnp::print_exception
    );
    CHECK(not succeeded(pnp::try_parse<colour>(u8"#600", strict)));

    // A policy can forbid them by default.
    const pnp::support::listed_enum_pnp<colour, strict_enum_policy>
        policy_strict;
    CHECK_THROWS_AS(
        pnp::to_u8string(policy_strict, static_cast<colour>(600)),
        pnp::print_exception
    );
}

TEST_CASE(
    "enums: bool-based enums",
    "[enums]"
)
{
    // Only "on" is listed, so "off" prints as its underlying bool.
    const pnp::support::listed_enum_pnp<lights> partial;
    check_enum_round_trip(partial, lights::on, u8"on");
    check_enum_round_trip(partial, lights::off, u8"#false");

    // The defaults behave the same way.
    CHECK(pnp::to_u8string(lights::off) == u8"#false");

    // With both listed, both print by name.
    const pnp::support::listed_enum_pnp<switch_state> full;
    check_enum_round_trip(full, switch_state::down, u8"down");
    check_enum_round_trip(full, switch_state::up, u8"up");

    // The underlying part is a bool pnp, configured like any other.
    using lights_pnp = pnp::support::listed_enum_pnp<lights>;
    const lights_pnp digits(
        {.underlying = lights_pnp::underlying_part_type(
             {.true_value = u8"1", .false_value = u8"0"}
         )}
    );
    check_enum_round_trip(digits, lights::off, u8"#0");
}

TEST_CASE(
    "enums: parse errors list every accepted form",
    "[enums]"
)
{
    const auto unknown = pnp::try_parse<colour>(u8"purple");
    CHECK(not succeeded(unknown));
    CHECK(
        first_message(unknown)
        == u8"Expected one of \"red\", \"green\", \"blue\" or \"reddish\", or "
           u8"a value after \"#\"."
    );

    // Without unlisted values, the prefix isn't mentioned.
    const colour_pnp strict({.allow_unlisted = false});
    const auto strict_unknown = pnp::try_parse<colour>(u8"purple", strict);
    CHECK(
        first_message(strict_unknown)
        == u8"Expected one of \"red\", \"green\", \"blue\" or \"reddish\"."
    );

    // A single listed name, and the custom prefix in the message.
    using lights_pnp = pnp::support::listed_enum_pnp<lights>;
    const lights_pnp tilde_lights({.unlisted_prefix = u8"~"});
    const auto       lights_unknown
        = pnp::try_parse<lights>(u8"maybe", tilde_lights);
    CHECK(
        first_message(lights_unknown)
        == u8"Expected one of \"on\", or a value after \"~\"."
    );

    // Case-sensitive, and no whitespace skipping.
    CHECK(not succeeded(pnp::try_parse<colour>(u8"Red")));
    CHECK(succeeded(pnp::try_parse<colour>(u8" red")));
    CHECK(not succeeded(pnp::try_parse<colour>(u8"")));
}

TEST_CASE(
    "enums: invalid options throw on construction",
    "[enums]"
)
{
    // The prefix must follow the token rules...
    CHECK_THROWS_AS(
        colour_pnp({.unlisted_prefix = u8""}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        colour_pnp({.unlisted_prefix = u8" #"}), std::invalid_argument
    );
    CHECK_THROWS_AS(
        colour_pnp({.unlisted_prefix = half_a_character}), std::invalid_argument
    );

    // ...and must not start any name ("r" starts "red" and "reddish").
    CHECK_THROWS_AS(
        colour_pnp({.unlisted_prefix = u8"r"}), std::invalid_argument
    );
    CHECK(
        invalid_argument_message(
            []
            {
                colour_pnp p({.unlisted_prefix = u8"r"});
            }
        )
        == "listed_enum_pnp_options unlisted prefix must not be the start of "
           "any enum_list name"
    );

    // The prefix is only checked when unlisted values are allowed.
    CHECK_NOTHROW(colour_pnp({.allow_unlisted = false, .unlisted_prefix = u8""})
    );
    CHECK_NOTHROW(colour_pnp({.allow_unlisted = false, .unlisted_prefix = u8"r"}
    ));

    // An enum with a name starting with "#" needs a different prefix, or no
    // unlisted values.
    using hashed_strict
        = pnp::support::listed_enum_pnp<hashed, strict_enum_policy>;
    CHECK_NOTHROW(
        hashed_strict({.allow_unlisted = true, .unlisted_prefix = u8"~"})
    );
    CHECK_THROWS_AS(
        hashed_strict({.allow_unlisted = true}), std::invalid_argument
    );
}

TEST_CASE(
    "enums: inside containers",
    "[enums]"
)
{
    const std::vector<colour> colours{
        colour::red, colour::reddish, static_cast<colour>(600)
    };
    const auto text = pnp::to_u8string(colours);
    CHECK(text == u8"[red,reddish,#600]");
    const auto parsed = pnp::try_parse<std::vector<colour>>(text);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == colours);
}

TEST_CASE(
    "enums: a fixed output width when every name has the same length",
    "[enums]"
)
{
    using direction_pnp = pnp::support::listed_enum_pnp<direction>;

    // Unlisted values print as a number of varying length, so allowing them
    // rules it out.
    CHECK(not direction_pnp{}.fixed_output_width().has_value());

    // Without them, every output is one of the names.
    const direction_pnp strict({.allow_unlisted = false});
    CHECK(strict.fixed_output_width() == 1);
    for (const auto value :
         {direction::north, direction::east, direction::south, direction::west})
    {
        CHECK(pnp::to_u8string(strict, value).size() == 1);
    }

    // Names of different lengths are never fixed, even with unlisted values
    // forbidden.
    CHECK(not colour_pnp({.allow_unlisted = false})
                  .fixed_output_width()
                  .has_value());

    // The length is in code units: "α" and "β" are two each.
    using greek_pnp = pnp::support::listed_enum_pnp<greek>;
    CHECK(greek_pnp({.allow_unlisted = false}).fixed_output_width() == 2);

    // A single name is trivially the same length as itself.
    using lights_pnp = pnp::support::listed_enum_pnp<lights>;
    CHECK(
        lights_pnp({.allow_unlisted = false}).fixed_output_width() == 2
    ); // "on"

    // Read through the options, kept by copies, and inherited by the defaults.
    CHECK(strict.options().fixed_output_width() == 1);
    const direction_pnp copy = strict;
    CHECK(copy.fixed_output_width() == 1);
    CHECK(
        pnp::default_printer<direction>({.allow_unlisted = false})
            .fixed_output_width()
        == 1
    );
}

namespace pnp_tests
{
// Names of different lengths, one the start of another: "red" and "reddish".
enum class shadey
{
    red,
    reddish,
    blue
};
} // namespace pnp_tests

template <>
struct pnp::enum_list<pnp_tests::shadey>
{
    static constexpr pnp::enum_element<pnp_tests::shadey> values[] = {
        {pnp_tests::shadey::red,     u8"red"    },
        {pnp_tests::shadey::reddish, u8"reddish"},
        {pnp_tests::shadey::blue,    u8"blue"   },
    };
};

template <>
inline constexpr bool pnp::enable_default_enum_pnp<pnp_tests::shadey> = true;

TEST_CASE(
    "read ahead: an enum name stops at the first character that doesn't match",
    "[io][read_ahead][enums]"
)
{
    using pnp_tests::shadey;
    {
        INFO("a short name, which is also the start of a longer one");
        // Telling red from reddish needs one character after red, and no more.
        const auto outcome
            = parse_from_pieces<shadey>({u8"red", u8"\n", u8"blue", u8"\n"});
        CHECK(outcome.succeeded);
        CHECK(outcome.value == shadey::red);
        CHECK(outcome.pieces_read == 2);
    }
    {
        INFO("a short name, split across pieces");
        const auto outcome = parse_from_pieces<shadey>(
            {u8"r", u8"e", u8"d", u8"\n", u8"blue", u8"\n"}
        );
        CHECK(outcome.succeeded);
        CHECK(outcome.value == shadey::red);
        CHECK(outcome.pieces_read == 4);
    }
    {
        INFO("the longest name");
        // Nothing after it is needed: no name is longer.
        const auto outcome
            = parse_from_pieces<shadey>({u8"reddish", u8"\n", u8"red", u8"\n"});
        CHECK(outcome.succeeded);
        CHECK(outcome.value == shadey::reddish);
        CHECK(outcome.pieces_read == 1);
    }
    {
        INFO("a name that isn't the start of any other");
        const auto outcome
            = parse_from_pieces<shadey>({u8"blue", u8"\n", u8"red", u8"\n"});
        CHECK(outcome.succeeded);
        CHECK(outcome.value == shadey::blue);
        CHECK(outcome.pieces_read == 1);
    }
}

TEST_CASE(
    "read ahead: enum values typed one per line come back straight away",
    "[io][read_ahead][value_reader]"
)
{
    using pnp_tests::shadey;
    counting_source           source({u8"red", u8"\n", u8"blue", u8"\n"});
    pnp::value_reader<shadey> reader(source);
    const auto                first = reader.next();
    REQUIRE(first.has_value());
    CHECK(first->value == shadey::red);
    CHECK(source.given == 2);
}

static_assert(pnp::support::fixed_output_width_c<
              pnp::support::listed_enum_pnp<pnp_tests::direction>>);

// --- Enum lists
// ------------------------------------------------------------------
static_assert(pnp::listed_enum_c<colour>);
static_assert(pnp::listed_enum_c<lights>);
static_assert(pnp::listed_enum_c<greek>);
static_assert(pnp::listed_enum_c<shade>, "whitespace inside a name is fine");

static_assert(not pnp::listed_enum_c<empty_list_enum>);
static_assert(not pnp::listed_enum_c<duplicate_value_enum>);
static_assert(not pnp::listed_enum_c<duplicate_name_enum>);
static_assert(not pnp::listed_enum_c<spaced_name_enum>);
static_assert(not pnp::listed_enum_c<empty_name_enum>);

// Each invalid list reports the rule it breaks.
static_assert(
    std::string_view(pnp::enum_list_error<empty_list_enum>())
    == "enum_list must contain at least one entry"
);
static_assert(
    std::string_view(pnp::enum_list_error<duplicate_value_enum>())
    == "enum_list values must be unique"
);
static_assert(
    std::string_view(pnp::enum_list_error<duplicate_name_enum>())
    == "enum_list names must be unique"
);
static_assert(
    std::string_view(pnp::enum_list_error<spaced_name_enum>())
    == "enum_list names must not start or end with whitespace"
);
static_assert(
    std::string_view(pnp::enum_list_error<empty_name_enum>())
    == "enum_list names must be non-empty"
);
static_assert(pnp::enum_list_error<colour>() == nullptr);

// --- Defaults
// ----------------------------------------------------------------------
// Listing an enum and opting it in gives it defaults; listing alone doesn't.
static_assert(pnp::default_printable_c<colour>);
static_assert(pnp::default_parseable_c<colour>);
static_assert(not pnp::default_printable_c<hashed>, "listed, but not opted in");

// --- Policies
// ----------------------------------------------------------------------
static_assert(pnp::support::valid_listed_enum_pnp_c<
              colour,
              pnp::support::listed_enum_pnp_default>);
static_assert(
    not pnp::support::
        valid_listed_enum_pnp_c<hashed, pnp::support::listed_enum_pnp_default>,
    "the default prefix starts the name \"#tag\""
);
static_assert(pnp::support::
                  valid_listed_enum_pnp_c<hashed, strict_enum_policy>);

// --- The compile-time tables
// -------------------------------------------------------
static_assert(
    pnp::support::internal::enum_parse_order<colour>[0] == 3,
    "\"reddish\" is tried first"
);

// --- Which part handles unlisted values
// --------------------------------------------
static_assert(std::same_as<
              colour_pnp::underlying_part_type,
              pnp::support::integer_pnp<int>>);
static_assert(std::same_as<
              pnp::support::listed_enum_pnp<lights>::underlying_part_type,
              pnp::support::bool_pnp<>>);

// --- The exact-type guard ----------------------------------------------------
static_assert(pnp::printer_c<colour_pnp, colour>);
static_assert(not pnp::printer_c<colour_pnp, int>);
static_assert(not pnp::printer_c<colour_pnp, level>);
static_assert(pnp::parser_c<colour_pnp, colour>);
static_assert(not pnp::parser_c<colour_pnp, int>);