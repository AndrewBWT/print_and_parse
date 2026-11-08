#pragma once
#include "pnp/core/parse/runtime_parser_container.hpp"
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <concepts>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using namespace pnp_tests;

namespace
{
    // ============================================================ helpers

    // Matches token, then sets the value to result. Copyable, so it can be
    // wrapped.
    struct literal_pnp
    {
        std::u8string token;
        int           result = 0;

        pnp::basic_parser_return_value
            run_parser(
                pnp::parser_input& input_arg,
                int& value_arg,
                pnp::parser_run_key
            ) const
        {
            if (not input_arg.advance_if_starts_with(token))
            {
                return input_arg.fail(u8"expected " + token);
            }
            value_arg = result;
            return {};
        }
    };

    // A pnp that can't be copied, so it can't be cloned.
    struct move_only_pnp
    {
        std::unique_ptr<int> state = std::make_unique<int>(0);

        pnp::basic_parser_return_value
            run_parser(
                pnp::parser_input&,
                int&,
                pnp::parser_run_key
            ) const
        {
            return {};
        }
    };

    using int_container = pnp::runtime_parser_container<int>;

    // Runs container_arg over text_arg; returns the result, the value and where
    // the input ended up.
    struct run_result
    {
        pnp::basic_parser_return_value result;
        int                            value = 0;
        std::size_t                    position = 0;
    };

    run_result
        run_over(
            const int_container& container_arg,
            const std::u8string_view text_arg,
            const int                start_value_arg = -1
        )
    {
        pnp::parser_input input(text_arg);
        run_result        outcome{ .value = start_value_arg };
        outcome.result = input.run_parser(container_arg, outcome.value);
        outcome.position = input.position();
        return outcome;
    }

} // namespace

// ============================================================ types

TEST_CASE(
    "runtime_parser_container: types",
    "[runtime_parser_container]"
)
{
    // It is a pnp itself.
    STATIC_CHECK(pnp::parser_c<int_container, int>);

    STATIC_CHECK(std::is_copy_constructible_v<int_container>);
    STATIC_CHECK(std::is_copy_assignable_v<int_container>);
    //STATIC_CHECK(std::is_nothrow_move_constructible_v<int_container>);
   // STATIC_CHECK(std::is_nothrow_move_assignable_v<int_container>);

    // Default: wraps default_parser<int>.
    STATIC_CHECK(std::is_default_constructible_v<int_container>);

    // Takes any copyable pnp for the same type, implicitly.
    STATIC_CHECK(std::is_constructible_v<int_container, literal_pnp>);
    STATIC_CHECK(std::is_convertible_v<literal_pnp, int_container>);

    // A pnp for a different type isn't accepted.
    STATIC_CHECK(
        not std::is_constructible_v<pnp::runtime_parser_container<double>, literal_pnp>
    );

    // A move-only pnp is a pnp, but can't be cloned, so it's rejected.
    STATIC_CHECK(pnp::parser_c<move_only_pnp, int>);
    STATIC_CHECK(not std::is_constructible_v<int_container, move_only_pnp>);
}

// ============================================================ running

TEST_CASE(
    "runtime_parser_container: runs the pnp it holds",
    "[runtime_parser_container]"
)
{
    const int_container container(literal_pnp{ u8"yes", 1 });

    // Success: the pnp's value and position.
    {
        const auto outcome = run_over(container, u8"yes!");
        CHECK(outcome.result.succeeded());
        CHECK(outcome.value == 1);
        CHECK(outcome.position == 3);
    }
    // Failure: the pnp's own failure, passed through unchanged.
    {
        const auto outcome = run_over(container, u8"no", 7);
        CHECK(outcome.result.failed());
        CHECK(outcome.result.position() == 0);
        REQUIRE(outcome.result.messages().size() == 1);
        CHECK(outcome.result.messages()[0] == u8"expected yes");
        CHECK(outcome.value == 7); // left alone
    }
}

TEST_CASE(
    "runtime_parser_container: the default wraps default_parser",
    "[runtime_parser_container]"
)
{
    const int_container container;
    const auto          outcome = run_over(container, u8"42");
    CHECK(outcome.result.succeeded());
    CHECK(outcome.value == 42);
    CHECK(outcome.position == 2);
}

TEST_CASE(
    "runtime_parser_container: chosen at runtime",
    "[runtime_parser_container]"
)
{
    // Different pnp types, one container type.
    const std::vector<int_container> choices{
        literal_pnp{u8"one", 1},
        literal_pnp{u8"two", 2},
        int_container(), // default_parser<int>
    };
    CHECK(run_over(choices[0], u8"one").value == 1);
    CHECK(run_over(choices[1], u8"two").value == 2);
    CHECK(run_over(choices[2], u8"3").value == 3);
    CHECK(run_over(choices[0], u8"two").result.failed());
}

// ============================================================ copying and
// assigning

TEST_CASE(
    "runtime_parser_container: a copy is independent",
    "[runtime_parser_container]"
)
{
    int_container       original(literal_pnp{ u8"a", 1 });
    const int_container copy(original);

    // Replacing the original leaves the copy as it was.
    original = literal_pnp{ u8"b", 2 };
    CHECK(run_over(original, u8"b").value == 2);
    CHECK(run_over(copy, u8"a").value == 1);
    CHECK(run_over(copy, u8"b").result.failed());

    // Copying from a container gives a copy, not a container wrapping a
    // container: it behaves exactly the same.
    const int_container copy_of_copy = copy;
    CHECK(run_over(copy_of_copy, u8"a").value == 1);
}

TEST_CASE(
    "runtime_parser_container: assignment replaces the pnp",
    "[runtime_parser_container]"
)
{
    int_container       target(literal_pnp{ u8"a", 1 });
    const int_container source(literal_pnp{ u8"b", 2 });

    target = source;
    CHECK(run_over(target, u8"b").value == 2);
    CHECK(run_over(target, u8"a").result.failed());
    // The source is still usable.
    CHECK(run_over(source, u8"b").value == 2);

    // Assigning to itself changes nothing.
    auto& same = target;
    target = same;
    CHECK(run_over(target, u8"b").value == 2);

    // Moving in.
    target = int_container(literal_pnp{ u8"c", 3 });
    CHECK(run_over(target, u8"c").value == 3);
}

TEST_CASE(
    "runtime_parser_container: moved-from containers",
    "[runtime_parser_container]"
)
{
    int_container original(literal_pnp{ u8"a", 1 });
    int_container moved_to = std::move(original);
    CHECK(run_over(moved_to, u8"a").value == 1);

    // A moved-from container can be copied and assigned from without
    // crashing; the copies are empty too.
    CHECK_NOTHROW(int_container(original));
    int_container assigned(literal_pnp{ u8"z", 26 });
    CHECK_NOTHROW(assigned = original);

    // And it's usable again once it's given a pnp.
    original = literal_pnp{ u8"b", 2 };
    CHECK(run_over(original, u8"b").value == 2);
    assigned = original;
    CHECK(run_over(assigned, u8"b").value == 2);
    // Running an empty one is a bug in the caller, and asserts: not tested.
}