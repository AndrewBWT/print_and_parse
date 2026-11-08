#pragma once

#include "pnp/core/parse/parser_input.hpp"
#include "pnp/sources/complete_text_source.hpp"
#include "pnp/sources/string_source.hpp"
#include "pnp/sources/string_view_source.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <limits>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace string_source_tests
{
    using PNP_NAMESPACE::complete_text_source;
    using PNP_NAMESPACE::parser_input;
    using PNP_NAMESPACE::string_source;
    using PNP_NAMESPACE::string_view_source;
    using PNP_NAMESPACE::text_source;

    inline constexpr std::size_t max_size = std::numeric_limits<std::size_t>::max();

    // Raw bytes as UTF-8 code units, for invalid or partial characters.
    inline std::u8string
        bytes(
            const std::string_view text_arg
        )
    {
        return std::u8string(text_arg.begin(), text_arg.end());
    }

    // ---- Static properties: complete_text_source ----

    static_assert(std::is_base_of_v<text_source, complete_text_source>);
    static_assert(std::is_base_of_v<complete_text_source, string_view_source>);
    static_assert(std::is_base_of_v<complete_text_source, string_source>);
    static_assert(std::has_virtual_destructor_v<complete_text_source>);
    // Only derived classes can construct one.
    static_assert(not std::is_default_constructible_v<complete_text_source>);
    static_assert(
        not std::is_constructible_v<complete_text_source, std::u8string_view>
        );

    // ---- Static properties: string_view_source ----

    static_assert(not std::is_copy_constructible_v<string_view_source>);
    static_assert(not std::is_copy_assignable_v<string_view_source>);
    static_assert(not std::is_move_constructible_v<string_view_source>);
    // Anything that converts to a std::u8string_view, and outlives the source.
    static_assert(std::is_constructible_v<string_view_source, std::u8string&>);
    static_assert(
        std::is_constructible_v<string_view_source, const std::u8string&>
        );
    static_assert(std::is_constructible_v<string_view_source, std::u8string_view>);
    static_assert(std::is_constructible_v<string_view_source, const char8_t*>);
    static_assert(
        std::is_constructible_v<string_view_source, const char8_t(&)[4]>
        );
    // A temporary string would leave the view dangling.
    static_assert(not std::is_constructible_v<string_view_source, std::u8string>);
    static_assert(
        not std::is_constructible_v<string_view_source, std::u8string&&>
        );
    static_assert(
        not std::is_constructible_v<string_view_source, std::pmr::u8string&&>
        );
    // Narrow text isn't accepted.
    static_assert(not std::is_constructible_v<string_view_source, std::string_view>);
    // Built only on purpose.
    static_assert(
        not std::is_convertible_v<std::u8string_view, string_view_source>
        );
    static_assert(
        std::is_nothrow_constructible_v<string_view_source, std::u8string_view>
        );

    // ---- Static properties: string_source ----

    static_assert(not std::is_copy_constructible_v<string_source>);
    static_assert(not std::is_copy_assignable_v<string_source>);
    static_assert(not std::is_move_constructible_v<string_source>);
    // Anything a std::u8string can be made from.
    static_assert(std::is_constructible_v<string_source, std::u8string>);
    static_assert(std::is_constructible_v<string_source, std::u8string&>);
    static_assert(std::is_constructible_v<string_source, const std::u8string&>);
    static_assert(std::is_constructible_v<string_source, std::u8string_view>);
    static_assert(std::is_constructible_v<string_source, const char8_t*>);
    static_assert(std::is_constructible_v<string_source, const char8_t(&)[4]>);
    static_assert(not std::is_constructible_v<string_source, std::string>);
    // Built only on purpose.
    static_assert(not std::is_convertible_v<std::u8string, string_source>);
    static_assert(not std::is_convertible_v<const char8_t*, string_source>);

    // ---- Construction ----

    TEST_CASE(
        "string sources hold the whole input from the start",
        "[string_source]"
    )
    {
        SECTION("string_view_source")
        {
            const std::u8string text = u8"abc def";
            string_view_source  source{ text };

            REQUIRE(source.buffered() == text);
            REQUIRE(source.valid_size() == text.size());
            REQUIRE(source.offset() == 0);
            REQUIRE(not source.stopped_at_invalid());
            REQUIRE(not source.exhausted());
            REQUIRE(not source.in_use());
        }
        SECTION("string_source")
        {
            string_source source{ u8"abc def" };

            REQUIRE(source.buffered() == u8"abc def");
            REQUIRE(source.valid_size() == 7);
            REQUIRE(source.offset() == 0);
            REQUIRE(not source.stopped_at_invalid());
            REQUIRE(not source.exhausted());
            REQUIRE(not source.in_use());
        }
        SECTION("empty")
        {
            string_view_source from_view{ u8"" };
            string_source      owned{ u8"" };

            REQUIRE(from_view.buffered().empty());
            REQUIRE(owned.buffered().empty());
            REQUIRE(not from_view.read_more());
            REQUIRE(not owned.read_more());
            REQUIRE(from_view.exhausted());
            REQUIRE(owned.exhausted());
        }
    }

    TEST_CASE(
        "string sources' settings",
        "[string_source]"
    )
    {
        const std::u8string text = u8"abc";
        string_view_source  from_view{ text };
        string_source       owned{ text };

        // Never released by parser_input, as releasing frees nothing.
        REQUIRE(from_view.minimum_release() == max_size);
        REQUIRE(owned.minimum_release() == max_size);
        // The text is already in memory, so reading never waits.
        REQUIRE(from_view.reads_without_waiting());
        REQUIRE(owned.reads_without_waiting());
    }

    // ---- string_view_source: viewing ----

    TEST_CASE(
        "string_view_source views the caller's text without copying it",
        "[string_source][string_view_source]"
    )
    {
        SECTION("a std::u8string")
        {
            const std::u8string text = u8"skip:value";
            string_view_source  source{ text };

            REQUIRE(source.buffered().data() == text.data());

            source.release_up_to(5);
            REQUIRE(source.buffered() == u8"value");
            REQUIRE(source.buffered().data() == text.data() + 5);
            REQUIRE(source.offset() == 5);
        }
        SECTION("a std::u8string_view")
        {
            const std::u8string text = u8"abc";
            string_view_source  source{ std::u8string_view(text) };
            REQUIRE(source.buffered().data() == text.data());
        }
        SECTION("a literal: everything but the closing null")
        {
            string_view_source source{ u8"abc" };
            REQUIRE(source.buffered() == u8"abc");
            REQUIRE(source.buffered().size() == 3);
        }
        SECTION("a pointer: up to the first null, as a C string")
        {
            const char8_t  text[] = u8"ab\0cd";
            const char8_t* start = text;
            string_view_source source{ start };
            REQUIRE(source.buffered() == u8"ab");
            REQUIRE(source.buffered().data() == text);
        }
    }

    // ---- string_source: owning ----

    TEST_CASE(
        "string_source owns its text",
        "[string_source]"
    )
    {
        SECTION("a copy outlives the original")
        {
            std::optional<std::u8string> original(u8"skip:value");
            string_source                 source{ *original };

            REQUIRE(source.buffered().data() != original->data());
            original->assign(u8"changed!!!");
            original.reset();

            REQUIRE(source.buffered() == u8"skip:value");
            source.release_up_to(5);
            REQUIRE(source.buffered() == u8"value");
            REQUIRE(source.offset() == 5);
        }
        SECTION("a view is copied")
        {
            std::u8string      text = u8"abc";
            string_source      source{ std::u8string_view(text) };

            REQUIRE(source.buffered().data() != text.data());
            text[0] = u8'x';
            REQUIRE(source.buffered() == u8"abc");
        }
        SECTION("a long string is moved in, not copied")
        {
            // Long enough to be on the heap, so moving hands the buffer over.
            std::u8string        text(1'000, u8'x');
            const char8_t* const buffer = text.data();
            string_source        source{ std::move(text) };

            REQUIRE(source.buffered().data() == buffer);
            REQUIRE(source.buffered().size() == 1'000);
        }
    }

    // ---- Reading ----

    TEST_CASE(
        "string sources never have more to read",
        "[string_source]"
    )
    {
        string_view_source from_view{ u8"abc" };
        string_source      owned{ u8"abc" };

        for (text_source* source : { static_cast<text_source*>(&from_view),
                                    static_cast<text_source*>(&owned) })
        {
            REQUIRE(not source->exhausted());
            REQUIRE(not source->read_more());
            REQUIRE(source->exhausted());
            REQUIRE(not source->read_more());
            REQUIRE(source->buffered() == u8"abc");
        }
    }

    // ---- UTF-8 ----

    TEST_CASE(
        "string sources check their text for valid UTF-8",
        "[string_source][utf8]"
    )
    {
        SECTION("valid multi-byte text")
        {
            string_source source{ u8"café 𝄞" };
            REQUIRE(source.valid_size() == source.buffered().size());
            REQUIRE(not source.stopped_at_invalid());
        }
        SECTION("an invalid byte: found at once")
        {
            // Split literal: "\x80cd" would read as one hex escape.
            const std::u8string text = bytes("ab\x80" "cd");
            string_view_source  source{ text };
            REQUIRE(source.valid_size() == 2);
            REQUIRE(source.stopped_at_invalid());
        }
        SECTION("a character cut off at the end: invalid once the input ends")
        {
            // It could still be completed, until reading finds nothing more.
            string_source source{ bytes("ab\xC3") };
            REQUIRE(source.valid_size() == 2);
            REQUIRE(not source.stopped_at_invalid());

            REQUIRE(not source.read_more());
            REQUIRE(source.stopped_at_invalid());
            REQUIRE(source.valid_size() == 2);
        }
    }

    // ---- Releasing ----

    TEST_CASE(
        "string sources: release_up_to still releases",
        "[string_source][release]"
    )
    {
        string_source        source{ u8"ab\ncd" };
        const char8_t* const start = source.buffered().data();

        source.release_up_to(3);

        REQUIRE(source.offset() == 3);
        REQUIRE(source.buffered() == u8"cd");
        // The text doesn't move: the view just starts later.
        REQUIRE(source.buffered().data() == start + 3);
        // Lines and columns still count from the very start.
        REQUIRE(source.location_at(3).line == 2);
        REQUIRE(source.location_at(3).column == 1);
        REQUIRE(source.location_at(4).column == 2);
    }

    TEST_CASE(
        "string sources: release_up_to releases only valid text",
        "[string_source][release]"
    )
    {
        string_source source{ bytes("ab\x80" "cd") };

        source.release_up_to(1'000);

        REQUIRE(source.offset() == 2);
        REQUIRE(source.buffered() == bytes("\x80" "cd"));
    }

    // ---- Through parser_input ----

    TEST_CASE(
        "string sources: parser_input never releases their text",
        "[string_source][release]"
    )
    {
        const std::u8string text(10'000, u8'x');
        string_view_source  source{ text };
        {
            // With a fraction of 0, only minimum_release() could stop a release.
            parser_input input(source, 0, 0.0);
            input.advance(text.size());
            input.release_consumed();
            REQUIRE(input.at_end());
        }
        REQUIRE(source.offset() == 0);
        REQUIRE(source.buffered().size() == text.size());
    }

    TEST_CASE(
        "string sources: parser_input reads them like a string",
        "[string_source]"
    )
    {
        string_source        source{ u8"abc def" };
        const char8_t* const start = source.buffered().data();
        parser_input         input(source);

        REQUIRE(input.advance_if_starts_with(u8"abc"));
        input.skip_whitespace();
        const auto rest = input.peek_chars(3);
        REQUIRE(rest.has_value());
        REQUIRE(rest->get() == u8"def");
        // The view is into the source's own text: nothing was copied.
        REQUIRE(rest->get().data() == start + 4);
        REQUIRE(input.peek_chars(4) == std::nullopt);
    }

    TEST_CASE(
        "string sources: positions count from the start, after earlier text was "
        "released",
        "[string_source]"
    )
    {
        string_source source{ u8"skip:value" };
        source.release_up_to(5);

        parser_input input(source);
        REQUIRE(input.position() == 5);
        REQUIRE(input.advance_if_starts_with(u8"value"));
        REQUIRE(input.position() == 10);
    }
} // namespace string_source_tests