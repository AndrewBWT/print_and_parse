#pragma once

#include "pnp/core/parse/parser_input.hpp"
#include "pnp/sources/istream_text_source.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace istream_source_tests
{
    using PNP_NAMESPACE::istream_source;
    using PNP_NAMESPACE::parser_input;
    using PNP_NAMESPACE::text_source;

    // Raw bytes as UTF-8 code units, for invalid or partial characters.
    inline std::u8string
        bytes(
            const std::string_view text_arg
        )
    {
        return std::u8string(text_arg.begin(), text_arg.end());
    }

    /*
     * A stream buffer that hands its text over a chunk at a time, like a terminal
     * handing over each line as it's typed. Within a chunk, everything has
     * "arrived"; the next chunk only arrives when asked for. Chunks must not be
     * empty.
     *
     * With fail_after_last set, asking for more after the last chunk throws, which
     * the stream turns into badbit: a stand-in for "this would have waited".
     */
    struct chunk_buf : std::streambuf
    {
        std::vector<std::string> chunks;
        bool                     fail_after_last = false;
        std::size_t              next_chunk = 0;
        int                      underflows = 0;
        std::string              current;

        explicit chunk_buf(
            std::vector<std::string> chunks_arg,
            const bool               fail_after_last_arg = false
        )
            : chunks(std::move(chunks_arg))
            , fail_after_last(fail_after_last_arg)
        {
        }
    protected:
        int_type
            underflow() override
        {
            ++underflows;
            if (next_chunk >= chunks.size())
            {
                if (fail_after_last)
                {
                    throw std::runtime_error("chunk_buf: asked past the last chunk");
                }
                return traits_type::eof();
            }
            current = chunks[next_chunk++];
            setg(current.data(), current.data(), current.data() + current.size());
            return traits_type::to_int_type(current.front());
        }
    };

    // Reads until the input ends.
    inline void
        read_all(
            text_source& source_arg
        )
    {
        while (source_arg.read_more())
        {
        }
    }

    // ---- Static properties ----

    static_assert(std::is_base_of_v<
        PNP_NAMESPACE::buffered_text_source,
        istream_source>);
    static_assert(not std::is_copy_constructible_v<istream_source>);
    static_assert(not std::is_copy_assignable_v<istream_source>);
    static_assert(std::is_constructible_v<istream_source, std::istream&, bool>);
    // A temporary stream would be gone before it was read.
    static_assert(
        not std::is_constructible_v<istream_source, std::istringstream, bool>
        );
    static_assert(
        not std::is_convertible_v<std::istream&, istream_source>
        );

    // ---- Construction ----

    TEST_CASE(
        "istream_source reads nothing until asked",
        "[istream_source]"
    )
    {
        chunk_buf    buffer{ {"abc"} };
        std::istream stream(&buffer);
        istream_source source{ stream, false };

        REQUIRE(source.buffered().empty());
        REQUIRE(buffer.underflows == 0);
        REQUIRE(not source.reads_without_waiting());
    }

    TEST_CASE(
        "istream_source passes its settings on",
        "[istream_source]"
    )
    {
        std::istringstream stream("abc");
        istream_source     source{ stream, true, 123, 64, 256, 0.5, 2.0, 0.25 };

        REQUIRE(source.reads_without_waiting());
        REQUIRE(source.minimum_release() == 123);
        REQUIRE(source.minimum_block_size() == 64);
        REQUIRE(source.maximum_reused_block_size() == 256);
        REQUIRE(source.minimum_read_size() == 32);
        REQUIRE(source.growth_factor() == 2.0);
        REQUIRE(source.compaction_fraction() == 0.25);
    }

    TEST_CASE(
        "istream_source rejects a stream that has already failed",
        "[istream_source][errors]"
    )
    {
        SECTION("a file that couldn't be opened")
        {
            std::ifstream stream(
                "pnp_istream_source_test_this_file_does_not_exist.txt",
                std::ios::binary
            );
            REQUIRE_THROWS_AS(istream_source(stream, true), std::invalid_argument);
        }
        SECTION("a stream already in the fail state")
        {
            std::istringstream stream("abc");
            stream.setstate(std::ios::failbit);
            REQUIRE_THROWS_AS(istream_source(stream, true), std::invalid_argument);
        }
        SECTION("a stream already at its end is fine: it's just empty")
        {
            std::istringstream stream("");
            stream.get();
            stream.clear(std::ios::eofbit);
            istream_source source{ stream, true };
            REQUIRE(not source.read_more());
            REQUIRE(source.buffered().empty());
        }
    }

    // ---- Reading: both modes ----

    TEST_CASE(
        "istream_source reads a whole stream",
        "[istream_source]"
    )
    {
        for (const bool waits : {false, true})
        {
            CAPTURE(waits);
            std::istringstream stream("abc def\nghi");
            istream_source     source{ stream, not waits };

            read_all(source);
            REQUIRE(source.buffered() == u8"abc def\nghi");
            REQUIRE(source.exhausted());
            REQUIRE(not source.stopped_at_invalid());
        }
    }

    TEST_CASE(
        "istream_source: an empty stream",
        "[istream_source]"
    )
    {
        for (const bool waits : {false, true})
        {
            CAPTURE(waits);
            std::istringstream stream("");
            istream_source     source{ stream, not waits };

            REQUIRE(not source.read_more());
            REQUIRE(source.exhausted());
            REQUIRE(source.buffered().empty());
        }
    }

    TEST_CASE(
        "istream_source reads a stream far larger than its blocks",
        "[istream_source]"
    )
    {
        std::string text;
        for (int i = 0; i < 10'000; ++i)
        {
            text += "0123456789";
        }
        for (const bool waits : {false, true})
        {
            CAPTURE(waits);
            std::istringstream stream(text);
            istream_source     source{ stream, not waits, 1, 64 };
            std::u8string      seen;

            // Release everything after each read, so the blocks stay small.
            while (source.read_more())
            {
                seen += source.buffered();
                source.release_up_to(source.offset() + source.buffered().size());
            }
            REQUIRE(seen.size() == text.size());
            REQUIRE(seen == bytes(text));
            REQUIRE(source.offset() == text.size());
        }
    }

    // ---- Reading: not waiting for more than has arrived ----

    TEST_CASE(
        "istream_source: an interactive source takes only what has arrived",
        "[istream_source][interactive]"
    )
    {
        SECTION("each read returns one chunk, not a full block")
        {
            chunk_buf      buffer{ {"ab", "cd"} };
            std::istream   stream(&buffer);
            istream_source source{ stream, false };

            REQUIRE(source.read_more());
            REQUIRE(source.buffered() == u8"ab");
            REQUIRE(source.read_more());
            REQUIRE(source.buffered() == u8"abcd");
            REQUIRE(not source.read_more());
        }
        SECTION("it doesn't ask for more once it has some")
        {
            // Asking for a second chunk would fail: the user hasn't typed it yet.
            chunk_buf      buffer{ {"ab"}, true };
            std::istream   stream(&buffer);
            istream_source source{ stream, false };

            REQUIRE(source.read_more());
            REQUIRE(source.buffered() == u8"ab");
            REQUIRE(buffer.underflows == 1);
        }
        SECTION("a non-interactive source does ask for more")
        {
            // read() keeps asking until the block is full, so reaches the trap.
            chunk_buf      buffer{ {"ab"}, true };
            std::istream   stream(&buffer);
            istream_source source{ stream, true };

            REQUIRE_THROWS_AS(source.read_more(), std::ios_base::failure);
        }
    }

    TEST_CASE(
        "istream_source: an interactive parse decides as soon as it can",
        "[istream_source][interactive]"
    )
    {
        // "[1" has arrived; a parser checking for "[" mustn't wait for more.
        chunk_buf      buffer{ {"[1"}, true };
        std::istream   stream(&buffer);
        istream_source source{ stream, false };
        parser_input   input(source);

        REQUIRE(input.advance_if_starts_with(u8"["));
        REQUIRE(input.peek_char().value().first == U'1');
        REQUIRE(buffer.underflows == 1);
    }

    // ---- UTF-8 ----

    TEST_CASE(
        "istream_source: characters split across reads",
        "[istream_source][utf8]"
    )
    {
        SECTION("a character split across chunks")
        {
            chunk_buf      buffer{ {"caf\xC3", "\xA9!"} };
            std::istream   stream(&buffer);
            istream_source source{ stream, false };

            REQUIRE(source.read_more());
            REQUIRE(source.valid_size() == 3);
            REQUIRE(not source.stopped_at_invalid());
            REQUIRE(source.read_more());
            REQUIRE(source.buffered() == u8"café!");
            REQUIRE(source.valid_size() == 6);
        }
        SECTION("a character cut off by the end of the stream")
        {
            std::istringstream stream("ab\xC3");
            istream_source     source{ stream, true };

            read_all(source);
            REQUIRE(source.valid_size() == 2);
            REQUIRE(source.stopped_at_invalid());
        }
        SECTION("the bytes are read as they are, whatever the locale")
        {
            std::istringstream stream("caf\xC3\xA9 \xF0\x9D\x84\x9E");
            istream_source     source{ stream, true };

            read_all(source);
            REQUIRE(source.buffered() == u8"café 𝄞");
        }
    }

    // ---- Errors ----

    TEST_CASE(
        "istream_source: a stream error is an error, not the end of the input",
        "[istream_source][errors]"
    )
    {
        for (const bool waits : {false, true})
        {
            CAPTURE(waits);
            // Fails after its last chunk, rather than reporting the end.
            chunk_buf      buffer{ {"abc"}, true };
            std::istream   stream(&buffer);
            istream_source source{ stream, not waits };

            if (waits)
            {
                REQUIRE(source.read_more());
                REQUIRE(source.buffered() == u8"abc");
            }
            REQUIRE_THROWS_AS(source.read_more(), std::ios_base::failure);
            REQUIRE(not source.exhausted());
        }
    }

    // ---- Files ----

    TEST_CASE(
        "istream_source reads a file",
        "[istream_source][file]"
    )
    {
        const std::filesystem::path path
            = std::filesystem::temp_directory_path()
            / "pnp_istream_source_test.txt";
        // A long file, with Windows line endings that binary mode must keep.
        std::string text;
        for (int i = 0; i < 5'000; ++i)
        {
            text += "line ";
            text += std::to_string(i);
            text += " caf\xC3\xA9\r\n";
        }
        {
            std::ofstream out(path, std::ios::binary);
            out << text;
        }

        {
            std::ifstream  in(path, std::ios::binary);
            istream_source source{ in, true, 1 };

            read_all(source);
            REQUIRE(source.buffered() == bytes(text));
            REQUIRE(source.valid_size() == text.size());
            REQUIRE(not source.stopped_at_invalid());
        }
        {
            // Through parser_input, a line at a time, releasing as it goes.
            std::ifstream  in(path, std::ios::binary);
            istream_source source{ in, true, 1 };
            parser_input   input(source, 0, 0.0);

            int lines = 0;
            while (not input.at_end())
            {
                REQUIRE(input.advance_until(u8"\r\n").has_value());
                REQUIRE(input.advance_if_starts_with(u8"\r\n"));
                input.release_consumed();
                ++lines;
            }
            REQUIRE(lines == 5'000);
            REQUIRE(source.buffered().size() < text.size());
        }
        std::filesystem::remove(path);
    }
} // namespace istream_source_tests