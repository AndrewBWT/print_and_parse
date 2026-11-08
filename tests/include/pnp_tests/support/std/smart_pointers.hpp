#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace pnp_tests
{
struct smart_pointer_test_deleter
{
    void
        operator()(
            int* pointer_arg
        ) const noexcept
    {
        delete pointer_arg;
    }
};

using unique_int_printer    = pnp::default_printer<std::unique_ptr<int>>;
using unique_int_parser     = pnp::default_parser<std::unique_ptr<int>>;

using nested_unique         = std::unique_ptr<std::unique_ptr<int>>;
using nested_unique_printer = pnp::default_printer<nested_unique>;
using nested_unique_parser  = pnp::default_parser<nested_unique>;

// A null token that starts with an opening parenthesis.
struct parenthesised_null
{
    static constexpr std::u8string_view null_id = u8"(null)";
};

// The null token starts with the opening delimiter: the parser couldn't tell
// them apart.
struct confusable_smart_pointer_policy
{
    using null_defaults                                 = parenthesised_null;
    static constexpr std::u8string_view open_delimiter  = u8"(";
    static constexpr std::u8string_view close_delimiter = u8")";
};

// The same null token with different delimiters: fine.
struct angled_smart_pointer_policy
{
    using null_defaults                                 = parenthesised_null;
    static constexpr std::u8string_view open_delimiter  = u8"<";
    static constexpr std::u8string_view close_delimiter = u8">";
};
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "smart pointers: unique_ptr",
    "[smart_pointers]"
)
{
    CHECK(pnp::to_u8string(std::make_unique<int>(5)) == u8"unique_ptr(5)");
    CHECK(pnp::to_u8string(std::unique_ptr<int>{}) == u8"nullptr");
    CHECK(
        pnp::to_u8string(std::make_unique<std::string>("a,b"))
        == u8"unique_ptr(\"a,b\")"
    );
    CHECK(
        pnp::to_u8string(
            std::make_unique<std::vector<int>>(std::vector<int>{1, 2})
        )
        == u8"unique_ptr([1,2])"
    );
    CHECK(
        pnp::to_u8string(std::make_unique<const int>(7)) == u8"unique_ptr(7)"
    );

    const auto value = pnp::try_parse<std::unique_ptr<int>>(u8"unique_ptr(5)");
    REQUIRE(succeeded(value));
    REQUIRE(value.value != nullptr);
    CHECK(*value.value == 5);

    const auto null = pnp::try_parse<std::unique_ptr<int>>(u8"nullptr");
    REQUIRE(succeeded(null));
    CHECK(null.value == nullptr);

    const auto text
        = pnp::try_parse<std::unique_ptr<std::string>>(u8"unique_ptr(\"hi\")");
    REQUIRE(succeeded(text));
    CHECK(*text.value == "hi");

    // A custom deleter is still a unique_ptr, and prints as one.
    const std::unique_ptr<int, smart_pointer_test_deleter> custom(new int(9));
    CHECK(pnp::to_u8string(custom) == u8"unique_ptr(9)");
}

TEST_CASE(
    "smart pointers: shared_ptr",
    "[smart_pointers]"
)
{
    CHECK(pnp::to_u8string(std::make_shared<int>(5)) == u8"shared_ptr(5)");
    CHECK(pnp::to_u8string(std::shared_ptr<int>{}) == u8"nullptr");

    const auto value = pnp::try_parse<std::shared_ptr<int>>(u8"shared_ptr(42)");
    REQUIRE(succeeded(value));
    REQUIRE(value.value != nullptr);
    CHECK(*value.value == 42);
    CHECK(value.value.use_count() == 1); // a new, unshared pointee

    const auto null = pnp::try_parse<std::shared_ptr<int>>(u8"nullptr");
    REQUIRE(succeeded(null));
    CHECK(null.value == nullptr);
}

TEST_CASE(
    "smart pointers: weak_ptr prints what it points to",
    "[smart_pointers]"
)
{
    auto                     shared = std::make_shared<int>(5);
    const std::weak_ptr<int> weak   = shared;
    CHECK(pnp::to_u8string(weak) == u8"weak_ptr(5)");

    shared.reset();
    CHECK(pnp::to_u8string(weak) == u8"nullptr"); // expired
    CHECK(pnp::to_u8string(std::weak_ptr<int>{}) == u8"nullptr");
}

TEST_CASE(
    "smart pointers: each pointer names itself",
    "[smart_pointers]"
)
{
    // The text says what it was, so one pointer's text doesn't parse as
    // another's.
    CHECK(not succeeded(pnp::try_parse<std::unique_ptr<int>>(u8"shared_ptr(5)"))
    );
    CHECK(not succeeded(pnp::try_parse<std::shared_ptr<int>>(u8"unique_ptr(5)"))
    );

    // A null pointer is the same token for every kind.
    CHECK(succeeded(pnp::try_parse<std::unique_ptr<int>>(u8"nullptr")));
    CHECK(succeeded(pnp::try_parse<std::shared_ptr<int>>(u8"nullptr")));

    // A pointer type without its own default policy gets the generic name.
    using generic_pnp = pnp::support::generic_smart_pointer_pnp<
        std::unique_ptr<int>,
        pnp::default_printer<int>>;
    CHECK(
        pnp::to_u8string(generic_pnp{}, std::make_unique<int>(5))
        == u8"smart_pointer(5)"
    );
}

TEST_CASE(
    "smart pointers: nested pointers keep all three states apart",
    "[smart_pointers]"
)
{
    // The outer pointer null, the inner one null, and both set: each prints
    // differently.
    const nested_unique outer_null{};
    const nested_unique inner_null = std::make_unique<std::unique_ptr<int>>();
    const nested_unique both_set
        = std::make_unique<std::unique_ptr<int>>(std::make_unique<int>(5));

    CHECK(pnp::to_u8string(outer_null) == u8"nullptr");
    CHECK(pnp::to_u8string(inner_null) == u8"unique_ptr(nullptr)");
    CHECK(pnp::to_u8string(both_set) == u8"unique_ptr(unique_ptr(5))");

    // And each parses back to the same state.
    const auto parsed_outer_null = pnp::try_parse<nested_unique>(u8"nullptr");
    REQUIRE(succeeded(parsed_outer_null));
    CHECK(parsed_outer_null.value == nullptr);

    const auto parsed_inner_null
        = pnp::try_parse<nested_unique>(u8"unique_ptr(nullptr)");
    REQUIRE(succeeded(parsed_inner_null));
    REQUIRE(parsed_inner_null.value != nullptr);
    CHECK(*parsed_inner_null.value == nullptr);

    const auto parsed_both_set
        = pnp::try_parse<nested_unique>(u8"unique_ptr(unique_ptr(5))");
    REQUIRE(succeeded(parsed_both_set));
    REQUIRE(parsed_both_set.value != nullptr);
    REQUIRE(*parsed_both_set.value != nullptr);
    CHECK(**parsed_both_set.value == 5);
}

TEST_CASE(
    "smart pointers: inside containers",
    "[smart_pointers]"
)
{
    std::vector<std::unique_ptr<int>> pointers;
    pointers.push_back(std::make_unique<int>(1));
    pointers.push_back(nullptr);
    pointers.push_back(std::make_unique<int>(3));
    CHECK(
        pnp::to_u8string(pointers) == u8"[unique_ptr(1),nullptr,unique_ptr(3)]"
    );

    // Move-only elements parse too.
    const auto parsed = pnp::try_parse<std::vector<std::unique_ptr<int>>>(
        u8"[unique_ptr(1),nullptr,unique_ptr(3)]"
    );
    REQUIRE(succeeded(parsed));
    REQUIRE(parsed.value.size() == 3);
    CHECK(*parsed.value[0] == 1);
    CHECK(parsed.value[1] == nullptr);
    CHECK(*parsed.value[2] == 3);
}

TEST_CASE(
    "smart pointers: configuration",
    "[smart_pointers]"
)
{
    // Different delimiters, on both the printer and the parser.
    const unique_int_printer starred(
        {.open_delimiter = u8"*(", .close_delimiter = u8")"}
    );
    CHECK(pnp::to_u8string(starred, std::make_unique<int>(5)) == u8"*(5)");
    CHECK(pnp::to_u8string(starred, std::unique_ptr<int>{}) == u8"nullptr");

    const unique_int_parser starred_parser(
        {.open_delimiter = u8"*(", .close_delimiter = u8")"}
    );
    const auto parsed
        = pnp::try_parse<std::unique_ptr<int>>(u8"*(5)", starred_parser);
    REQUIRE(succeeded(parsed));
    CHECK(*parsed.value == 5);

    // A custom null token.
    const unique_int_printer null_word(
        {.null = unique_int_printer::null_part_type({.null_id = u8"null"})}
    );
    CHECK(pnp::to_u8string(null_word, std::unique_ptr<int>{}) == u8"null");

    // A configured pointee.
    const unique_int_printer plus(
        {.pointee = unique_int_printer::pointee_part_type(
             {.use_plus_for_positive_numbers = true}
         )}
    );
    CHECK(
        pnp::to_u8string(plus, std::make_unique<int>(5)) == u8"unique_ptr(+5)"
    );

    // Empty delimiters are still allowed, for pointees that can't print the
    // null token.
    const unique_int_printer bare(
        {.open_delimiter = u8"", .close_delimiter = u8""}
    );
    CHECK(pnp::to_u8string(bare, std::make_unique<int>(5)) == u8"5");
    const unique_int_parser bare_parser(
        {.open_delimiter = u8"", .close_delimiter = u8""}
    );
    const auto bare_parsed
        = pnp::try_parse<std::unique_ptr<int>>(u8"5", bare_parser);
    REQUIRE(succeeded(bare_parsed));
    CHECK(*bare_parsed.value == 5);
}

TEST_CASE(
    "smart pointers: empty delimiters make nested pointers ambiguous",
    "[smart_pointers]"
)
{
    // The reason for the default: without delimiters, an outer pointer holding
    // a null inner pointer prints exactly as a null outer pointer, and reads
    // back as one.
    const nested_unique_printer bare(
        {.open_delimiter = u8"", .close_delimiter = u8""}
    );
    const nested_unique inner_null = std::make_unique<std::unique_ptr<int>>();
    CHECK(pnp::to_u8string(bare, inner_null) == u8"nullptr");
    CHECK(pnp::to_u8string(bare, nested_unique{}) == u8"nullptr");

    const nested_unique_parser bare_parser(
        {.open_delimiter = u8"", .close_delimiter = u8""}
    );
    const auto parsed = pnp::try_parse<nested_unique>(u8"nullptr", bare_parser);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == nullptr); // the outer pointer, not the inner one
}

TEST_CASE(
    "smart pointers: the opening delimiter and null token must be "
    "distinguishable",
    "[smart_pointers]"
)
{
    // A null token that starts with the opening delimiter.
    CHECK(
        invalid_argument_message(
            []
            {
                unique_int_printer p({
                    .null
                    = unique_int_printer::null_part_type({.null_id = u8"(null)"}
                    ),
                    .open_delimiter = u8"(",
                });
            }
        )
        == "smart_pointer_pnp_options open delimiter and null token must not "
           "be prefixes of each other"
    );

    // A null token that's a prefix of the default delimiter, unique_ptr(.
    CHECK_THROWS_AS(
        unique_int_printer(
            {.null = unique_int_printer::null_part_type({.null_id = u8"unique"})
            }
        ),
        std::invalid_argument
    );

    // An opening delimiter that starts with the null token, or is a prefix of
    // it.
    CHECK_THROWS_AS(
        unique_int_printer({.open_delimiter = u8"nullptr!"}),
        std::invalid_argument
    );
    CHECK_THROWS_AS(
        unique_int_printer({.open_delimiter = u8"null"}), std::invalid_argument
    );

    // Different from the first character: fine, including "(null)" against the
    // default.
    CHECK_NOTHROW(
        unique_int_printer({.open_delimiter = u8"<", .close_delimiter = u8">"})
    );
    CHECK_NOTHROW(unique_int_printer(
        {.null = unique_int_printer::null_part_type({.null_id = u8"(null)"})}
    ));

    // With empty delimiters, the rule doesn't apply.
    CHECK_NOTHROW(unique_int_printer({
        .null = unique_int_printer::null_part_type({.null_id = u8"(null)"}),
        .open_delimiter  = u8"",
        .close_delimiter = u8"",
    }));

    // Invalid delimiters still throw for their own reasons.
    CHECK(
        invalid_argument_message(
            []
            {
                unique_int_printer p({.open_delimiter = u8" *"});
            }
        )
        == "smart_pointer_pnp_options delimiters must not start or end with "
           "whitespace"
    );
    CHECK(
        invalid_argument_message(
            []
            {
                unique_int_printer p({.close_delimiter = half_a_character});
            }
        )
        == "smart_pointer_pnp_options delimiters must be valid UTF-8"
    );
}

TEST_CASE(
    "smart pointers: parse errors",
    "[smart_pointers]"
)
{
    // Neither the null token nor the opening delimiter.
    const auto neither = pnp::try_parse<std::unique_ptr<int>>(u8"5");
    CHECK(not succeeded(neither));
    CHECK(
        first_message(neither)
        == u8"Expected the null token \"nullptr\" or the opening delimiter "
           u8"\"unique_ptr(\"."
    );

    // A bad pointee keeps its own message.
    const auto bad = pnp::try_parse<std::unique_ptr<int>>(u8"unique_ptr(x)");
    CHECK(not succeeded(bad));
    CHECK(
        first_message(bad)
        == u8"Expected a valid integer, but no valid digits were found."
    );

    // A missing closing delimiter.
    const auto no_close
        = pnp::try_parse<std::unique_ptr<int>>(u8"unique_ptr(5");
    CHECK(not succeeded(no_close));
    CHECK(first_message(no_close) == u8"Expected the closing delimiter \")\".");

    // Configured delimiters appear in the messages.
    const unique_int_parser starred(
        {.open_delimiter = u8"*(", .close_delimiter = u8")"}
    );
    const auto starred_no_open
        = pnp::try_parse<std::unique_ptr<int>>(u8"(5)", starred);
    CHECK(
        first_message(starred_no_open)
        == u8"Expected the null token \"nullptr\" or the opening delimiter "
           u8"\"*(\"."
    );

    // A partial null token or delimiter, trailing input, and empty input.
    CHECK(not succeeded(pnp::try_parse<std::unique_ptr<int>>(u8"null")));
    CHECK(not succeeded(pnp::try_parse<std::unique_ptr<int>>(u8"unique(5)")));
    CHECK(not succeeded(pnp::try_parse<std::unique_ptr<int>>(u8"unique_ptr(5)x")
    ));
    CHECK(not succeeded(pnp::try_parse<std::unique_ptr<int>>(u8"nullptrx")));
    CHECK(not succeeded(pnp::try_parse<std::unique_ptr<int>>(u8"")));
}

TEST_CASE(
    "smart pointers: never fixed width",
    "[smart_pointers][fixed_width]"
)
{
    // A null pointer and a set one print at different lengths, even with a
    // fixed-width pointee.
    CHECK(not unique_int_printer{}.fixed_output_width().has_value());
    CHECK(not pnp::default_printer<std::unique_ptr<bool>>(
                  {.pointee = pnp::default_printer<bool>(
                       {.true_value = u8"1", .false_value = u8"0"}
                   )}
    )
                  .fixed_output_width()
                  .has_value());

    // So a container of them keeps its separator.
    using pointers_pnp
        = pnp::default_printer<std::vector<std::unique_ptr<int>>>;
    CHECK_THROWS_AS(pointers_pnp({.separator = u8""}), std::invalid_argument);
}

// --- The shared token-or-value rule
// -----------------------------------------------------
static_assert(pnp::support::internal::token_and_delimiter_distinguishable(
    u8"nullptr",
    u8"unique_ptr("
));
static_assert(
    pnp::support::internal::token_and_delimiter_distinguishable(
        u8"nullptr",
        u8""
    ),
    "empty delimiters are allowed"
);
static_assert(not pnp::support::internal::token_and_delimiter_distinguishable(
    u8"(null)",
    u8"("
));
static_assert(not pnp::support::internal::token_and_delimiter_distinguishable(
    u8"null",
    u8"nullptr!"
));
static_assert(
    not pnp::support::internal::token_and_delimiter_distinguishable(
        u8"x",
        u8"x"
    ),
    "identical"
);

// --- Policies
// ------------------------------------------------------------------------ A
// policy is checked with a pointee, on the real default options, so every rule
// applies.
static_assert(pnp::support::valid_smart_pointer_pnp_c<
              pnp::default_printer<int>,
              pnp::support::smart_pointer_pnp_default>);
static_assert(pnp::support::valid_smart_pointer_pnp_c<
              pnp::default_printer<int>,
              pnp::support::unique_ptr_pnp_default>);
static_assert(pnp::support::valid_smart_pointer_pnp_c<
              pnp::default_printer<int>,
              pnp::support::shared_ptr_pnp_default>);
static_assert(pnp::support::valid_smart_pointer_pnp_c<
              pnp::default_printer<int>,
              pnp::support::weak_ptr_pnp_default>);
static_assert(
    not pnp::support::valid_smart_pointer_pnp_c<
        pnp::default_printer<int>,
        confusable_smart_pointer_policy>,
    "the policy's null token and opening delimiter are checked against each "
    "other"
);
static_assert(pnp::support::valid_smart_pointer_pnp_c<
              pnp::default_printer<int>,
              angled_smart_pointer_policy>);
static_assert(pnp::support::checkable_options_c<
              unique_int_printer::options_type>);

// --- Defaults
// ------------------------------------------------------------------------
static_assert(pnp::default_printable_c<std::unique_ptr<int>>);
static_assert(pnp::default_parseable_c<std::unique_ptr<int>>);
static_assert(pnp::default_printable_c<std::shared_ptr<int>>);
static_assert(pnp::default_parseable_c<std::shared_ptr<int>>);
static_assert(pnp::default_printable_c<std::weak_ptr<int>>);
static_assert(
    not pnp::default_parseable_c<std::weak_ptr<int>>,
    "a weak_ptr can't own what it points to"
);
static_assert(pnp::default_parseable_c<nested_unique>);

// A custom deleter prints, but has no factory, so doesn't parse.
static_assert(pnp::default_printable_c<
              std::unique_ptr<int, smart_pointer_test_deleter>>);
static_assert(not pnp::default_parseable_c<
              std::unique_ptr<int, smart_pointer_test_deleter>>);

// Arrays have no known size.
static_assert(not pnp::default_printable_c<std::unique_ptr<int[]>>);

// --- The exact-type guard
// ------------------------------------------------------------
static_assert(not pnp::printer_c<unique_int_printer, std::shared_ptr<int>>);
static_assert(not pnp::printer_c<unique_int_printer, int*>);
static_assert(not pnp::printer_c<
              pnp::default_printer<std::shared_ptr<int>>,
              std::weak_ptr<int>>);

static_assert(
    std::is_base_of_v<
        pnp::support::generic_weak_pointer_printer<
            std::weak_ptr<int>,
            pnp::default_printer<int>,
            pnp::support::weak_ptr_pnp_default>,
        pnp::default_printer<std::weak_ptr<int>>>,
    "a weak_ptr prints through what it locks to"
);