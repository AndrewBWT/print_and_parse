#pragma once

#include "pnp/core/options.hpp"
#include "pnp/core/parse/parser_input.hpp"
#include "pnp/sources/buffered_text_source.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace buffered_text_source_tests
{
    using PNP_NAMESPACE::buffered_text_source;
    using PNP_NAMESPACE::parser_input;
    using PNP_NAMESPACE::pnp_options;
    using PNP_NAMESPACE::text_source;

    inline constexpr double nan = std::numeric_limits<double>::quiet_NaN();
    inline constexpr double infinity = std::numeric_limits<double>::infinity();

    // Raw bytes as UTF-8 code units, for invalid or partial characters.
    inline std::u8string
        bytes(
            const std::string_view text_arg
        )
    {
        return std::u8string(text_arg.begin(), text_arg.end());
    }

    // Fixed values, so the tests don't depend on the global options.
    struct source_settings
    {
        std::size_t minimum_block_size = 8;
        std::size_t maximum_reused_block_size = 1'024;
        double      minimum_read_fraction = 0.25;
        double      growth_factor = 1.5;
        double      compaction_fraction = 0.5;
        std::size_t minimum_release = 1;
        bool        reads_without_waiting = true;
        // Report the end with a separate empty read, not with the last piece.
        bool        end_reported_separately = false;
        // Throw from this read_piece call (counting from 1); 0 for never.
        int         throw_on_call = 0;
    };

    // Hands out the given pieces, splitting them if the space given is smaller.
    struct scripted_source final : buffered_text_source
    {
        std::vector<std::u8string> pieces;
        std::size_t                next_piece = 0;
        std::size_t                piece_offset = 0;
        int                        read_calls = 0;
        std::size_t                last_span = 0;
        std::size_t smallest_span = std::numeric_limits<std::size_t>::max();
        bool        end_reported_separately;
        int         throw_on_call;

        explicit scripted_source(
            std::vector<std::u8string> pieces_arg,
            const source_settings& settings_arg = {}
        )
            : buffered_text_source(
                settings_arg.reads_without_waiting,
                settings_arg.minimum_release,
                settings_arg.minimum_block_size,
                settings_arg.maximum_reused_block_size,
                settings_arg.minimum_read_fraction,
                settings_arg.growth_factor,
                settings_arg.compaction_fraction
            )
            , pieces(std::move(pieces_arg))
            , end_reported_separately(settings_arg.end_reported_separately)
            , throw_on_call(settings_arg.throw_on_call)
        {
        }

        void
            release_all()
        {
            release_up_to(offset() + buffered().size());
        }
    private:
        read_result
            read_piece(const std::span<char8_t> into_arg) override
        {
            ++read_calls;
            last_span = into_arg.size();
            smallest_span = std::min(smallest_span, into_arg.size());
            if (read_calls == throw_on_call)
            {
                throw std::runtime_error("scripted_source: planned failure");
            }
            if (next_piece >= pieces.size())
            {
                return read_result{ .code_units_written = 0, .more_content = false };
            }
            const std::u8string& current = pieces[next_piece];
            const std::size_t    count
                = std::min(current.size() - piece_offset, into_arg.size());
            std::char_traits<char8_t>::copy(
                into_arg.data(), current.data() + piece_offset, count
            );
            piece_offset += count;
            if (piece_offset == current.size())
            {
                piece_offset = 0;
                ++next_piece;
            }
            return read_result{
                .code_units_written = count,
                .more_content
                = end_reported_separately || next_piece < pieces.size(),
            };
        }
    };

    // Constructed with every default, to check they come from the options.
    struct default_options_source final : buffered_text_source
    {
        explicit default_options_source(
            const bool reads_without_waiting_arg
        )
            : buffered_text_source(reads_without_waiting_arg)
        {
        }
    private:
        read_result
            read_piece(const std::span<char8_t>) override
        {
            return read_result{ .code_units_written = 0, .more_content = false };
        }
    };

    struct global_options_restorer
    {
        pnp_options saved = PNP_NAMESPACE::global_pnp_options();

        global_options_restorer() = default;
        global_options_restorer(const global_options_restorer&) = delete;
        global_options_restorer&
            operator=(const global_options_restorer&)
            = delete;

        ~global_options_restorer()
        {
            PNP_NAMESPACE::set_global_pnp_options(saved);
        }
    };

    // ---- Static properties ----

    static_assert(std::is_abstract_v<buffered_text_source>);
    static_assert(std::is_base_of_v<text_source, buffered_text_source>);
    static_assert(std::has_virtual_destructor_v<buffered_text_source>);
    static_assert(not std::is_copy_constructible_v<scripted_source>);
    static_assert(not std::is_copy_assignable_v<scripted_source>);

    // ---- Construction and settings ----

    TEST_CASE(
        "buffered_text_source starts empty",
        "[buffered_text_source]"
    )
    {
        scripted_source source{ {u8"abc"} };

        REQUIRE(source.buffered().empty());
        REQUIRE(source.offset() == 0);
        REQUIRE(source.valid_size() == 0);
        REQUIRE(not source.exhausted());
        REQUIRE(not source.stopped_at_invalid());
        REQUIRE(not source.in_use());
        REQUIRE(source.read_calls == 0);
    }

    TEST_CASE(
        "buffered_text_source passes constructor values to text_source",
        "[buffered_text_source]"
    )
    {
        scripted_source waits{
            {},
            {.minimum_release = 123, .reads_without_waiting = false}
        };
        scripted_source never_waits{
            {},
            {.minimum_release = 456, .reads_without_waiting = true}
        };

        REQUIRE(waits.minimum_release() == 123);
        REQUIRE(not waits.reads_without_waiting());
        REQUIRE(never_waits.minimum_release() == 456);
        REQUIRE(never_waits.reads_without_waiting());
    }

    TEST_CASE(
        "buffered_text_source keeps the settings it was given",
        "[buffered_text_source]"
    )
    {
        scripted_source source{
            {},
            {.minimum_block_size = 100,
             .maximum_reused_block_size = 200,
             .minimum_read_fraction = 0.5,
             .growth_factor = 2.5,
             .compaction_fraction = 0.75}
        };

        REQUIRE(source.minimum_block_size() == 100);
        REQUIRE(source.maximum_reused_block_size() == 200);
        REQUIRE(source.minimum_read_size() == 50);
        REQUIRE(source.growth_factor() == 2.5);
        REQUIRE(source.compaction_fraction() == 0.75);
    }

    TEST_CASE(
        "buffered_text_source defaults come from the global options",
        "[buffered_text_source][options]"
    )
    {
        const global_options_restorer restorer;

        pnp_options options = PNP_NAMESPACE::global_pnp_options();
        options.text_source_minimum_release = 321;
        options.buffered_source_minimum_block_size = 80;
        options.buffered_source_maximum_reused_block_size = 500;
        options.buffered_source_minimum_read_fraction = 0.5;
        options.buffered_source_growth_factor = 2.0;
        options.buffered_source_compaction_fraction = 0.25;
        PNP_NAMESPACE::set_global_pnp_options(options);

        const default_options_source source{ true };

        REQUIRE(source.minimum_release() == 321);
        REQUIRE(source.minimum_block_size() == 80);
        REQUIRE(source.maximum_reused_block_size() == 500);
        REQUIRE(source.minimum_read_size() == 40);
        REQUIRE(source.growth_factor() == 2.0);
        REQUIRE(source.compaction_fraction() == 0.25);
    }

    TEST_CASE(
        "buffered_text_source works out the minimum read size",
        "[buffered_text_source][options]"
    )
    {
        struct read_size_case
        {
            std::size_t block;
            double      fraction;
            std::size_t expected;
        };
        // A fraction of the block size, capped at it, and never below 1.
        const read_size_case cases[] = {
            {  8,     0.25, 2},
            {100,      0.5, 50},
            {  8,      1.0, 8},
            {  8,     10.0, 8},
            {  8,      0.0, 1},
            {  0,      0.5, 1},
            {  8,     -1.0, 1},
            {  8,      nan, 1},
            {  8, infinity, 1},
        };
        for (const read_size_case& test_case : cases)
        {
            CAPTURE(test_case.block, test_case.fraction);
            scripted_source source{
                {},
                {.minimum_block_size = test_case.block,
                 .minimum_read_fraction = test_case.fraction}
            };
            REQUIRE(source.minimum_read_size() == test_case.expected);
        }
    }

    TEST_CASE(
        "buffered_text_source normalises the growth factor and compaction fraction",
        "[buffered_text_source][options]"
    )
    {
        SECTION("growth factor: finite and at least 0, or 0")
        {
            for (const double value : {nan, infinity, -infinity, -1.0})
            {
                CAPTURE(value);
                scripted_source source{ {}, {.growth_factor = value} };
                REQUIRE(source.growth_factor() == 0.0);
            }
            for (const double value : {0.0, 0.5, 2.0})
            {
                CAPTURE(value);
                scripted_source source{ {}, {.growth_factor = value} };
                REQUIRE(source.growth_factor() == value);
            }
        }
        SECTION("compaction fraction: in [0, 1), or 0")
        {
            for (const double value : {nan, infinity, -0.5, 1.0, 1.5})
            {
                CAPTURE(value);
                scripted_source source{ {}, {.compaction_fraction = value} };
                REQUIRE(source.compaction_fraction() == 0.0);
            }
            for (const double value : {0.0, 0.5, 0.95})
            {
                CAPTURE(value);
                scripted_source source{ {}, {.compaction_fraction = value} };
                REQUIRE(source.compaction_fraction() == value);
            }
        }
    }

    // ---- Reading ----

    TEST_CASE(
        "buffered_text_source reads pieces in order",
        "[buffered_text_source]"
    )
    {
        scripted_source source{
            {u8"ab", u8"cd", u8"ef"}
        };

        REQUIRE(source.read_more());
        REQUIRE(source.buffered() == u8"ab");

        REQUIRE(source.read_more());
        REQUIRE(source.buffered() == u8"abcd");

        REQUIRE(source.read_more());
        REQUIRE(source.buffered() == u8"abcdef");
        REQUIRE(source.valid_size() == 6);
    }

    TEST_CASE(
        "buffered_text_source handles the end of the input",
        "[buffered_text_source]"
    )
    {
        SECTION("the last piece also reports the end")
        {
            scripted_source source{
                {u8"ab", u8"cd"}
            };

            REQUIRE(source.read_more());
            REQUIRE(source.read_more());
            REQUIRE(source.buffered() == u8"abcd");
            REQUIRE(not source.exhausted());

            REQUIRE(not source.read_more());
            REQUIRE(source.exhausted());
            REQUIRE(source.buffered() == u8"abcd");
            // The end came with the last piece, so no further read was needed.
            REQUIRE(source.read_calls == 2);
        }

        SECTION("the end is reported by a separate empty read")
        {
            scripted_source source{
                {u8"ab", u8"cd"},
                {.end_reported_separately = true}
            };

            REQUIRE(source.read_more());
            REQUIRE(source.read_more());
            REQUIRE(not source.read_more());
            REQUIRE(source.exhausted());
            REQUIRE(source.buffered() == u8"abcd");
            REQUIRE(source.read_calls == 3);
        }

        SECTION("no pieces at all")
        {
            scripted_source source{ {} };

            REQUIRE(not source.read_more());
            REQUIRE(source.exhausted());
            REQUIRE(source.buffered().empty());
        }

        SECTION("read_piece isn't called again once it reports the end")
        {
            scripted_source source{ {u8"ab"} };

            REQUIRE(source.read_more());
            const int calls = source.read_calls;

            REQUIRE(not source.read_more());
            REQUIRE(not source.read_more());
            REQUIRE(source.read_calls == calls);
        }
    }

    TEST_CASE(
        "buffered_text_source skips empty pieces",
        "[buffered_text_source]"
    )
    {
        SECTION("empty pieces before and between text")
        {
            scripted_source source{
                {u8"", u8"", u8"ab", u8"", u8"cd"}
            };

            REQUIRE(source.read_more());
            REQUIRE(source.buffered() == u8"ab");
            REQUIRE(source.read_calls == 3);

            REQUIRE(source.read_more());
            REQUIRE(source.buffered() == u8"abcd");
            REQUIRE(source.read_calls == 5);
        }

        SECTION("only empty pieces")
        {
            scripted_source source{
                {u8"", u8"", u8""}
            };

            REQUIRE(not source.read_more());
            REQUIRE(source.exhausted());
            REQUIRE(source.buffered().empty());
        }
    }

    TEST_CASE(
        "buffered_text_source: a piece bigger than the free space arrives over "
        "several reads",
        "[buffered_text_source]"
    )
    {
        const std::u8string big(100, u8'x');
        scripted_source     source{
            {u8"ab", big}
        };

        while (source.read_more())
        {
        }

        REQUIRE(source.buffered().size() == 102);
        REQUIRE(source.buffered().substr(0, 2) == u8"ab");
        REQUIRE(source.buffered().substr(2) == big);
        REQUIRE(source.read_calls > 2);
    }

    TEST_CASE(
        "buffered_text_source: an exception from read_piece leaves the text alone",
        "[buffered_text_source][errors]"
    )
    {
        scripted_source source{
            {u8"ab", u8"cd"},
            {.throw_on_call = 2}
        };

        REQUIRE(source.read_more());
        REQUIRE_THROWS_AS(source.read_more(), std::runtime_error);
        REQUIRE(source.buffered() == u8"ab");
        REQUIRE(not source.exhausted());

        // Reading carries on afterwards.
        REQUIRE(source.read_more());
        REQUIRE(source.buffered() == u8"abcd");
    }

    TEST_CASE(
        "buffered_text_source: a block too large to represent throws length_error",
        "[buffered_text_source][errors]"
    )
    {
        scripted_source source{
            {u8"ab"},
            {.growth_factor = 1e300}
        };

        REQUIRE_THROWS_AS(source.read_more(), std::length_error);
        REQUIRE(source.buffered().empty());
        REQUIRE(not source.exhausted());
        REQUIRE(source.read_calls == 0);
    }

    // ---- UTF-8 across pieces ----

    TEST_CASE(
        "buffered_text_source joins characters split across pieces",
        "[buffered_text_source][utf8]"
    )
    {
        SECTION("a two-byte character")
        {
            scripted_source source{
                {bytes("caf\xC3"), bytes("\xA9!")}
            };

            REQUIRE(source.read_more());
            REQUIRE(source.valid_size() == 3);
            REQUIRE(not source.stopped_at_invalid());

            REQUIRE(source.read_more());
            REQUIRE(source.buffered() == u8"café!");
            REQUIRE(source.valid_size() == 6);
            REQUIRE(not source.stopped_at_invalid());
        }

        SECTION("a four-byte character split one byte per piece")
        {
            scripted_source source{
                {bytes("\xF0"), bytes("\x9D"), bytes("\x84"), bytes("\x9E")}
            };

            for (int i = 0; i < 3; ++i)
            {
                REQUIRE(source.read_more());
                REQUIRE(source.valid_size() == 0);
                REQUIRE(not source.stopped_at_invalid());
            }

            REQUIRE(source.read_more());
            REQUIRE(source.buffered() == u8"𝄞");
            REQUIRE(source.valid_size() == 4);
        }

        SECTION("a character cut off by the end of the input")
        {
            scripted_source source{ {bytes("ab\xC3")} };

            REQUIRE(source.read_more());
            REQUIRE(source.valid_size() == 2);
            REQUIRE(not source.stopped_at_invalid());

            REQUIRE(not source.read_more());
            REQUIRE(source.stopped_at_invalid());
            REQUIRE(source.valid_size() == 2);
        }
    }

    // ---- Releasing ----

    TEST_CASE(
        "buffered_text_source releases text",
        "[buffered_text_source][release]"
    )
    {
        scripted_source source{
            {u8"abcdef", u8"gh"}
        };
        REQUIRE(source.read_more());

        SECTION("part of the text")
        {
            source.release_up_to(4);

            REQUIRE(source.offset() == 4);
            REQUIRE(source.buffered() == u8"ef");

            REQUIRE(source.read_more());
            REQUIRE(source.buffered() == u8"efgh");
            REQUIRE(source.offset() == 4);
        }

        SECTION("all of the text")
        {
            source.release_all();

            REQUIRE(source.offset() == 6);
            REQUIRE(source.buffered().empty());

            REQUIRE(source.read_more());
            REQUIRE(source.buffered() == u8"gh");
            REQUIRE(source.offset() == 6);
        }

        SECTION("a position before the offset releases nothing")
        {
            source.release_up_to(3);
            source.release_up_to(1);

            REQUIRE(source.offset() == 3);
            REQUIRE(source.buffered() == u8"def");
        }

        SECTION("a position past the text releases only the text")
        {
            source.release_up_to(1'000);

            REQUIRE(source.offset() == 6);
            REQUIRE(source.buffered().empty());
        }
    }

    TEST_CASE(
        "buffered_text_source never releases an incomplete character",
        "[buffered_text_source][release]"
    )
    {
        scripted_source source{
            {bytes("ab\xC3"), bytes("\xA9")}
        };
        REQUIRE(source.read_more());

        source.release_up_to(1'000);

        REQUIRE(source.offset() == 2);
        REQUIRE(source.buffered() == bytes("\xC3"));

        REQUIRE(source.read_more());
        REQUIRE(source.buffered() == u8"é");
        REQUIRE(source.valid_size() == 2);
    }

    // ---- Block sizes ----

    TEST_CASE(
        "buffered_text_source: the first block is at least minimum_block_size",
        "[buffered_text_source][storage]"
    )
    {
        SECTION("8")
        {
            scripted_source source{ {u8"a"}, {.minimum_block_size = 8} };
            REQUIRE(source.read_more());
            REQUIRE(source.last_span == 8);
        }
        SECTION("64")
        {
            scripted_source source{ {u8"a"}, {.minimum_block_size = 64} };
            REQUIRE(source.read_more());
            REQUIRE(source.last_span == 64);
        }
        SECTION("0 still gives a usable block")
        {
            scripted_source source{
                {u8"abc"},
                {.minimum_block_size = 0}
            };
            while (source.read_more())
            {
            }
            REQUIRE(source.buffered() == u8"abc");
            REQUIRE(source.smallest_span >= 1);
        }
    }

    TEST_CASE(
        "buffered_text_source: read_piece is always given at least "
        "minimum_read_size",
        "[buffered_text_source][storage]"
    )
    {
        SECTION("a fraction of 0.5 moves the text sooner")
        {
            // 6 of 8 used: 2 free is less than the minimum read size of 4.
            scripted_source source{
                {u8"aaaaaa", u8"b"},
                {.minimum_read_fraction = 0.5}
            };
            REQUIRE(source.read_more());
            const char8_t* const start = source.buffered().data();
            REQUIRE(source.read_more());
            REQUIRE(source.buffered().data() != start);
            REQUIRE(source.buffered() == u8"aaaaaab");
            REQUIRE(source.smallest_span >= 4);
        }
        SECTION("a fraction of 0.25 uses the space left")
        {
            // 6 of 8 used: 2 free is enough for the minimum read size of 2.
            scripted_source source{
                {u8"aaaaaa", u8"b"},
                {.minimum_read_fraction = 0.25}
            };
            REQUIRE(source.read_more());
            const char8_t* const start = source.buffered().data();
            REQUIRE(source.read_more());
            REQUIRE(source.buffered().data() == start);
            REQUIRE(source.last_span == 2);
        }
    }

    TEST_CASE(
        "buffered_text_source: the growth factor sizes new blocks",
        "[buffered_text_source][storage]"
    )
    {
        // The first piece fills an 8 code unit block. The next read needs the 8
        // buffered, plus a minimum read size of 2: 10. The new block's free space
        // shows its size.
        struct growth_case
        {
            double      factor;
            std::size_t expected_span;
        };
        const growth_case cases[] = {
            {1.5,  7}, // 15 - 8
            {3.0, 22}, // 30 - 8
            {1.0,  3}, // just over needed: 11 - 8
            {0.0,  3}, // the same
        };
        for (const growth_case& test_case : cases)
        {
            CAPTURE(test_case.factor);
            scripted_source source{
                {u8"aaaaaaaa", u8"b"},
                {.growth_factor = test_case.factor}
            };
            REQUIRE(source.read_more());
            REQUIRE(source.read_more());
            REQUIRE(source.last_span == test_case.expected_span);
            REQUIRE(source.buffered() == u8"aaaaaaaab");
        }
    }

    // ---- Storage ----

    TEST_CASE(
        "buffered_text_source keeps text in place while it fits",
        "[buffered_text_source][storage]"
    )
    {
        SECTION("appending within the block")
        {
            scripted_source source{
                {u8"ab", u8"cd", u8"ef"},
                {.minimum_block_size = 64}
            };

            REQUIRE(source.read_more());
            const char8_t* const start = source.buffered().data();

            REQUIRE(source.read_more());
            REQUIRE(source.read_more());

            REQUIRE(source.buffered().data() == start);
            REQUIRE(source.buffered() == u8"abcdef");
        }

        SECTION("appending after a partial release")
        {
            scripted_source source{
                {u8"abcd", u8"ef"}
            };

            REQUIRE(source.read_more());
            const char8_t* const start = source.buffered().data();

            source.release_up_to(2);
            REQUIRE(source.buffered().data() == start + 2);

            REQUIRE(source.read_more());
            REQUIRE(source.buffered().data() == start + 2);
            REQUIRE(source.buffered() == u8"cdef");
        }
    }

    TEST_CASE(
        "buffered_text_source reuses its block once everything is released",
        "[buffered_text_source][storage]"
    )
    {
        scripted_source source{
            {u8"abc", u8"def", u8"ghi"}
        };

        REQUIRE(source.read_more());
        const char8_t* const start = source.buffered().data();

        source.release_all();
        REQUIRE(source.read_more());

        REQUIRE(source.buffered().data() == start);
        REQUIRE(source.buffered() == u8"def");

        source.release_all();
        REQUIRE(source.read_more());

        REQUIRE(source.buffered().data() == start);
        REQUIRE(source.buffered() == u8"ghi");
    }

    TEST_CASE(
        "buffered_text_source: maximum_reused_block_size decides whether an "
        "emptied block is kept",
        "[buffered_text_source][storage]"
    )
    {
        // The second read grows the block to 15. Once everything is released, the
        // free space of the next read shows whether that block was kept.
        const std::vector<std::u8string> pieces{ u8"aaaaaaaa", u8"bb", u8"cc" };

        SECTION("a block no larger than the maximum is reused")
        {
            scripted_source source{ pieces, {.maximum_reused_block_size = 15} };
            REQUIRE(source.read_more());
            REQUIRE(source.read_more());
            source.release_all();
            REQUIRE(source.read_more());
            REQUIRE(source.last_span == 15);
            REQUIRE(source.buffered() == u8"cc");
        }
        SECTION("a larger block is freed, and a minimum-sized one allocated")
        {
            scripted_source source{ pieces, {.maximum_reused_block_size = 8} };
            REQUIRE(source.read_more());
            REQUIRE(source.read_more());
            source.release_all();
            REQUIRE(source.read_more());
            REQUIRE(source.last_span == 8);
            REQUIRE(source.buffered() == u8"cc");
        }
    }

    TEST_CASE(
        "buffered_text_source moves text to a new block when full",
        "[buffered_text_source][storage]"
    )
    {
        SECTION("growing with nothing released")
        {
            scripted_source source{
                {u8"aaaa", u8"bbbb", u8"cccc"}
            };

            REQUIRE(source.read_more());
            const char8_t* const start = source.buffered().data();

            REQUIRE(source.read_more());
            REQUIRE(source.buffered().data() == start);

            REQUIRE(source.read_more());
            REQUIRE(source.buffered().data() != start);
            REQUIRE(source.buffered() == u8"aaaabbbbcccc");
            REQUIRE(source.offset() == 0);
        }

        SECTION("only the unreleased text is moved")
        {
            // Compaction is off, so the full block must be replaced.
            scripted_source source{
                {u8"abcdefgh", u8"ij"},
                {.compaction_fraction = 0.0}
            };

            REQUIRE(source.read_more());
            const char8_t* const start = source.buffered().data();

            source.release_up_to(4);
            REQUIRE(source.read_more());

            REQUIRE(source.buffered().data() != start);
            REQUIRE(source.buffered() == u8"efghij");
            REQUIRE(source.offset() == 4);
        }
    }

    TEST_CASE(
        "buffered_text_source compacts the buffered text in place",
        "[buffered_text_source][storage][compaction]"
    )
    {
        // The first piece fills the 8 code unit block exactly, so the next read
        // has to make room.
        const std::vector<std::u8string> pieces{ u8"abcdefgh", u8"ij" };

        SECTION("a little text left, with the default fraction: compacted")
        {
            scripted_source source{ pieces, {.compaction_fraction = 0.5} };
            REQUIRE(source.read_more());
            const char8_t* const start = source.buffered().data();

            source.release_up_to(7);
            REQUIRE(source.read_more());

            REQUIRE(source.buffered().data() == start);
            REQUIRE(source.buffered() == u8"hij");
            REQUIRE(source.offset() == 7);
        }
        SECTION("a little text left, with compaction off: moved")
        {
            scripted_source source{ pieces, {.compaction_fraction = 0.0} };
            REQUIRE(source.read_more());
            const char8_t* const start = source.buffered().data();

            source.release_up_to(7);
            REQUIRE(source.read_more());

            REQUIRE(source.buffered().data() != start);
            REQUIRE(source.buffered() == u8"hij");
        }
        SECTION("more text left than the fraction allows: moved")
        {
            // 6 of 8 left is more than half the block.
            scripted_source source{ pieces, {.compaction_fraction = 0.5} };
            REQUIRE(source.read_more());
            const char8_t* const start = source.buffered().data();

            source.release_up_to(2);
            REQUIRE(source.read_more());

            REQUIRE(source.buffered().data() != start);
            REQUIRE(source.buffered() == u8"cdefghij");
        }
        SECTION("a higher fraction compacts the same text")
        {
            // 6 of 8 is within 0.9 of the block, and leaves the 2 needed free.
            scripted_source source{ pieces, {.compaction_fraction = 0.9} };
            REQUIRE(source.read_more());
            const char8_t* const start = source.buffered().data();

            source.release_up_to(2);
            REQUIRE(source.read_more());

            REQUIRE(source.buffered().data() == start);
            REQUIRE(source.buffered() == u8"cdefghij");
        }
    }

    // ---- Pinning, through parser_input ----
    //
    // Only parser_input pins storage, so these read through one. A context of 0
    // and a release fraction of 0 make release_consumed() release exactly up to
    // the position. If a retired block were never freed, the source's destructor
    // would assert at the end of the test in a debug build.

    TEST_CASE(
        "buffered_text_source moves pinned text to a new block, rather than "
        "compacting it",
        "[buffered_text_source][pins]"
    )
    {
        SECTION("pinned: the old block is retired, and its text kept")
        {
            scripted_source source{
                {u8"abcdefgh", u8"ij"}
            };
            parser_input input(source, 0, 0.0);
            const auto   first = input.peek_chars(2).value();
            const char8_t* const start = source.buffered().data();

            input.advance(7);
            input.release_consumed();
            REQUIRE(source.buffered() == u8"h");

            REQUIRE(input.peek_chars(3).value().get() == u8"hij");
            REQUIRE(source.buffered().data() != start);
            REQUIRE(first.get() == u8"ab");
        }
        SECTION("unpinned: the same text is compacted in place")
        {
            scripted_source source{
                {u8"abcdefgh", u8"ij"}
            };
            parser_input input(source, 0, 0.0);
            REQUIRE(input.peek_chars(2).value().get() == u8"ab");
            const char8_t* const start = source.buffered().data();

            input.advance(7);
            input.release_consumed();

            REQUIRE(input.peek_chars(3).value().get() == u8"hij");
            REQUIRE(source.buffered().data() == start);
        }
    }

    TEST_CASE(
        "buffered_text_source keeps a pinned block's buffer once its text is "
        "released",
        "[buffered_text_source][pins]"
    )
    {
        // A maximum reused size of 0 would free the emptied buffer, if nothing
        // pinned it.
        scripted_source source{
            {u8"abcdefgh", u8"ij"},
            {.maximum_reused_block_size = 0}
        };
        parser_input input(source, 0, 0.0);
        const auto   all = input.peek_chars(8).value();

        input.advance(8);
        input.release_consumed();
        REQUIRE(source.buffered().empty());
        REQUIRE(all.get() == u8"abcdefgh");

        // The next read can't reuse the pinned block, so retires it.
        REQUIRE(input.peek_chars(2).value().get() == u8"ij");
        REQUIRE(all.get() == u8"abcdefgh");
    }

    TEST_CASE(
        "buffered_text_source frees retired blocks in any order",
        "[buffered_text_source][pins]"
    )
    {
        // Sizes chosen so the block is pinned each time it fills: the first by
        // first, the second by second.
        scripted_source source{
            {u8"aaaaaaaa", u8"bbbbbbbb", std::u8string(20, u8'c')}
        };
        parser_input input(source, 0, 0.0);

        auto first = input.peek_chars(8);
        auto second = input.peek_chars(16);
        auto third = input.peek_chars(36);
        REQUIRE(first.has_value());
        REQUIRE(second.has_value());
        REQUIRE(third.has_value());

        const std::u8string expected_second = u8"aaaaaaaabbbbbbbb";
        const std::u8string expected_third
            = expected_second + std::u8string(20, u8'c');

        SECTION("oldest first")
        {
            first.reset();
            REQUIRE(second->get() == expected_second);
            REQUIRE(third->get() == expected_third);
            second.reset();
            REQUIRE(third->get() == expected_third);
        }
        SECTION("newest first")
        {
            second.reset();
            REQUIRE(first->get() == u8"aaaaaaaa");
            REQUIRE(third->get() == expected_third);
            first.reset();
            REQUIRE(third->get() == expected_third);
        }
    }

    // ---- Streaming ----

    TEST_CASE(
        "buffered_text_source streams a long input",
        "[buffered_text_source][streaming]"
    )
    {
        std::vector<std::u8string> pieces;
        for (int i = 0; i < 10'000; ++i)
        {
            pieces.emplace_back(u8"line\n");
        }

        scripted_source source{
            std::move(pieces),
            {.minimum_block_size = 64}
        };
        std::size_t total = 0;

        while (source.read_more())
        {
            REQUIRE(source.buffered() == u8"line\n");
            total += source.buffered().size();
            source.release_all();
        }

        REQUIRE(total == 50'000);
        REQUIRE(source.offset() == 50'000);
        REQUIRE(source.exhausted());
        REQUIRE(source.buffered().empty());
        REQUIRE(source.smallest_span >= source.minimum_read_size());
    }

    TEST_CASE(
        "buffered_text_source streams with text carried over",
        "[buffered_text_source][streaming]"
    )
    {
        std::vector<std::u8string> pieces;
        for (int i = 0; i < 1'000; ++i)
        {
            pieces.emplace_back(u8"0123456789");
        }

        scripted_source source{
            std::move(pieces),
            {.minimum_block_size = 16}
        };
        std::u8string seen;

        // Keep the last 3 code units each time, so text is always carried over,
        // and pieces arrive split by the space left.
        while (source.read_more())
        {
            const std::u8string_view text = source.buffered();
            if (text.size() > 3)
            {
                seen += text.substr(0, text.size() - 3);
                source.release_up_to(source.offset() + text.size() - 3);
            }
            REQUIRE(source.buffered().size() <= 3);
        }
        seen += source.buffered();

        REQUIRE(seen.size() == 10'000);
        for (std::size_t i = 0; i < seen.size(); ++i)
        {
            REQUIRE(seen[i] == static_cast<char8_t>(u8'0' + i % 10));
        }
        REQUIRE(source.smallest_span >= source.minimum_read_size());
    }
} // namespace buffered_text_source_tests