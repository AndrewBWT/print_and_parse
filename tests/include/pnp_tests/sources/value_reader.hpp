#pragma once
#include "pnp/sources/istream_text_source.hpp"
#include "pnp/sources/testing/piece_source.hpp"
#include "pnp/sources/string_source.hpp"
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <cstddef>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace pnp_tests;

// ============================================================ separated by
// whitespace

TEST_CASE(
    "value_reader: values separated by whitespace",
    "[io][value_reader]"
)
{
    CHECK(read_all_from<int>(u8"1 2 3").values == std::vector<int>{1, 2, 3});
    CHECK(
        read_all_from<int>(u8"1\n2\n3\n").values == std::vector<int>{1, 2, 3}
    );
    CHECK(
        read_all_from<int>(u8"  1 \t 2\n\n\n3  ").values
        == std::vector<int>{1, 2, 3}
    );
    CHECK(read_all_from<int>(u8"42").values == std::vector<int>{42});

    // No values at all: nothing, or only whitespace.
    const auto empty = read_all_from<int>(u8"");
    CHECK(empty.values.empty());
    CHECK(not empty.failure.has_value());
    CHECK(read_all_from<int>(u8" \n\t ").values.empty());
}

TEST_CASE(
    "value_reader: values with structure of their own",
    "[io][value_reader]"
)
{
    // Strings containing spaces, commas and escaped newlines.
    CHECK(
        read_all_from<std::u8string>(u8"\"a b\" \"c,d\"\n\"e\\nf\"").values
        == std::vector<std::u8string>{u8"a b", u8"c,d", u8"e\nf"}
    );

    // Lists, one per line.
    CHECK(read_all_from<std::vector<int>>(u8"[1,2]\n[]\n[3]").values
        == std::vector<std::vector<int>>{{1, 2}, {}, { 3 }});

    // Tuples, as in a log of readings.
    using reading = std::tuple<std::u8string, int, double>;
    const auto readings
        = read_all_from<reading>(u8"(\"kitchen\",1,21.5)\n(\"hall\",2,18.25)\n"
        );
    REQUIRE(readings.values.size() == 2);
    CHECK(readings.values[1] == reading{u8"hall", 2, 18.25});
}

TEST_CASE(
    "value_reader: a configured parser",
    "[io][value_reader]"
)
{
    using vector_pnp  = pnp::default_parser<std::vector<int>>;
    const auto spaced = read_all_from<std::vector<int>, vector_pnp>(
        u8"[1, 2]\n[3, 4, 5]", vector_pnp({.separator_spacing = u8" "})
    );
    CHECK(spaced.values == std::vector<std::vector<int>>{{1, 2}, { 3, 4, 5 }});
}

// ============================================================ failures

TEST_CASE(
    "value_reader: a bad value stops the reader",
    "[io][value_reader]"
)
{
    const auto outcome = read_all_from<int>(u8"1 2 x 4");
    CHECK(
        outcome.values == std::vector<int>{1, 2}
    ); // the values before it are kept
    REQUIRE(outcome.failure.has_value());
    CHECK(
        outcome.failure->parse_outcome_state
        == pnp::parse_outcome::parse_outcome_state_enum::failure_parser_error
    );
    CHECK(outcome.failure->position == 4); // in the whole input
}

TEST_CASE(
    "value_reader: values can't run into what follows",
    "[io][value_reader]"
)
{
    // 12 followed straight away by x: not a value, then something else.
    const auto outcome = read_all_from<int>(u8"12x 34");
    CHECK(outcome.values.empty());
    REQUIRE(outcome.failure.has_value());
    CHECK(outcome.failure->position == 2);
}

TEST_CASE(
    "value_reader: invalid UTF-8 is a failure, not an exception",
    "[io][value_reader]"
)
{
    const std::u8string text    = std::u8string(u8"1 ") + char8_t(0xFF);
    const auto          outcome = read_all_from<int>(text);
    CHECK(outcome.values == std::vector<int>{1});
    REQUIRE(outcome.failure.has_value());
    CHECK(
        outcome.failure->parse_outcome_state
        == pnp::parse_outcome::parse_outcome_state_enum::
            failure_parser_exception
    );
    CHECK(outcome.failure->position == 2);
}

// ============================================================ streamed input

TEST_CASE(
    "value_reader: the same values however the input is split",
    "[io][value_reader]"
)
{
    const std::u8string text = u8"[1,22] \"a b\"\n[333]  \"\"\n";
    for (const std::size_t piece_size :
         {std::size_t{1}, std::size_t{2}, std::size_t{3}, std::size_t{7}})
    {
        INFO("piece size " << piece_size);
        pnp::testing::piece_source vectors_and_strings(text, piece_size);
        pnp::value_reader<std::vector<int>> first_reader(vectors_and_strings);

        // Each value read in turn, alternating types, from the same source.
        const auto first = first_reader.next();
        REQUIRE(first.has_value());
        CHECK(first->value == std::vector<int>{1, 22});
    }

    for (const std::size_t piece_size :
         {std::size_t{1}, std::size_t{2}, std::size_t{5}})
    {
        INFO("piece size " << piece_size);
        pnp::testing::piece_source source(std::u8string(u8"10 200\n3000\n"), piece_size);
        CHECK(read_all<int>(source).values == std::vector<int>{10, 200, 3'000});
    }
}

/*TEST_CASE(
    "value_reader: a line at a time from a stream",
    "[io][value_reader]"
)
{
    std::istringstream stream(
        "(\"kitchen\",1,21.5)\n(\"hall\",2,18.25)\n(\"bedroom\",1,19)\n"
    );
    pnp::line_source source(stream, true);
    using reading      = std::tuple<std::u8string, int, double>;
    const auto outcome = read_all<reading>(source);
    REQUIRE(outcome.values.size() == 3);
    CHECK(std::get<0>(outcome.values[2]) == u8"bedroom");
}*/

TEST_CASE(
    "value_reader: each value's text is released once it's read",
    "[io][value_reader]"
)
{
    pnp::string_source     source(std::u8string(u8"11 22 33"));
    pnp::value_reader<int> reader(source);

    REQUIRE(reader.next().has_value());
    CHECK(source.offset() == 2); // "11" released
    REQUIRE(reader.next().has_value());
    CHECK(source.offset() == 5); // " 22" released
    CHECK(source.buffered() == u8" 33");
}

TEST_CASE(
    "value_reader: a value is returned without waiting for the next",
    "[io][value_reader]"
)
{
    // As if typed: each piece is what one keystroke or line delivers. Once the
    // newline after 5 has arrived, 5 is complete, so the reader must return it
    // without asking for more.
    counting_source        source({u8"5", u8"\n", u8"6", u8"\n"});
    pnp::value_reader<int> reader(source);

    const auto             first = reader.next();
    REQUIRE(first.has_value());
    CHECK(first->value == 5);
    CHECK(source.given == 2); // "5" and "\n", and nothing more

    const auto second = reader.next();
    REQUIRE(second.has_value());
    CHECK(second->value == 6);
    CHECK(source.given == 4);

    CHECK(not reader.next().has_value());
}

namespace
{
// "[1,1,...,1]" with count_arg elements, a piece at a time, recording the most
// text it ever held.
struct long_vector_source : public pnp::buffered_text_source
{
    explicit long_vector_source(
        const std::size_t count_arg
    )
        : pnp::buffered_text_source(true), remaining(count_arg)
    {}

    std::size_t remaining;
    std::size_t largest = 0;
    bool        opened  = false;
    bool        closed  = false;
private:
    std::u8string pending;
    std::size_t   pending_offset = 0;
private:
    static std::size_t
        copy_part(
            const std::u8string_view text_arg,
            std::size_t&             offset_arg,
            const std::span<char8_t> into_arg
        )
    {
        const std::size_t count
            = std::min(text_arg.size() - offset_arg, into_arg.size());
        std::char_traits<char8_t>::copy(
            into_arg.data(), text_arg.data() + offset_arg, count
        );
        offset_arg += count;
        return count;
    }

    // The original generator: appends the next batch of text, or returns false
    // once the vector is complete.
    bool
        generate(
            std::u8string& piece_arg
        )
    {
        if (not opened)
        {
            opened     = true;
            piece_arg += u8'[';
        }
        if (remaining > 0)
        {
            const std::size_t n = std::min<std::size_t>(remaining, 500);
            for (std::size_t i = 0; i < n; ++i)
            {
                piece_arg += (remaining == n && i == n - 1) ? u8"1" : u8"1,";
            }
            remaining -= n;
            return true;
        }
        if (not closed)
        {
            closed     = true;
            piece_arg += u8']';
            return true;
        }
        return false;
    }

    read_result
        read_piece(
            const std::span<char8_t> into_arg
        ) override
    {
        largest = std::max(largest, buffered().size());
        if (pending_offset == pending.size())
        {
            pending.clear();
            pending_offset = 0;
            if (not generate(pending))
            {
                return read_result{
                    .code_units_written = 0, .more_content = false
                };
            }
        }
        return read_result{
            .code_units_written = copy_part(pending, pending_offset, into_arg),
            .more_content       = true,
        };
    }
};
} // namespace

TEST_CASE(
    "release: a long vector needs little of its text at once",
    "[release][sequences]"
)
{
    long_vector_source source(200'000); // about 400 KB of text
    const auto parsed = pnp::try_parse<std::vector<int>>(source);
    CHECK(parsed.outcome.succeeded());
    CHECK(parsed.value.size() == 200'000);
    CHECK(source.largest < 32 * 1'024);
}

TEST_CASE(
    "release: a long vector inside a pair still releases",
    "[release][sequences]"
)
{
    // The pair doesn't mark its start, so the vector inside it can release.
    // Adapt the type and text to your pair pnp's defaults.
    // ...
}

TEST_CASE(
    "parse: successive parses from one source read successive values",
    "[release][parse]"
)
{
    pnp::string_source source(u8"1 22 333");
    for (const int expected : {1, 22, 333})
    {
        const auto parsed = pnp::try_parse<int>(source);
        CHECK(parsed.value == expected);
    }
    CHECK(source.buffered().empty());
}

namespace
{
std::u8string
    make_int_list(
        const std::size_t count_arg
    )
{
    std::u8string text = u8"[";
    for (std::size_t i = 0; i < count_arg; ++i)
    {
        const std::string number = std::to_string(i);
        text.append(number.begin(), number.end());
        if (i + 1 < count_arg)
        {
            text += u8',';
        }
    }
    text += u8']';
    return text;
}

std::u8string
    make_spaced_ints(
        const std::size_t count_arg
    )
{
    std::u8string text;
    for (std::size_t i = 0; i < count_arg; ++i)
    {
        const std::string number = std::to_string(i);
        text.append(number.begin(), number.end());
        text += u8' ';
    }
    return text;
}

std::u8string
    make_string_list(
        const std::size_t count_arg
    )
{
    std::u8string text = u8"[";
    for (std::size_t i = 0; i < count_arg; ++i)
    {
        const std::string word  = "word" + std::to_string(i);
        text                   += u8'"';
        text.append(word.begin(), word.end());
        text += u8'"';
        if (i + 1 < count_arg)
        {
            text += u8',';
        }
    }
    text += u8']';
    return text;
}

std::string
    as_narrow(
        const std::u8string& text_arg
    )
{
    return std::string(text_arg.begin(), text_arg.end());
}
} // namespace

TEST_CASE(
    "benchmarks: parsing",
    "[!benchmark]"
)
{
    const std::u8string ints    = make_int_list(1'000'000);  // about 7 MB
    const std::u8string spaced  = make_spaced_ints(1'000'000);
    const std::u8string strings = make_string_list(200'000); // about 2.5 MB

    BENCHMARK("vector<int>: fixed string")
    {
        return pnp::try_parse<std::vector<int>>(std::u8string_view(ints)
        )
            .value.size();
    };
    BENCHMARK("vector<int>: string_source")
    {
        pnp::string_source source(ints);
        return pnp::try_parse<std::vector<int>>(source).value.size();
    };
    BENCHMARK("vector<int>: istream_source")
    {
        std::istringstream  stream(as_narrow(ints));
        pnp::istream_source source(stream, true);
        return pnp::try_parse<std::vector<int>>(source).value.size();
    };
    BENCHMARK("vector<u8string>: fixed string")
    {
        return pnp::try_parse<std::vector<std::u8string>>(
            std::u8string_view(strings)
        )
            .value.size();
    };
    BENCHMARK("vector<u8string>: istream_source")
    {
        std::istringstream  stream(as_narrow(strings));
        pnp::istream_source source(stream, true);
        return pnp::try_parse<std::vector<std::u8string>>(source)
            .value.size();
    };
    BENCHMARK("value_reader<int>: istream_source")
    {
        std::istringstream     stream(as_narrow(spaced));
        pnp::istream_source    source(stream, true);
        pnp::value_reader<int> reader(source);
        std::size_t            count = 0;
        while (const auto value = reader.next())
        {
            count += value->outcome.succeeded() ? 1 : 0;
        }
        return count;
    };
    BENCHMARK("string_source: arrival only")
    {
        pnp::string_source source(ints);
        return source.valid_size();
    };
}
