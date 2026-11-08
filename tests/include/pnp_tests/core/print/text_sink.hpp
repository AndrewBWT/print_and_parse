#pragma once

#include "pnp/core/print/text_sink.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

namespace text_sink_tests
{
    using PNP_NAMESPACE::text_sink;

    struct write_only_sink final : text_sink
    {
        std::u8string text;
        int           write_text_calls = 0;
    private:
        void
            write_text(const std::u8string_view text_arg) override
        {
            ++write_text_calls;
            text.append(text_arg);
        }
    };

    struct put_overriding_sink final : text_sink
    {
        std::u8string text;
        int           put_char_calls = 0;
        int           write_text_calls = 0;
    private:
        void
            put_char(const char8_t char_arg) override
        {
            ++put_char_calls;
            text.push_back(char_arg);
        }

        void
            write_text(const std::u8string_view text_arg) override
        {
            ++write_text_calls;
            text.append(text_arg);
        }
    };

    struct destructor_flag_sink final : text_sink
    {
        bool* destroyed;

        explicit destructor_flag_sink(bool& destroyed_arg) noexcept
            : destroyed(&destroyed_arg)
        {
        }

        ~destructor_flag_sink() override
        {
            *destroyed = true;
        }
    private:
        void
            write_text(const std::u8string_view) override
        {
        }
    };

    // ---- Static properties ----

    static_assert(std::is_abstract_v<text_sink>);
    static_assert(std::has_virtual_destructor_v<text_sink>);
    static_assert(not std::is_default_constructible_v<text_sink>);

    static_assert(not std::is_copy_constructible_v<text_sink>);
    static_assert(not std::is_copy_assignable_v<text_sink>);
    static_assert(not std::is_move_constructible_v<text_sink>);
    static_assert(not std::is_move_assignable_v<text_sink>);

    static_assert(std::is_default_constructible_v<write_only_sink>);
    static_assert(not std::is_copy_constructible_v<write_only_sink>);
    static_assert(not std::is_copy_assignable_v<write_only_sink>);
    static_assert(not std::is_move_constructible_v<write_only_sink>);

    static_assert(requires (text_sink& sink_arg) {
        sink_arg.put(u8'a');
        sink_arg.write(u8"a");
        sink_arg.write("a");
        sink_arg.write(std::u8string{});
        sink_arg.write(std::string{});
    });

    static_assert(requires (write_only_sink& sink_arg) {
        sink_arg.put(u8'a');
        sink_arg.write(u8"a");
        sink_arg.write("a");
        sink_arg.write(std::u8string{});
        sink_arg.write(std::string{});
    });

    static_assert(requires (put_overriding_sink& sink_arg) {
        sink_arg.put(u8'a');
        sink_arg.write(u8"a");
        sink_arg.write("a");
    });

    template <typename Sink>
    concept can_call_put_char = requires (Sink & sink_arg) {
        sink_arg.put_char(u8'a');
    };

    template <typename Sink>
    concept can_call_write_text = requires (Sink & sink_arg) {
        sink_arg.write_text(u8"a");
    };

    static_assert(not can_call_put_char<text_sink>);
    static_assert(not can_call_write_text<text_sink>);
    static_assert(not can_call_put_char<write_only_sink>);
    static_assert(not can_call_write_text<write_only_sink>);

    // ---- write(std::u8string_view) ----

    TEST_CASE("text_sink::write(u8string_view) forwards to write_text", "[text_sink]")
    {
        write_only_sink sink;

        SECTION("through the derived type")
        {
            sink.write(u8"hello");

            REQUIRE(sink.text == u8"hello");
            REQUIRE(sink.write_text_calls == 1);
        }

        SECTION("through a text_sink reference")
        {
            text_sink& base = sink;
            base.write(u8"hello");

            REQUIRE(sink.text == u8"hello");
            REQUIRE(sink.write_text_calls == 1);
        }

        SECTION("non-ASCII text")
        {
            sink.write(u8"café ☕ 𝄞");

            REQUIRE(sink.text == u8"café ☕ 𝄞");
        }

        SECTION("a view that isn't null-terminated")
        {
            const std::u8string text = u8"abcdef";
            sink.write(std::u8string_view{ text.data(), 3 });

            REQUIRE(sink.text == u8"abc");
        }

        SECTION("empty text")
        {
            sink.write(std::u8string_view{});

            REQUIRE(sink.text.empty());
        }
    }

    // ---- write(std::string_view) ----

    TEST_CASE("text_sink::write(string_view) writes the bytes unchanged", "[text_sink]")
    {
        write_only_sink sink;

        SECTION("a literal through the derived type")
        {
            sink.write("hello");

            REQUIRE(sink.text == u8"hello");
            REQUIRE(sink.write_text_calls == 1);
        }

        SECTION("a literal through a text_sink reference")
        {
            text_sink& base = sink;
            base.write("hello");

            REQUIRE(sink.text == u8"hello");
            REQUIRE(sink.write_text_calls == 1);
        }

        SECTION("a std::string")
        {
            const std::string text = "from std::string";
            sink.write(text);

            REQUIRE(sink.text == u8"from std::string");
        }

        SECTION("UTF-8 bytes in a char string")
        {
            sink.write("caf\xC3\xA9");

            REQUIRE(sink.text == u8"café");
        }

        SECTION("invalid UTF-8 is not checked")
        {
            const std::string bytes(1, static_cast<char>(0xFF));
            sink.write(bytes);

            REQUIRE(sink.text == std::u8string(1, static_cast<char8_t>(0xFF)));
        }

        SECTION("embedded null characters are kept")
        {
            sink.write(std::string_view{ "a\0b", 3 });

            REQUIRE(sink.text.size() == 3);
            REQUIRE(sink.text == std::u8string(u8"a\0b", 3));
        }

        SECTION("a view that isn't null-terminated")
        {
            const std::string text = "abcdef";
            sink.write(std::string_view{ text.data(), 3 });

            REQUIRE(sink.text == u8"abc");
        }
    }

    // ---- put ----

    TEST_CASE("text_sink::put uses write_text by default", "[text_sink]")
    {
        write_only_sink sink;

        SECTION("a single code unit")
        {
            sink.put(u8'x');

            REQUIRE(sink.text == u8"x");
            REQUIRE(sink.write_text_calls == 1);
        }

        SECTION("through a text_sink reference")
        {
            text_sink& base = sink;
            base.put(u8'y');

            REQUIRE(sink.text == u8"y");
            REQUIRE(sink.write_text_calls == 1);
        }

        SECTION("a multi-byte character one code unit at a time")
        {
            sink.put(static_cast<char8_t>(0xC3));
            sink.put(static_cast<char8_t>(0xA9));

            REQUIRE(sink.text == u8"é");
            REQUIRE(sink.write_text_calls == 2);
        }
    }

    TEST_CASE("text_sink::put uses an overridden put_char", "[text_sink]")
    {
        put_overriding_sink sink;

        SECTION("put calls put_char, not write_text")
        {
            sink.put(u8'x');

            REQUIRE(sink.text == u8"x");
            REQUIRE(sink.put_char_calls == 1);
            REQUIRE(sink.write_text_calls == 0);
        }

        SECTION("through a text_sink reference")
        {
            text_sink& base = sink;
            base.put(u8'y');

            REQUIRE(sink.put_char_calls == 1);
            REQUIRE(sink.write_text_calls == 0);
        }

        SECTION("write calls write_text, not put_char")
        {
            sink.write(u8"ab");
            sink.write("cd");

            REQUIRE(sink.text == u8"abcd");
            REQUIRE(sink.put_char_calls == 0);
            REQUIRE(sink.write_text_calls == 2);
        }
    }

    // ---- Mixed use and destruction ----

    TEST_CASE("text_sink keeps the order of mixed writes", "[text_sink]")
    {
        write_only_sink sink;
        text_sink& base = sink;

        base.write(u8"[");
        base.put(u8'1');
        base.write(", ");
        base.write(u8"é");
        base.put(u8']');

        REQUIRE(sink.text == u8"[1, é]");
        REQUIRE(sink.write_text_calls == 5);
    }

    TEST_CASE("text_sink destroys derived sinks through a base pointer", "[text_sink]")
    {
        bool destroyed = false;
        {
            std::unique_ptr<text_sink> sink
                = std::make_unique<destructor_flag_sink>(destroyed);
            sink->write(u8"anything");
        }

        REQUIRE(destroyed);
    }
}