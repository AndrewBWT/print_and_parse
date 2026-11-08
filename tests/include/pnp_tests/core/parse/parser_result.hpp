#pragma once
#include "pnp/core/parse/parser_input.hpp"
#include "pnp/core/parse/parser_result.hpp"
#include "pnp_tests_internal.hpp"

#include <concepts>
#include <exception>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

using namespace pnp_tests;

namespace
{
    // ============================================================ helpers

    // Whether text_arg contains part_arg.
    bool
        contains(
            const std::u8string_view text_arg,
            const std::u8string_view part_arg
        )
    {
        return text_arg.find(part_arg) != std::u8string_view::npos;
    }

    // Any narrow or char8_t text, as a u8string. Takes the value itself, so a
    // temporary std::string (e.g. from message()) lives long enough.
    template <typename T>
    std::u8string
        as_u8(const T& text_arg)
    {
        const std::basic_string_view view(text_arg);
        return std::u8string(view.begin(), view.end());
    }

    std::u8string
        number(const std::size_t value_arg)
    {
        return as_u8(std::to_string(value_arg));
    }

    // Whether add_msg accepts a T.
    template <typename T>
    concept can_add_msg_c = requires(
        pnp::basic_parser_return_value & value_arg,
        T && message_arg
        ) {
        value_arg.add_msg(std::forward<T>(message_arg));
    };

    using messages_t = pnp::basic_parser_return_value::messages_t;

} // namespace

// ============================================================ text_location
// and parse_error_context

TEST_CASE(
    "text_location: starts at line 1, column 1",
    "[parser_result]"
)
{
    STATIC_CHECK(pnp::text_location{}.line == 1);
    STATIC_CHECK(pnp::text_location{}.column == 1);
    constexpr pnp::text_location location{ .line = 3, .column = 7 };
    STATIC_CHECK(location.line == 3);
    STATIC_CHECK(location.column == 7);
}

TEST_CASE(
    "parse_error_context: defaults",
    "[parser_result]"
)
{
    const pnp::parse_error_context context;
    CHECK(context.before_pos_text.empty());
    CHECK(context.current_pos_text.empty());
    CHECK(context.after_pos_text.empty());
    CHECK(context.before_text_cut_off);
    CHECK(context.after_text_cut_off);
    CHECK(context.location.line == 1);
    CHECK(context.location.column == 1);
}

// ============================================================
// basic_parser_return_value

TEST_CASE(
    "basic_parser_return_value: types",
    "[parser_result][return_value]"
)
{
    using pnp::basic_parser_return_value;
    // Only parser_input makes failures.
    STATIC_CHECK(std::is_default_constructible_v<basic_parser_return_value>);
    STATIC_CHECK(
        not std::is_constructible_v<
        basic_parser_return_value,
        std::size_t,
        const char8_t*>
    );
    STATIC_CHECK(std::is_copy_constructible_v<basic_parser_return_value>);
    STATIC_CHECK(std::is_nothrow_move_constructible_v<basic_parser_return_value>);

    // messages() keeps the value category it was called with.
    STATIC_CHECK(
        std::same_as<
        decltype(std::declval<basic_parser_return_value&>().messages()),
        messages_t&>
    );
    STATIC_CHECK(
        std::same_as<
        decltype(std::declval<const basic_parser_return_value&>().messages()),
        const messages_t&>
    );
    STATIC_CHECK(
        std::same_as<
        decltype(std::declval<basic_parser_return_value>().messages()),
        messages_t&&>
    );

    // add_msg: a reference back for an lvalue, the object by value for an
    // rvalue.
    STATIC_CHECK(
        std::same_as<
        decltype(std::declval<basic_parser_return_value&>().add_msg(u8"x")),
        basic_parser_return_value&>
    );
    STATIC_CHECK(
        std::same_as<
        decltype(std::declval<basic_parser_return_value>().add_msg(u8"x")),
        basic_parser_return_value>
    );

    // add_msg takes anything a u8string can be made from, and nothing else.
    STATIC_CHECK(can_add_msg_c<const char8_t*>);
    STATIC_CHECK(can_add_msg_c<std::u8string_view>);
    STATIC_CHECK(can_add_msg_c<std::u8string>);
    STATIC_CHECK(can_add_msg_c<const std::u8string&>);
    STATIC_CHECK(not can_add_msg_c<int>);
    STATIC_CHECK(not can_add_msg_c<std::string>);
}

TEST_CASE(
    "basic_parser_return_value: a success",
    "[parser_result][return_value]"
)
{
    const pnp::basic_parser_return_value result;
    CHECK(result.succeeded());
    CHECK(not result.failed());
    CHECK(result.position() == 0);
    CHECK(result.messages().empty());
}

TEST_CASE(
    "basic_parser_return_value: a failure from fail()",
    "[parser_result][return_value]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abcdef"));
    input.advance(2);
    // At the current position.
    {
        const auto result = input.fail(u8"here");
        CHECK(result.failed());
        CHECK(not result.succeeded());
        CHECK(result.position() == 2);
        REQUIRE(result.messages().size() == 1);
        CHECK(result.messages()[0] == u8"here");
    }
    // At a given position.
    {
        const auto result = input.fail(u8"there", 5);
        CHECK(result.position() == 5);
        CHECK(result.messages()[0] == u8"there");
    }
    // From a view, and from an owned string.
    {
        const std::u8string_view view = u8"a view";
        CHECK(input.fail(view).messages()[0] == u8"a view");
        CHECK(input.fail(std::u8string(u8"owned")).messages()[0] == u8"owned");
    }
}

TEST_CASE(
    "basic_parser_return_value: add_msg on a named failure",
    "[parser_result][return_value]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abc"));
    input.advance(1);
    auto  result = input.fail(u8"first");
    // The same object comes back, so calls chain on it.
    auto& same = result.add_msg(u8"second");
    CHECK(&same == &result);
    result.add_msg(std::u8string_view(u8"third"))
        .add_msg(std::u8string(u8"fourth"));

    const auto& messages = result.messages();
    REQUIRE(messages.size() == 4);
    CHECK(messages[0] == u8"first");
    CHECK(messages[1] == u8"second");
    CHECK(messages[2] == u8"third");
    CHECK(messages[3] == u8"fourth");
    // Adding messages never moves the failure.
    CHECK(result.position() == 1);
    CHECK(result.failed());
}

TEST_CASE(
    "basic_parser_return_value: add_msg on a temporary failure",
    "[parser_result][return_value]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abc"));
    // Comes back by value, so keeping it is safe.
    auto&& kept = input.fail(u8"first").add_msg(u8"second").add_msg(u8"third");
    REQUIRE(kept.messages().size() == 3);
    CHECK(kept.messages()[0] == u8"first");
    CHECK(kept.messages()[2] == u8"third");
    CHECK(kept.position() == 0);
}

TEST_CASE(
    "basic_parser_return_value: the usual pattern in a pnp",
    "[parser_result][return_value]"
)
{
    // As a sequence pnp would: add context only to a failure, then return it.
    const auto with_context
        = [](pnp::basic_parser_return_value result_arg)
        {
            if (result_arg.failed())
            {
                result_arg.add_msg(u8"while parsing element 2");
            }
            return result_arg;
        };
    pnp::parser_input input(std::u8string_view(u8"abc"));

    const auto failure = with_context(input.fail(u8"expected a digit"));
    REQUIRE(failure.messages().size() == 2);
    CHECK(failure.messages()[1] == u8"while parsing element 2");

    const auto success = with_context(pnp::basic_parser_return_value{});
    CHECK(success.succeeded());
    CHECK(success.messages().empty());
}

TEST_CASE(
    "basic_parser_return_value: moving the messages out",
    "[parser_result][return_value]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abc"));
    auto              result = input.fail(u8"first");
    result.add_msg(u8"second");
    const messages_t taken = std::move(result).messages();
    REQUIRE(taken.size() == 2);
    CHECK(taken[0] == u8"first");
    CHECK(taken[1] == u8"second");
}

// ============================================================
// parse_result_error

TEST_CASE(
    "parse_result_error: types",
    "[parser_result][error]"
)
{
    // Only parser_input makes them, through the static constructors.
    STATIC_CHECK(not std::is_default_constructible_v<pnp::parse_result_error>);
    // Copied out of exceptions, so it must copy.
    STATIC_CHECK(std::is_copy_constructible_v<pnp::parse_result_error>);
    STATIC_CHECK(std::is_move_constructible_v<pnp::parse_result_error>);
}

TEST_CASE(
    "parse_result_error: invalid_utf8",
    "[parser_result][error]"
)
{
    const std::u8string text = u8"a" + byte(0x80);
    pnp::parser_input   input{ std::u8string_view(text) };
    const auto          error = thrown_error(
        [&]
        {
            (void)input.peek_chars(2);
        }
    );
    REQUIRE(error.has_value());
    CHECK(error->code() == error_code::invalid_utf8);
    CHECK(error->details().error_code == error_code::invalid_utf8);
    CHECK(error->position() == 1);
    REQUIRE(error->details().utf8_error.has_value());
    // Fields for other errors are left alone.
    CHECK(error->details().requested == 0);
    CHECK(error->details().token.empty());
    CHECK(error->details().char8 == u8'\0');

    const std::u8string message = error->to_message();
    CHECK(message.starts_with(u8"At code unit 1: "));
    CHECK(contains(message, u8"not valid UTF-8"));
    // The decoder's own explanation is passed on.
    CHECK(contains(message, as_u8(error->details().utf8_error->message())));
}

TEST_CASE(
    "parse_result_error: invalid_parser_advanced_past_end",
    "[parser_result][error]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abcd"));
    input.advance(1);
    const auto error = thrown_error(
        [&]
        {
            input.advance(5);
        }
    );
    REQUIRE(error.has_value());
    CHECK(error->code() == error_code::invalid_parser_advanced_past_end);
    CHECK(error->position() == 1);
    CHECK(error->details().requested == 5);
    CHECK(error->details().available == 3);
    CHECK(not error->details().utf8_error.has_value());

    const std::u8string message = error->to_message();
    CHECK(message.starts_with(u8"At code unit 1: "));
    CHECK(contains(message, u8"advance(5)"));
    CHECK(contains(message, u8"only 3"));
}

TEST_CASE(
    "parse_result_error: invalid_parser_escape_matches_token",
    "[parser_result][error]"
)
{
    pnp::parser_input input(std::u8string_view(u8"abc"));
    const auto        error = thrown_error(
        [&]
        {
            (void)input.peek_until(u8"'end", u8"'esc");
        }
    );
    REQUIRE(error.has_value());
    CHECK(error->code() == error_code::invalid_parser_escape_matches_token);
    CHECK(error->position() == 0);
    CHECK(error->details().token == u8"'end");
    CHECK(error->details().escape == u8"'esc");

    const std::u8string message = error->to_message();
    CHECK(message.starts_with(u8"At code unit 0: "));
    CHECK(contains(message, u8"\"'end\""));
    CHECK(contains(message, u8"\"'esc\""));
}

TEST_CASE(
    "parse_result_error: invalid_parser_foreign_mark",
    "[parser_result][error]"
)
{
    const std::u8string this_file
        = as_u8(std::source_location::current().file_name());
    // From another parser_input.
    {
        pnp::parser_input a(std::u8string_view(u8"abc"));
        pnp::parser_input b(std::u8string_view(u8"abc"));
        const auto        mark_line = std::source_location::current().line() + 1;
        const auto        mark_a = a.mark();
        const auto        error = thrown_error(
            [&]
            {
                b.reset(mark_a);
            }
        );
        REQUIRE(error.has_value());
        CHECK(error->code() == error_code::invalid_parser_foreign_mark);
        CHECK(error->details().mark_problem == pnp::mark_problem_enum::other_input);
        CHECK(error->details().mark_location.line() == mark_line);

        const std::u8string message = error->to_message();
        CHECK(contains(message, u8"different parser_input"));
        CHECK(contains(message, this_file + u8":" + number(mark_line) + u8":"));
        CHECK(contains(
            message,
            this_file + u8":"
            + number(error->details().reset_location.line()) + u8":"
        ));
    }
    // Moved from.
    {
        pnp::parser_input input(std::u8string_view(u8"abc"));
        auto              moved_from = input.mark();
        auto              moved_to = std::move(moved_from);
        const auto        error = thrown_error(
            [&]
            {
                input.reset(moved_from);
            }
        );
        REQUIRE(error.has_value());
        CHECK(error->details().mark_problem == pnp::mark_problem_enum::moved_from);
        CHECK(contains(error->to_message(), u8"moved from"));
    }
}

TEST_CASE(
    "parse_result_error: invalid_parser_advanced_into_character",
    "[parser_result][error]"
)
{
    pnp::parser_input input(std::u8string_view(u8"aé"));
    input.advance(1);
    const auto error = thrown_error(
        [&]
        {
            input.advance(1);
        }
    );
    REQUIRE(error.has_value());
    CHECK(error->code() == error_code::invalid_parser_advanced_into_character);
    CHECK(error->position() == 2);
    CHECK(error->details().requested == 1);
    CHECK(error->details().char8 == static_cast<char8_t>(0xA9));

    const std::u8string message = error->to_message();
    CHECK(message.starts_with(u8"At code unit 2: "));
    CHECK(contains(message, u8"advance(1)"));
    CHECK(contains(message, u8"0xA9"));
}

TEST_CASE(
    "parse_result_error: invalid_parser_split_character",
    "[parser_result][error]"
)
{
    pnp::parser_input input(std::u8string_view(u8"aé"));
    const auto        error = thrown_error(
        [&]
        {
            (void)input.peek_while(
                [](const char8_t c)
                {
                    return c != static_cast<char8_t>(0xA9);
                }
            );
        }
    );
    REQUIRE(error.has_value());
    CHECK(error->code() == error_code::invalid_parser_split_character);
    CHECK(error->position() == 2);
    CHECK(error->details().char8 == static_cast<char8_t>(0xA9));

    const std::u8string message = error->to_message();
    CHECK(message.starts_with(u8"At code unit 2: "));
    CHECK(contains(message, u8"0xA9"));
    CHECK(contains(message, u8"peek_chars_while"));
}

TEST_CASE(
    "parse_result_error: a single-digit byte is still two hex digits",
    "[parser_result][error]"
)
{
    // Not a real continuation byte, but checks the formatting: the predicate
    // rejects a byte below 0x10 inside... a character can't contain one, so
    // the split must come from a real continuation byte. 0x80 is the smallest.
    const std::u8string text = u8"a" + byte(0xC2) + byte(0x80);
    pnp::parser_input   input{ std::u8string_view(text) };
    const auto          error = thrown_error(
        [&]
        {
            (void)input.peek_while(
                [](const char8_t c)
                {
                    return c != static_cast<char8_t>(0x80);
                }
            );
        }
    );
    REQUIRE(error.has_value());
    CHECK(contains(error->to_message(), u8"0x80"));
}

// ============================================================ parse_exception

TEST_CASE(
    "parse_exception: an std::exception carrying the error",
    "[parser_result][exception]"
)
{
    STATIC_CHECK(std::derived_from<pnp::parse_exception, std::exception>);
    STATIC_CHECK(not std::is_default_constructible_v<pnp::parse_exception>);

    pnp::parser_input input(std::u8string_view(u8"ab"));
    // Caught as itself: the error inside is the full error.
    try
    {
        input.advance(3);
        FAIL("advance(3) should have thrown");
    }
    catch (const pnp::parse_exception& exception)
    {
        CHECK(exception.error().code() == error_code::invalid_parser_advanced_past_end);
        CHECK(exception.error().details().requested == 3);
    }
    // Caught as an std::exception: what() is the message, as UTF-8 chars.
    try
    {
        input.advance(3);
        FAIL("advance(3) should have thrown");
    }
    catch (const std::exception& exception)
    {
        const auto& parse = dynamic_cast<const pnp::parse_exception&>(exception);
        const std::string_view what = exception.what();
        CHECK(as_u8(what) == parse.error().to_message());
        CHECK(what.starts_with("At code unit 0: "));
        // Built once, then the same text each time.
        CHECK(exception.what() == exception.what());
    }
}

TEST_CASE(
    "parse_exception: what() keeps non-ASCII text intact",
    "[parser_result][exception]"
)
{
    // The token and escape are copied into the message byte for byte.
    pnp::parser_input input(std::u8string_view(u8"abc"));
    try
    {
        (void)input.peek_until(u8"éa", u8"éb");
        FAIL("peek_until should have thrown");
    }
    catch (const pnp::parse_exception& exception)
    {
        const std::string_view what = exception.what();
        CHECK(as_u8(what) == exception.error().to_message());
        CHECK(contains(as_u8(what), u8"\"éa\""));
    }
}