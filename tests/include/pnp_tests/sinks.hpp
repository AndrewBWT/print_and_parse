#pragma once

#pragma once
#include "pnp/sinks/ostream.hpp"
#include "pnp/sinks/string.hpp"
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "pnp_tests/sinks/ostream.hpp"
#include "pnp_tests/sinks/string.hpp"

namespace pnp_tests
{
    // A user's own sink: records each call, to check how the base class routes its
    // overloads onto the two virtual functions.
    struct recording_sink : public pnp::text_sink
    {
        using pnp::text_sink::put;
        using pnp::text_sink::write;

        std::u8string            text;
        std::size_t              put_calls = 0;
        std::size_t              write_calls = 0;
        std::vector<std::size_t> write_sizes;
    private:
        void
            put_char(
                const char8_t char_arg
            ) noexcept override
        {
            ++put_calls;
            text.push_back(char_arg);
        }

        void
            write_text(
                const std::u8string_view str_arg
            ) noexcept override
        {
            ++write_calls;
            write_sizes.push_back(str_arg.size());
            text.append(str_arg);
        }
    };

    // Writes through the base class only, as a printer does.
    inline void
        write_greeting(
            pnp::text_sink& sink_arg
        )
    {
        sink_arg.write(std::u8string_view(u8"hello"));
        sink_arg.put(u8',');
        sink_arg.write(u8" world");
    }
} // namespace pnp_tests

using namespace pnp_tests;

// ============================================================ string_sink

TEST_CASE(
    "string_sink: appends to its string",
    "[io][sinks][string_sink]"
)
{
    std::u8string    text;
    pnp::string_sink sink(text);

    sink.put(u8'a');
    sink.write(u8"bc");
    sink.write(std::u8string_view(u8"de"));
    sink.write(u8"fg"); // null-terminated
    CHECK(text == u8"abcdefg");

    // It appends rather than replacing.
    std::u8string             existing = u8"start:";
    pnp::string_sink appending(existing);
    appending.write(std::u8string_view(u8"more"));
    CHECK(existing == u8"start:more");

    // Empty writes change nothing.
    std::u8string             unchanged = u8"x";
    pnp::string_sink quiet(unchanged);
    quiet.write(std::u8string_view{});
    quiet.write(u8"");
    CHECK(unchanged == u8"x");
}

TEST_CASE(
    "string_sink: accepts other convertible character types",
    "[io][sinks][string_sink]"
)
{
    // char into a char8_t sink, and the reverse: the bytes are unchanged.
    std::u8string             utf8;
    pnp::string_sink utf8_sink(utf8);
    utf8_sink.put('a');
    utf8_sink.write(std::string_view("bc"));
    utf8_sink.write("de");
    CHECK(utf8 == u8"abcde");

    std::string            narrow;
    pnp::string_sink narrow_sink(narrow);
    narrow_sink.put(u8'a');
    narrow_sink.write(std::u8string_view(u8"\u00E9t\u00E9")
    ); // "été", as UTF-8 bytes
    CHECK(narrow == "a\xC3\xA9t\xC3\xA9");
}

TEST_CASE(
    "string_sink: through the base class",
    "[io][sinks][string_sink]"
)
{
    std::u8string             text;
    pnp::string_sink sink(text);
    write_greeting(sink);
    CHECK(text == u8"hello, world");
}

// ============================================================ ostream_sink

TEST_CASE(
    "ostream_sink: writes to its stream",
    "[io][sinks][ostream_sink]"
)
{
    std::ostringstream      stream;
    pnp::ostream_sink sink(stream);

    sink.put('a');
    sink.write("bc");
    sink.write(std::string_view("de"));
    sink.write("fg");
    CHECK(stream.str() == "abcdefg");

    // UTF-8 written to a char stream arrives as its bytes.
    sink.write(std::u8string_view(u8"\u00E9"));
    CHECK(stream.str() == "abcdefg\xC3\xA9");

    // It writes after whatever the stream already holds.
    std::ostringstream existing;
    existing << "start:";
    pnp::ostream_sink appending(existing);
    appending.write(std::string_view("more"));
    CHECK(existing.str() == "start:more");
}

// ============================================================ the base class

TEST_CASE(
    "abstract_sink_t: each overload reaches one of the two virtual functions",
    "[io][sinks]"
)
{
    recording_sink sink;

    // A single character goes to put.
    sink.put(u8'a');
    CHECK(sink.put_calls == 1);
    CHECK(sink.write_calls == 0);

    // A view, a pointer and length, and a null-terminated string each become
    // one write, of the right length.
    sink.write(std::u8string_view(u8"bc"));
    sink.write(u8"def");
    sink.write(u8"gh");
    CHECK(sink.write_calls == 3);
    CHECK(sink.write_sizes == std::vector<std::size_t>{2, 3, 2});

    // Converted characters reach the same functions.
    sink.put('i');
    sink.write(std::string_view("jk"));
    CHECK(sink.put_calls == 2);
    CHECK(sink.write_calls == 4);

    CHECK(sink.text == u8"abcdefghijk");
}

// ============================================================ pnps printing
// through sinks

TEST_CASE(
    "sinks: pnps print through them",
    "[io][sinks]"
)
{
    // Into a string, appending.
    std::u8string             text = u8"value=";
    pnp::string_sink string_sink(text);
    pnp::default_printer<std::vector<int>>{}.run_printer(
        std::vector<int>{1, 2, 3}, string_sink
    );
    CHECK(text == u8"value=[1,2,3]");

    // Into a stream.
    std::ostringstream      stream;
    pnp::ostream_sink stream_sink(stream);
    pnp::default_printer<std::u8string>{}.run_printer(
        std::u8string(u8"a,b"), stream_sink
    );
    CHECK(stream.str() == "\"a,b\"");

    // Into a user's own sink: printing only ever goes through put and write.
    recording_sink recording;
    pnp::default_printer<bool>{}.run_printer(true, recording);
    CHECK(recording.text == u8"true");
    CHECK(recording.put_calls + recording.write_calls > 0);

    // The same output whichever sink it goes to.
    std::u8string             via_sink;
    pnp::string_sink compare_sink(via_sink);
    pnp::default_printer<int>{}.run_printer(-42, compare_sink);
    CHECK(via_sink == pnp::to_u8string(-42));
}

// --- Compile-time checks
// ------------------------------------------------------------------
static_assert(std::is_base_of_v<
    pnp::text_sink,
    pnp::string_sink<std::u8string>>);
static_assert(std::is_base_of_v<
    pnp::text_sink,
    pnp::ostream_sink>);
static_assert(std::is_abstract_v<pnp::text_sink>);
static_assert(not std::is_abstract_v<recording_sink>);