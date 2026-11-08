#pragma once
#include "pnp/core/print/free_functions.hpp"
#include "pnp_tests_internal.hpp"

#include <concepts>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace print_free_function_tests
{
    // ============================================================ helpers

    // Prints an int as "<digits>".
    struct tag_printer
    {
        void
            run_printer(
                const int& value_arg,
                pnp::text_sink& sink_arg
            ) const
        {
            sink_arg.put(u8'<');
            sink_arg.write(std::string_view(std::to_string(value_arg)));
            sink_arg.put(u8'>');
        }
    };

    // Prints an int after a configurable prefix: the default and template-argument
    // forms use the default prefix, the explicit form whatever it's given.
    struct prefixed_printer
    {
        std::u8string prefix = u8"#";

        void
            run_printer(
                const int& value_arg,
                pnp::text_sink& sink_arg
            ) const
        {
            sink_arg.write(prefix);
            sink_arg.write(std::string_view(std::to_string(value_arg)));
        }
    };

    // Prints non-ASCII text, to check it passes through unchanged.
    struct cafe_printer
    {
        void
            run_printer(
                const int&,
                pnp::text_sink& sink_arg
            ) const
        {
            sink_arg.write(u8"café");
        }
    };

    struct print_failure : std::runtime_error
    {
        print_failure()
            : std::runtime_error("print_failure")
        {
        }
    };

    // Writes part of its output, then throws.
    struct throwing_printer
    {
        void
            run_printer(
                const int&,
                pnp::text_sink& sink_arg
            ) const
        {
            sink_arg.write(u8"partial");
            throw print_failure();
        }
    };

    // A printer that can't be copied.
    struct move_only_printer
    {
        std::unique_ptr<int> state = std::make_unique<int>(0);

        void
            run_printer(
                const int& value_arg,
                pnp::text_sink& sink_arg
            ) const
        {
            sink_arg.write(std::string_view(std::to_string(value_arg)));
        }
    };

    // A type with its own default_printer, below.
    struct widget
    {
        int value = 0;
    };

    template <typename Printer, typename T>
    concept can_to_stream_c = requires(const Printer & printer_arg, const T & value_arg) {
        pnp::to_stream(printer_arg, value_arg);
    };

    template <typename Printer, typename T>
    concept can_stream_c = requires(
        const Printer & printer_arg,
        const T & value_arg,
        std::ostream & stream_arg
        ) {
        pnp::stream(printer_arg, value_arg, stream_arg);
    };

} // namespace print_free_function_tests

PNP_NS_BEGIN

template <>
struct default_printer<print_free_function_tests::widget>
{
    static constexpr bool is_specialized = true;

    void
        run_printer(
            const print_free_function_tests::widget& value_arg,
            text_sink& sink_arg
        ) const
    {
        sink_arg.write(u8"widget(");
        sink_arg.write(std::string_view(std::to_string(value_arg.value)));
        sink_arg.put(u8')');
    }
};

PNP_NS_END

using namespace print_free_function_tests;

// ============================================================ the three forms

TEST_CASE(
    "print free functions: the three forms agree",
    "[print][free_functions]"
)
{
    const widget value{ .value = 7 };
    // Explicit printer, default printer, printer as a template argument.
    {
        CHECK(pnp::to_u8string(tag_printer{}, 7) == u8"<7>");
        CHECK(pnp::to_u8string<tag_printer>(7) == u8"<7>");
        CHECK(pnp::to_u8string(value) == u8"widget(7)");
        CHECK(
            pnp::to_u8string(pnp::default_printer<widget>{}, value)
            == u8"widget(7)"
        );
    }
    {
        CHECK(pnp::to_native_string(tag_printer{}, 7) == "<7>");
        CHECK(pnp::to_native_string<tag_printer>(7) == "<7>");
        CHECK(pnp::to_native_string(value) == "widget(7)");
    }
    {
        std::u8string text;
        std::u8string sink_text;
        pnp::string_sink sink(sink_text);
        pnp::print(tag_printer{}, 7, sink);
        pnp::print<tag_printer>(7, sink);
        pnp::print(value, sink);
        CHECK(sink_text == u8"<7><7>widget(7)");
    }
    {
        std::ostringstream out;
        pnp::stream(tag_printer{}, 7, out);
        pnp::stream<tag_printer>(7, out);
        pnp::stream(value, out);
        CHECK(out.str() == "<7><7>widget(7)");
    }
    {
        std::ostringstream out;
        out << pnp::to_stream(tag_printer{}, 7) << pnp::to_stream<tag_printer>(7)
            << pnp::to_stream(value);
        CHECK(out.str() == "<7><7>widget(7)");
    }
}

TEST_CASE(
    "print free functions: an explicit printer's settings are used",
    "[print][free_functions]"
)
{
    const prefixed_printer dollar{ .prefix = u8"$" };
    CHECK(pnp::to_u8string(dollar, 5) == u8"$5");
    // The template-argument form default-constructs, so gets the default.
    CHECK(pnp::to_u8string<prefixed_printer>(5) == u8"#5");

    std::ostringstream out;
    out << pnp::to_stream(dollar, 5);
    CHECK(out.str() == "$5");
}

// ============================================================ append_str

TEST_CASE(
    "append_str: appends to both string types",
    "[print][free_functions]"
)
{
    {
        std::u8string text = u8"x=";
        pnp::append_str(tag_printer{}, 1, text);
        pnp::append_str<tag_printer>(2, text);
        CHECK(text == u8"x=<1><2>");
    }
    {
        std::string text = "x=";
        pnp::append_str(tag_printer{}, 1, text);
        pnp::append_str<tag_printer>(2, text);
        CHECK(text == "x=<1><2>");
    }
}

TEST_CASE(
    "print free functions: UTF-8 passes through unchanged",
    "[print][free_functions]"
)
{
    CHECK(pnp::to_u8string(cafe_printer{}, 0) == u8"café");
    // The same bytes in a std::string.
    CHECK(pnp::to_native_string(cafe_printer{}, 0) == "caf\xC3\xA9");

    std::ostringstream out;
    pnp::stream(cafe_printer{}, 0, out);
    CHECK(out.str() == "caf\xC3\xA9");
}

// ============================================================ partial output

TEST_CASE(
    "print free functions: output written before an exception stays",
    "[print][free_functions][errors]"
)
{
    // append_str: the string keeps what was there, and what was written.
    {
        std::u8string text = u8"x=";
        CHECK_THROWS_AS(pnp::append_str(throwing_printer{}, 0, text), print_failure);
        CHECK(text == u8"x=partial");
    }
    // stream: the same, in the stream.
    {
        std::ostringstream out;
        CHECK_THROWS_AS(pnp::stream(throwing_printer{}, 0, out), print_failure);
        CHECK(out.str() == "partial");
    }
    // <<: earlier parts of the chain are written; later parts aren't.
    {
        std::ostringstream out;
        CHECK_THROWS_AS(
            out << "a: " << pnp::to_stream(throwing_printer{}, 0) << "end",
            print_failure
        );
        CHECK(out.str() == "a: partial");
    }
    // to_u8string: the exception escapes; there's no result to inspect.
    CHECK_THROWS_AS(pnp::to_u8string(throwing_printer{}, 0), print_failure);
    // to_stream itself prints nothing, so can't throw a print error.
    CHECK_NOTHROW((void)pnp::to_stream(throwing_printer{}, 0));
}

// ============================================================ stream_proxy

TEST_CASE(
    "stream_proxy: types",
    "[print][free_functions][stream_proxy]"
)
{
    using proxy_t = pnp::stream_proxy<tag_printer, int>;
    STATIC_CHECK(not std::is_default_constructible_v<proxy_t>);
    STATIC_CHECK(std::is_copy_constructible_v<proxy_t>);

    // A printer that can't be copied can't make a proxy, but everything else
    // still works.
    STATIC_CHECK(pnp::printer_c<move_only_printer, int>);
    STATIC_CHECK(not pnp::stream_proxy_printer_c<move_only_printer, int>);
    STATIC_CHECK(not can_to_stream_c<move_only_printer, int>);
    STATIC_CHECK(can_stream_c<move_only_printer, int>);
    STATIC_CHECK(can_to_stream_c<tag_printer, int>);
}

TEST_CASE(
    "stream_proxy: a move-only printer still prints",
    "[print][free_functions][stream_proxy]"
)
{
    const move_only_printer printer;
    CHECK(pnp::to_u8string(printer, 3) == u8"3");
    std::ostringstream out;
    pnp::stream(printer, 3, out);
    CHECK(out.str() == "3");
}

TEST_CASE(
    "stream_proxy: refers to the value, rather than copying it",
    "[print][free_functions][stream_proxy]"
)
{
    int        value = 1;
    const auto proxy = pnp::to_stream(tag_printer{}, value);
    value = 2;
    std::ostringstream out;
    out << proxy;
    CHECK(out.str() == "<2>");
}