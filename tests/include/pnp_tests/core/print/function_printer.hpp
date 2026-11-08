#pragma once

#include "pnp/core/print/free_functions.hpp"
#include "pnp/core/print/function_printer.hpp"
#include "pnp/core/print/text_sink.hpp"

#include <catch2/catch_test_macros.hpp>
#include <concepts>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace function_printer_tests
{
using PNP_NAMESPACE::function_printer;
using PNP_NAMESPACE::function_printer_callable_c;
using PNP_NAMESPACE::printer_c;
using PNP_NAMESPACE::string_sink;
using PNP_NAMESPACE::text_sink;

template <typename P, typename T>
std::u8string
    print_with(
        const P& printer_arg,
        const T& value_arg
    )
{
    std::u8string              out;
    string_sink<std::u8string> sink{out};
    printer_arg.run_printer(value_arg, sink);
    return out;
}

template <typename T, typename F>
concept can_make_function_printer = requires (F&& callable_arg) {
    function_printer<T>(std::forward<F>(callable_arg));
};

struct construction_counts
{
    int copies = 0;
    int moves  = 0;
};

struct tracked_callable
{
    construction_counts* counts;

    explicit tracked_callable(
        construction_counts& counts_arg
    ) noexcept
        : counts(&counts_arg)
    {}

    tracked_callable(
        const tracked_callable& other_arg
    ) noexcept
        : counts(other_arg.counts)
    {
        ++counts->copies;
    }

    tracked_callable(
        tracked_callable&& other_arg
    ) noexcept
        : counts(other_arg.counts)
    {
        ++counts->moves;
    }

    tracked_callable&
        operator=(const tracked_callable&)
        = default;
    tracked_callable&
        operator=(tracked_callable&&)
        = default;

    void
        operator()(
            const int& value_arg,
            text_sink& sink_arg
        ) const
    {
        sink_arg.write(std::to_string(value_arg));
    }
};

struct throwing_move_callable
{
    throwing_move_callable() = default;

    throwing_move_callable(const throwing_move_callable&) noexcept(false)
    {}

    throwing_move_callable(throwing_move_callable&&) noexcept(false)
    {}

    void
            operator()(
                const int&,
                text_sink&
                ) const
    {}
};

struct test_failure : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

inline void
    print_stars(
        const int& value_arg,
        text_sink& sink_arg
    )
{
    for (int i = 0; i < value_arg; ++i)
    {
        sink_arg.put(u8'*');
    }
}

inline constexpr auto constexpr_printer = function_printer<int>(
    [](const int& value_arg, text_sink& sink_arg)
    {
        sink_arg.write(std::to_string(value_arg));
    }
);

// ---- Concept: accepted ----

static_assert(function_printer_callable_c<int, decltype(&print_stars)>);
static_assert(function_printer_callable_c<int, tracked_callable>);
static_assert(function_printer_callable_c<
        int,
        decltype([](const int&, text_sink&) {})
    > );
static_assert(function_printer_callable_c<
        int,
        decltype([](int, text_sink&) {})
    > );

// ---- Concept: rejected ----

static_assert(not function_printer_callable_c<
        int,
        decltype([](const int&, text_sink&) { return 1; })
    > );
static_assert(not function_printer_callable_c<
        int,
        decltype([](const int&, text_sink&) { return std::u8string{}; })
    > );
static_assert(not function_printer_callable_c<
        int,
        decltype([](int&, text_sink&) {})
    > );
static_assert(not function_printer_callable_c<
        int,
        decltype([](const int&) {})
    > );
static_assert(not function_printer_callable_c<
        int,
        decltype([](text_sink&, const int&) {})
    > );
static_assert(not function_printer_callable_c<
        int,
        decltype([n = 0](const int&, text_sink&) mutable { ++n; })
    > );

static_assert(not can_make_function_printer<
        int,
        decltype([](const int&, text_sink&) { return 1; })
    > );
static_assert(not can_make_function_printer<
        int,
        decltype([](int&, text_sink&) {})
    > );

// ---- func_printer properties ----

static_assert(std::is_nothrow_constructible_v<
              pnp::PNP_INTERNAL_NAMESPACE::func_printer<int, tracked_callable>,
              tracked_callable>);
static_assert(not std::is_nothrow_constructible_v<
              pnp::PNP_INTERNAL_NAMESPACE::
                  func_printer<int, throwing_move_callable>,
              throwing_move_callable>);
static_assert(not std::is_convertible_v<
              tracked_callable,
              pnp::PNP_INTERNAL_NAMESPACE::
                  func_printer<int, tracked_callable>>);
static_assert(not std::is_default_constructible_v<
              pnp::PNP_INTERNAL_NAMESPACE::
                  func_printer<int, tracked_callable>>);
static_assert(std::is_copy_constructible_v<
              pnp::PNP_INTERNAL_NAMESPACE::
                  func_printer<int, tracked_callable>>);
static_assert(printer_c<std::remove_cvref_t<decltype(constexpr_printer)>, int>);

TEST_CASE(
    "function_printer prints using a lambda",
    "[function_printer]"
)
{
    auto printer = function_printer<int>(
        [](const int& value_arg, text_sink& sink_arg)
        {
            sink_arg.write(u8"<");
            sink_arg.write(std::to_string(value_arg));
            sink_arg.write(u8">");
        }
    );

    REQUIRE(print_with(printer, 42) == u8"<42>");
    REQUIRE(print_with(printer, -7) == u8"<-7>");
}

TEST_CASE(
    "function_printer that writes nothing produces empty output",
    "[function_printer]"
)
{
    auto printer = function_printer<int>(
        [](const int&, text_sink&)
        {
        }
    );

    REQUIRE(print_with(printer, 1).empty());
}

TEST_CASE(
    "function_printer writes non-ASCII text unchanged",
    "[function_printer]"
)
{
    auto printer = function_printer<int>(
        [](const int&, text_sink& sink_arg)
        {
            sink_arg.write(u8"café ☕");
        }
    );

    REQUIRE(print_with(printer, 0) == u8"café ☕");
}

TEST_CASE(
    "function_printer accepts a function pointer",
    "[function_printer]"
)
{
    SECTION("address taken explicitly")
    {
        auto printer = function_printer<int>(&print_stars);

        STATIC_REQUIRE(std::same_as<
                       decltype(printer),
                       pnp::PNP_INTERNAL_NAMESPACE::func_printer<
                           int,
                           void (*)(const int&, text_sink&)>>);
        REQUIRE(print_with(printer, 3) == u8"***");
    }

    SECTION("function name decays to a pointer")
    {
        auto printer = function_printer<int>(print_stars);

        STATIC_REQUIRE(std::same_as<
                       decltype(printer),
                       pnp::PNP_INTERNAL_NAMESPACE::func_printer<
                           int,
                           void (*)(const int&, text_sink&)>>);
        REQUIRE(print_with(printer, 0).empty());
        REQUIRE(print_with(printer, 5) == u8"*****");
    }
}

TEST_CASE(
    "function_printer copies lvalues and moves rvalues",
    "[function_printer]"
)
{
    construction_counts counts;

    SECTION("lvalue is copied once")
    {
        tracked_callable callable{counts};
        auto             printer = function_printer<int>(callable);

        REQUIRE(counts.copies == 1);
        REQUIRE(counts.moves == 0);
        REQUIRE(print_with(printer, 12) == u8"12");
    }

    SECTION("const lvalue is copied once")
    {
        const tracked_callable callable{counts};
        auto                   printer = function_printer<int>(callable);

        REQUIRE(counts.copies == 1);
        REQUIRE(counts.moves == 0);
        REQUIRE(print_with(printer, 12) == u8"12");
    }

    SECTION("rvalue is moved once")
    {
        auto printer = function_printer<int>(tracked_callable{counts});

        REQUIRE(counts.copies == 0);
        REQUIRE(counts.moves == 1);
        REQUIRE(print_with(printer, 12) == u8"12");
    }

    SECTION("std::move of an lvalue is moved once")
    {
        tracked_callable callable{counts};
        auto             printer = function_printer<int>(std::move(callable));

        REQUIRE(counts.copies == 0);
        REQUIRE(counts.moves == 1);
        REQUIRE(print_with(printer, 12) == u8"12");
    }
}

TEST_CASE(
    "function_printer accepts a move-only callable",
    "[function_printer]"
)
{
    auto printer = function_printer<int>(
        [prefix = std::make_unique<std::u8string>(u8"#"
         )](const int& value_arg, text_sink& sink_arg)
        {
            sink_arg.write(*prefix);
            sink_arg.write(std::to_string(value_arg));
        }
    );

    STATIC_REQUIRE(not std::is_copy_constructible_v<decltype(printer)>);
    STATIC_REQUIRE(std::is_move_constructible_v<decltype(printer)>);

    REQUIRE(print_with(printer, 9) == u8"#9");

    auto moved = std::move(printer);
    REQUIRE(print_with(moved, 10) == u8"#10");
}

TEST_CASE(
    "function_printer uses captured settings",
    "[function_printer]"
)
{
    const std::u8string open    = u8"[";
    const std::u8string close   = u8"]";

    auto                printer = function_printer<int>(
        [open, close](const int& value_arg, text_sink& sink_arg)
        {
            sink_arg.write(open);
            sink_arg.write(std::to_string(value_arg));
            sink_arg.write(close);
        }
    );

    REQUIRE(print_with(printer, 5) == u8"[5]");
}

TEST_CASE(
    "function_printer can update state outside itself",
    "[function_printer]"
)
{
    int  calls   = 0;
    auto printer = function_printer<int>(
        [&calls](const int&, text_sink& sink_arg)
        {
            ++calls;
            sink_arg.put(u8'x');
        }
    );

    REQUIRE(print_with(printer, 0) == u8"x");
    REQUIRE(print_with(printer, 0) == u8"x");
    REQUIRE(calls == 2);
}

TEST_CASE(
    "function_printer copies print the same",
    "[function_printer]"
)
{
    auto printer = function_printer<int>(&print_stars);
    auto copy    = printer;

    REQUIRE(print_with(copy, 2) == print_with(printer, 2));
}

TEST_CASE(
    "function_printer can be built at compile time",
    "[function_printer]"
)
{
    REQUIRE(print_with(constexpr_printer, 77) == u8"77");
}

TEST_CASE(
    "function_printer passes exceptions through unchanged",
    "[function_printer]"
)
{
    auto printer = function_printer<int>(
        [](const int& value_arg, text_sink& sink_arg)
        {
            sink_arg.write(u8"partial");
            if (value_arg < 0)
            {
                throw test_failure("negative");
            }
            sink_arg.write(u8" done");
        }
    );

    SECTION("run_printer leaves partial output in the sink")
    {
        std::u8string              out;
        string_sink<std::u8string> sink{out};

        REQUIRE_THROWS_AS(printer.run_printer(-1, sink), test_failure);
        REQUIRE(out == u8"partial");
    }

    SECTION("no exception prints everything")
    {
        REQUIRE(print_with(printer, 1) == u8"partial done");
    }

    SECTION("to_u8string lets the exception through")
    {
        REQUIRE_THROWS_AS(
            PNP_NAMESPACE::to_u8string(printer, -1), test_failure
        );
    }
}

TEST_CASE(
    "function_printer works through to_u8string",
    "[function_printer]"
)
{
    auto printer = function_printer<int>(&print_stars);

    REQUIRE(PNP_NAMESPACE::to_u8string(printer, 4) == u8"****");
}
} // namespace function_printer_tests