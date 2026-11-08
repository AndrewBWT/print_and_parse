#pragma once

#include "pnp/core/parse/parser_input.hpp"
#include "pnp/sources/string_view_source.hpp"
#include "pnp/sources/testing/piece_source.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace piece_source_tests
{
    using PNP_NAMESPACE::buffered_text_source;
    using PNP_NAMESPACE::parser_input;
    using PNP_NAMESPACE::testing::piece_source;
    using PNP_NAMESPACE::string_view_source;
    using PNP_NAMESPACE::text_source;

    // Large enough that free space never limits a piece, for texts under 1,000
    // code units.
    inline constexpr std::size_t big_block = 4'096;

    // Reads until the input ends, and returns how much each read added. As
    // read_more() returns after the first read that hands anything over, each is
    // one piece.
    inline std::vector<std::size_t>
        pieces_read(
            text_source& source_arg
        )
    {
        std::vector<std::size_t> pieces;
        std::size_t              before = source_arg.buffered().size();
        while (source_arg.read_more())
        {
            pieces.push_back(source_arg.buffered().size() - before);
            before = source_arg.buffered().size();
        }
        return pieces;
    }

    // Splits the input into words at spaces and newlines, releasing as it goes.
    // Each word's input_text is still held when text is released, so a source
    // that frees pinned text would be caught here.
    inline std::vector<std::u8string>
        words(
            parser_input& input_arg
        )
    {
        const auto not_space = [](const char32_t char_arg)
            {
                return char_arg != U' ' && char_arg != U'\n';
            };
        std::vector<std::u8string> result;
        input_arg.skip_whitespace();
        while (not input_arg.at_end())
        {
            const auto word = input_arg.peek_chars_while(not_space);
            input_arg.advance(word.get().size());
            input_arg.release_consumed();
            result.emplace_back(word.get());
            input_arg.skip_whitespace();
        }
        return result;
    }

    // Mixed widths, so pieces split one-, two-, three- and four-byte characters.
    inline const std::u8string sample_text = []()
        {
            std::u8string text;
            for (int i = 0; i < 20; ++i)
            {
                text += u8"the quick brown 🦊 jumps over the lazy 犬\n";
                text += u8"café über niño ";
            }
            return text;
        }();

    // The words of sample_text, parsed directly.
    inline std::vector<std::u8string>
        expected_words()
    {
        string_view_source source{ sample_text };
        parser_input       input(source);
        return words(input);
    }

    // ---- Static properties ----

    static_assert(std::is_base_of_v<buffered_text_source, piece_source>);
    static_assert(not std::is_copy_constructible_v<piece_source>);
    static_assert(not std::is_copy_assignable_v<piece_source>);
    static_assert(std::is_constructible_v<piece_source, std::u8string>);
    static_assert(std::is_constructible_v<piece_source, std::u8string, std::size_t>);
    static_assert(std::is_constructible_v<
        piece_source,
        std::u8string,
        std::mt19937_64&,
        std::size_t,
        std::size_t>);
    // A temporary generator would be gone before it was used.
    static_assert(not std::is_constructible_v<
        piece_source,
        std::u8string,
        std::mt19937_64,
        std::size_t,
        std::size_t>);
    // Drawing from a generator changes it.
    static_assert(not std::is_constructible_v<
        piece_source,
        std::u8string,
        const std::mt19937_64&,
        std::size_t,
        std::size_t>);
    // Built only on purpose.
    static_assert(not std::is_convertible_v<std::u8string, piece_source>);

    // ---- Construction ----

    TEST_CASE(
        "piece_source: settings",
        "[piece_source]"
    )
    {
        SECTION("the defaults behave like interactive input, a code unit at a time")
        {
            piece_source source{ u8"abc" };
            REQUIRE(not source.reads_without_waiting());
            REQUIRE(source.minimum_piece_size() == 1);
            REQUIRE(source.maximum_piece_size() == 1);
            REQUIRE(source.buffered().empty());
        }
        SECTION("a fixed size")
        {
            piece_source source{ u8"abc", 5, true };
            REQUIRE(source.reads_without_waiting());
            REQUIRE(source.minimum_piece_size() == 5);
            REQUIRE(source.maximum_piece_size() == 5);
        }
        SECTION("a random range")
        {
            std::mt19937_64 generator(0);
            piece_source    source{ u8"abc", generator, 2, 9 };
            REQUIRE(source.minimum_piece_size() == 2);
            REQUIRE(source.maximum_piece_size() == 9);
        }
        SECTION("the buffered_text_source settings are passed on")
        {
            piece_source source{ u8"abc", 1, false, 7, 64, 256, 0.5, 2.0, 0.25 };
            REQUIRE(source.minimum_release() == 7);
            REQUIRE(source.minimum_block_size() == 64);
            REQUIRE(source.maximum_reused_block_size() == 256);
            REQUIRE(source.minimum_read_size() == 32);
            REQUIRE(source.growth_factor() == 2.0);
            REQUIRE(source.compaction_fraction() == 0.25);
        }
    }

    TEST_CASE(
        "piece_source: invalid sizes are rejected",
        "[piece_source][errors]"
    )
    {
        std::mt19937_64 generator(0);

        REQUIRE_THROWS_AS(piece_source(u8"abc", 0), std::invalid_argument);
        REQUIRE_THROWS_AS(
            piece_source(u8"abc", generator, 0, 0), std::invalid_argument
        );
        REQUIRE_THROWS_AS(
            piece_source(u8"abc", generator, 5, 4), std::invalid_argument
        );
        REQUIRE_NOTHROW(piece_source(u8"abc", generator, 0, 1));
        REQUIRE_NOTHROW(piece_source(u8"abc", generator, 4, 4));
    }

    // ---- Fixed sizes ----

    TEST_CASE(
        "piece_source: fixed sizes hand the text over in pieces of that size",
        "[piece_source]"
    )
    {
        const std::u8string text = u8"abcdefghij"; // 10 code units

        SECTION("1")
        {
            piece_source source{ text, 1, false, 1, big_block };
            REQUIRE(pieces_read(source) == std::vector<std::size_t>(10, 1));
            REQUIRE(source.buffered() == text);
        }
        SECTION("3: the last piece is what's left")
        {
            piece_source source{ text, 3, false, 1, big_block };
            REQUIRE(pieces_read(source) == std::vector<std::size_t>{3, 3, 3, 1});
            REQUIRE(source.buffered() == text);
        }
        SECTION("larger than the text: all of it at once")
        {
            piece_source source{ text, 100, false, 1, big_block };
            REQUIRE(pieces_read(source) == std::vector<std::size_t>{10});
            REQUIRE(source.buffered() == text);
        }
        SECTION("an empty text")
        {
            piece_source source{ u8"", 3 };
            REQUIRE(not source.read_more());
            REQUIRE(source.exhausted());
            REQUIRE(source.buffered().empty());
        }
    }

    TEST_CASE(
        "piece_source: the last piece reports the end",
        "[piece_source]"
    )
    {
        piece_source source{ u8"abcd", 2, false, 1, big_block };

        REQUIRE(source.read_more());
        REQUIRE(source.read_more());
        REQUIRE(source.buffered() == u8"abcd");
        REQUIRE(not source.exhausted());
        REQUIRE(not source.read_more());
        REQUIRE(source.exhausted());
    }

    TEST_CASE(
        "piece_source: a piece is cut short when the block has less room",
        "[piece_source]"
    )
    {
        // A minimum block size of 8: no piece can be larger than the room left.
        const std::u8string text(100, u8'x');
        piece_source        source{ text, 100, false, 1, 8 };

        const auto pieces = pieces_read(source);
        REQUIRE(pieces.size() > 1);
        REQUIRE(source.buffered() == text);
    }

    TEST_CASE(
        "piece_source: characters are split across pieces",
        "[piece_source][utf8]"
    )
    {
        piece_source source{ u8"é𝄞", 1, false, 1, big_block };

        REQUIRE(source.read_more());
        REQUIRE(source.valid_size() == 0); // half of é
        REQUIRE(source.read_more());
        REQUIRE(source.valid_size() == 2); // all of é
        for (int i = 0; i < 3; ++i)
        {
            REQUIRE(source.read_more());
            REQUIRE(source.valid_size() == 2); // part of 𝄞
        }
        REQUIRE(source.read_more());
        REQUIRE(source.valid_size() == 6);
        REQUIRE(source.buffered() == u8"é𝄞");
    }

    // ---- Random sizes ----

    TEST_CASE(
        "piece_source: random sizes stay within the range",
        "[piece_source][random]"
    )
    {
        for (std::uint64_t seed = 0; seed < 20; ++seed)
        {
            CAPTURE(seed);
            std::mt19937_64 generator(seed);
            piece_source source{ sample_text, generator, 3, 7, false, 1, big_block };

            const auto pieces = pieces_read(source);
            REQUIRE(source.buffered() == sample_text);
            // Every piece but the last is a full one; the last may be what's left.
            for (std::size_t i = 0; i + 1 < pieces.size(); ++i)
            {
                REQUIRE(pieces[i] >= 3);
                REQUIRE(pieces[i] <= 7);
            }
            REQUIRE(pieces.back() >= 1);
            REQUIRE(pieces.back() <= 7);
        }
    }

    TEST_CASE(
        "piece_source: the same seed splits the text the same way",
        "[piece_source][random]"
    )
    {
        std::mt19937_64 first_generator(42);
        std::mt19937_64 second_generator(42);
        piece_source    first{
            sample_text, first_generator, 1, 8, false, 1, big_block
        };
        piece_source second{
            sample_text, second_generator, 1, 8, false, 1, big_block
        };

        REQUIRE(pieces_read(first) == pieces_read(second));
    }

    TEST_CASE(
        "piece_source: different seeds split the text differently",
        "[piece_source][random]"
    )
    {
        std::mt19937_64 first_generator(1);
        std::mt19937_64 second_generator(2);
        piece_source    first{
            sample_text, first_generator, 1, 8, false, 1, big_block
        };
        piece_source second{
            sample_text, second_generator, 1, 8, false, 1, big_block
        };

        REQUIRE(pieces_read(first) != pieces_read(second));
    }

    TEST_CASE(
        "piece_source: draws from the caller's generator",
        "[piece_source][random]"
    )
    {
        std::mt19937_64       generator(7);
        const std::mt19937_64 untouched = generator;
        piece_source source{ u8"abcdefgh", generator, 1, 3, false, 1, big_block };

        (void)pieces_read(source);
        // It used the generator itself, not a copy.
        REQUIRE(generator != untouched);
    }

    TEST_CASE(
        "piece_source: zero-sized pieces are skipped",
        "[piece_source][random]"
    )
    {
        std::mt19937_64 generator(3);
        piece_source source{ sample_text, generator, 0, 1, false, 1, big_block };

        const auto pieces = pieces_read(source);
        // Each read that returns hands over exactly one code unit.
        REQUIRE(pieces == std::vector<std::size_t>(sample_text.size(), 1));
        REQUIRE(source.buffered() == sample_text);
    }

    TEST_CASE(
        "piece_source: every size in the range comes up, whatever the generator's "
        "minimum",
        "[piece_source][random]"
    )
    {
        // std::minstd_rand starts at 1, not 0.
        std::minstd_rand generator(5);
        piece_source source{ sample_text, generator, 1, 3, false, 1, big_block };

        auto pieces = pieces_read(source);
        pieces.pop_back(); // the last may be cut short
        for (const std::size_t size : {1, 2, 3})
        {
            CAPTURE(size);
            REQUIRE(std::find(pieces.begin(), pieces.end(), size) != pieces.end());
        }
    }

    TEST_CASE(
        "piece_source: a random range of one size is that fixed size",
        "[piece_source][random]"
    )
    {
        std::mt19937_64 generator(0);
        piece_source    random{
            u8"abcdefghij", generator, 3, 3, false, 1, big_block
        };
        piece_source fixed{ u8"abcdefghij", 3, false, 1, big_block };

        REQUIRE(pieces_read(random) == pieces_read(fixed));
    }

    // ---- Parsing through it ----

    TEST_CASE(
        "piece_source: parsing gives the same result however the text is split",
        "[piece_source][parse]"
    )
    {
        const auto expected = expected_words();

        SECTION("fixed sizes")
        {
            for (const std::size_t size : {1, 2, 3, 7, 64})
            {
                CAPTURE(size);
                piece_source source{ sample_text, size };
                parser_input input(source);
                REQUIRE(words(input) == expected);
            }
        }
        SECTION("random sizes")
        {
            for (std::uint64_t seed = 0; seed < 20; ++seed)
            {
                CAPTURE(seed);
                std::mt19937_64 generator(seed);
                piece_source    source{ sample_text, generator, 0, 8 };
                parser_input    input(source);
                REQUIRE(words(input) == expected);
            }
        }
        SECTION("releasing and moving text as often as possible")
        {
            // A minimum release of 0, a minimum block size of 1, and a parser_input
            // that releases everything it can: the most thorough check.
            for (std::uint64_t seed = 0; seed < 20; ++seed)
            {
                CAPTURE(seed);
                std::mt19937_64 generator(seed);
                piece_source source{ sample_text, generator, 0, 8, false, 0, 1 };
                parser_input input(source, 0, 0.0);
                REQUIRE(words(input) == expected);
            }
        }
    }
} // namespace piece_source_tests

namespace check_split_example
{
    /*
     * A parser a user might write: "true" or "false", and nothing else.
     */
    struct bool_parser
    {
        pnp::basic_parser_return_value
            run_parser(
                pnp::parser_input& input_arg,
                bool& value_arg,
                pnp::parser_run_key
            ) const
        {
            if (input_arg.advance_if_starts_with(u8"true"))
            {
                value_arg = true;
                return {};
            }
            if (input_arg.advance_if_starts_with(u8"false"))
            {
                value_arg = false;
                return {};
            }
            return input_arg.fail(u8"Expected \"true\" or \"false\".");
        }
    };

    TEST_CASE(
        "bool_parser: gives the same result however its input is split",
        "[example][check_split]"
    )
    {
        const pnp::testing::check_parser_using_split_text_config config{
            .exact_splits_to_run = {1, 2, 3, 7}
        };
        // Inputs it should parse, and inputs it should fail on: both must behave
        // the same whichever way the text arrives.
        for (const std::u8string_view text :
        {u8"true",
            u8"false",
            u8"  true  ",
            u8"false\n",
            u8"tru",
            u8"maybe",
            u8"",
            u8"truest"})
        {
            CAPTURE(std::string(text.begin(), text.end()));
            const std::optional<std::string> report
                = pnp::testing::check_parser_using_split_text<bool>(
                    config, text, bool_parser{}
                );
            // Empty if every split matched; otherwise, which one failed, and how.
            INFO(report.value_or(""));
            REQUIRE_FALSE(report.has_value());
        }
    }
} // namespace check_split_example