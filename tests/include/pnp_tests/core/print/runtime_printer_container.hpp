#pragma once

#include "pnp/core/print/runtime_printer_container.hpp"
#include "pnp/core/print/default_printer.hpp"
#include "pnp/core/print/function_printer.hpp"
#include "pnp/core/print/print_exception.hpp"
#include "pnp/core/print/text_sink.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace runtime_printer_container_tests
{
    struct widget
    {
        int id;
    };

    struct no_default_printer_type
    {
    };
}

PNP_NS_BEGIN

template <>
struct default_printer<runtime_printer_container_tests::widget>
{
    static constexpr bool is_specialized = true;

    void
        run_printer(
            const runtime_printer_container_tests::widget& value_arg,
            text_sink& sink_arg
        ) const
    {
        sink_arg.write(u8"widget#");
        sink_arg.write(std::to_string(value_arg.id));
    }
};

PNP_NS_END

namespace runtime_printer_container_tests
{
    using PNP_NAMESPACE::function_printer;
    using PNP_NAMESPACE::print_exception;
    using PNP_NAMESPACE::printer_c;
    using PNP_NAMESPACE::runtime_printer_container;
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
        string_sink<std::u8string> sink{ out };
        printer_arg.run_printer(value_arg, sink);
        return out;
    }

    struct tag_printer
    {
        std::u8string tag;

        void
            run_printer(
                const int& value_arg,
                text_sink& sink_arg
            ) const
        {
            sink_arg.write(tag);
            sink_arg.write(std::to_string(value_arg));
        }
    };

    struct copy_counting_printer
    {
        int* copies;

        explicit copy_counting_printer(int& copies_arg) noexcept
            : copies(&copies_arg)
        {
        }

        copy_counting_printer(const copy_counting_printer& other_arg) noexcept
            : copies(other_arg.copies)
        {
            ++*copies;
        }

        copy_counting_printer(copy_counting_printer&& other_arg) noexcept
            : copies(other_arg.copies)
        {
        }

        copy_counting_printer& operator=(const copy_counting_printer&) = default;
        copy_counting_printer& operator=(copy_counting_printer&&) = default;

        void
            run_printer(
                const int& value_arg,
                text_sink& sink_arg
            ) const
        {
            sink_arg.write(std::to_string(value_arg));
        }
    };

    struct call_counting_printer
    {
        int* calls;

        void
            run_printer(
                const int&,
                text_sink& sink_arg
            ) const
        {
            ++*calls;
            sink_arg.put(u8'x');
        }
    };

    struct move_only_printer
    {
        std::unique_ptr<int> data = std::make_unique<int>(0);

        void
            run_printer(
                const int&,
                text_sink&
            ) const
        {
        }
    };

    struct string_printer
    {
        void
            run_printer(
                const std::u8string& value_arg,
                text_sink& sink_arg
            ) const
        {
            sink_arg.write(value_arg);
        }
    };

    struct throwing_printer
    {
        void
            run_printer(
                const int& value_arg,
                text_sink& sink_arg
            ) const
        {
            sink_arg.write(u8"partial");
            if (value_arg < 0)
            {
                throw print_exception{ u8"negative" };
            }
        }
    };

    // ---- Static properties ----

    static_assert(printer_c<runtime_printer_container<int>, int>);
    static_assert(printer_c<runtime_printer_container<widget>, widget>);

    static_assert(std::is_copy_constructible_v<runtime_printer_container<int>>);
    static_assert(std::is_copy_assignable_v<runtime_printer_container<int>>);

    static_assert(std::is_default_constructible_v<runtime_printer_container<widget>>);
    static_assert(not std::is_default_constructible_v<
        runtime_printer_container<no_default_printer_type>
    >);

    static_assert(std::is_constructible_v<runtime_printer_container<int>, tag_printer>);
    static_assert(std::is_convertible_v<tag_printer, runtime_printer_container<int>>);
    static_assert(not std::is_constructible_v<
        runtime_printer_container<int>,
        move_only_printer
    >);
    static_assert(not std::is_constructible_v<
        runtime_printer_container<int>,
        string_printer
    >);
    static_assert(not std::is_constructible_v<runtime_printer_container<int>, int>);

    // ---- Construction and printing ----

    TEST_CASE("runtime_printer_container prints with the printer it holds", "[runtime_printer_container]")
    {
        const runtime_printer_container<int> container{ tag_printer{u8"#"} };

        REQUIRE(print_with(container, 5) == u8"#5");
        REQUIRE(print_with(container, -12) == u8"#-12");
    }

    TEST_CASE("runtime_printer_container default constructs with default_printer", "[runtime_printer_container]")
    {
        const runtime_printer_container<widget> container;

        REQUIRE(print_with(container, widget{ 7 }) == u8"widget#7");
    }

    TEST_CASE("runtime_printer_container holds a function_printer", "[runtime_printer_container]")
    {
        const runtime_printer_container<int> container{ function_printer<int>(
            [](const int& value_arg, text_sink& sink_arg)
            {
                sink_arg.write(u8"café ");
                sink_arg.write(std::to_string(value_arg));
            }
        ) };

        REQUIRE(print_with(container, 3) == u8"café 3");
    }

    TEST_CASE("runtime_printer_container can change printer at runtime", "[runtime_printer_container]")
    {
        runtime_printer_container<int> container{ tag_printer{u8"a"} };
        REQUIRE(print_with(container, 1) == u8"a1");

        container = tag_printer{ u8"b" };
        REQUIRE(print_with(container, 1) == u8"b1");

        container = function_printer<int>(
            [](const int&, text_sink& sink_arg)
            {
                sink_arg.write(u8"function");
            }
        );
        REQUIRE(print_with(container, 1) == u8"function");
    }

    // ---- Copying ----

    TEST_CASE("runtime_printer_container copies clone the printer once", "[runtime_printer_container]")
    {
        int                                  copies = 0;
        const runtime_printer_container<int> original{ copy_counting_printer{copies} };
        copies = 0;

        SECTION("copy construction from a const lvalue")
        {
            const runtime_printer_container<int> copy{ original };

            REQUIRE(copies == 1);
            REQUIRE(print_with(copy, 4) == u8"4");
        }

        SECTION("copy construction from a non-const lvalue isn't wrapped")
        {
            runtime_printer_container<int> source{ original };
            copies = 0;

            runtime_printer_container<int> copy{ source };

            REQUIRE(copies == 1);
            REQUIRE(print_with(copy, 4) == u8"4");
        }

        SECTION("copy assignment")
        {
            runtime_printer_container<int> copy{ tag_printer{u8"x"} };
            copy = original;

            REQUIRE(copies == 1);
            REQUIRE(print_with(copy, 4) == u8"4");
        }
    }

    TEST_CASE("runtime_printer_container copies are independent", "[runtime_printer_container]")
    {
        runtime_printer_container<int>       original{ tag_printer{u8"old"} };
        const runtime_printer_container<int> copy{ original };

        original = tag_printer{ u8"new" };

        REQUIRE(print_with(original, 1) == u8"new1");
        REQUIRE(print_with(copy, 1) == u8"old1");
    }

    TEST_CASE("runtime_printer_container self-assignment is safe", "[runtime_printer_container]")
    {
        int                            copies = 0;
        runtime_printer_container<int> container{ copy_counting_printer{copies} };
        copies = 0;

        auto& same = container;
        container = same;

        REQUIRE(copies == 0);
        REQUIRE(print_with(container, 8) == u8"8");
    }

    // ---- Never empty ----

    TEST_CASE("runtime_printer_container is never left empty by a move", "[runtime_printer_container]")
    {
        SECTION("move construction copies")
        {
            runtime_printer_container<int>       source{ tag_printer{u8"src"} };
            const runtime_printer_container<int> target{ std::move(source) };

            REQUIRE(print_with(target, 1) == u8"src1");
            REQUIRE(print_with(source, 1) == u8"src1");
        }

        SECTION("move assignment copies")
        {
            runtime_printer_container<int> source{ tag_printer{u8"src"} };
            runtime_printer_container<int> target{ tag_printer{u8"tgt"} };

            target = std::move(source);

            REQUIRE(print_with(target, 1) == u8"src1");
            REQUIRE(print_with(source, 1) == u8"src1");
        }

        SECTION("a moved-from container can still be copied")
        {
            runtime_printer_container<int>       source{ tag_printer{u8"src"} };
            const runtime_printer_container<int> target{ std::move(source) };
            const runtime_printer_container<int> copy{ source };

            REQUIRE(print_with(copy, 2) == u8"src2");
        }
    }

    // ---- State and exceptions ----

    TEST_CASE("runtime_printer_container keeps the held printer's state", "[runtime_printer_container]")
    {
        int                                  calls = 0;
        const runtime_printer_container<int> container{ call_counting_printer{&calls} };

        REQUIRE(print_with(container, 0) == u8"x");
        REQUIRE(print_with(container, 0) == u8"x");
        REQUIRE(calls == 2);

        const runtime_printer_container<int> copy{ container };
        REQUIRE(print_with(copy, 0) == u8"x");
        REQUIRE(calls == 3);
    }

    TEST_CASE("runtime_printer_container passes exceptions through", "[runtime_printer_container]")
    {
        const runtime_printer_container<int> container{ throwing_printer{} };

        std::u8string              out;
        string_sink<std::u8string> sink{ out };

        SECTION("exception and partial output")
        {
            REQUIRE_THROWS_AS(container.run_printer(-1, sink), print_exception);
            REQUIRE(out == u8"partial");
        }

        SECTION("no exception for valid values")
        {
            REQUIRE_NOTHROW(container.run_printer(1, sink));
            REQUIRE(out == u8"partial");
        }

        SECTION("the container is still usable after a throw")
        {
            REQUIRE_THROWS_AS(container.run_printer(-1, sink), print_exception);
            REQUIRE_NOTHROW(container.run_printer(1, sink));
            REQUIRE(out == u8"partialpartial");
        }
    }
}