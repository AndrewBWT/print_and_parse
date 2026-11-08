#pragma once

#include "pnp/core/print/print_exception.hpp"
#include "pnp/core/print/function_printer.hpp"
#include "pnp/core/print/text_sink.hpp"

#include <catch2/catch_test_macros.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace print_exception_tests
{
    using PNP_NAMESPACE::function_printer;
    using PNP_NAMESPACE::print_exception;
    using PNP_NAMESPACE::string_sink;
    using PNP_NAMESPACE::text_sink;

    static_assert(std::is_base_of_v<std::runtime_error, print_exception>);
    static_assert(std::is_base_of_v<std::exception, print_exception>);
    static_assert(not std::is_base_of_v<std::logic_error, print_exception>);

    static_assert(not std::is_default_constructible_v<print_exception>);

    static_assert(std::is_constructible_v<print_exception, const char*>);
    static_assert(std::is_constructible_v<print_exception, std::string>);
    static_assert(std::is_constructible_v<print_exception, const std::string&>);
    static_assert(std::is_constructible_v<print_exception, const char8_t*>);
    static_assert(std::is_constructible_v<print_exception, std::u8string>);
    static_assert(std::is_constructible_v<print_exception, std::u8string_view>);

    static_assert(not std::is_convertible_v<std::string, print_exception>);
    static_assert(not std::is_convertible_v<std::u8string_view, print_exception>);
    static_assert(not std::is_convertible_v<const char*, print_exception>);

    static_assert(std::is_nothrow_copy_constructible_v<print_exception>);
    static_assert(std::is_nothrow_copy_assignable_v<print_exception>);

    inline std::string_view
        message_of(
            const std::exception& exception_arg
        )
    {
        return std::string_view{ exception_arg.what() };
    }

    TEST_CASE("print_exception stores a std::string message", "[print_exception]")
    {
        SECTION("from a std::string")
        {
            const std::string     message = "cannot print value";
            const print_exception ex{ message };

            REQUIRE(message_of(ex) == "cannot print value");
        }

        SECTION("from a string literal")
        {
            const print_exception ex{ "literal message" };

            REQUIRE(message_of(ex) == "literal message");
        }

        SECTION("empty message")
        {
            const print_exception ex{ std::string{} };

            REQUIRE(message_of(ex).empty());
        }
    }

    TEST_CASE("print_exception stores a UTF-8 message", "[print_exception]")
    {
        SECTION("from a u8 literal")
        {
            const print_exception ex{ u8"cannot print value" };

            REQUIRE(message_of(ex) == "cannot print value");
        }

        SECTION("from a std::u8string")
        {
            const std::u8string   message = u8"from u8string";
            const print_exception ex{ message };

            REQUIRE(message_of(ex) == "from u8string");
        }

        SECTION("non-ASCII bytes are kept unchanged")
        {
            const print_exception ex{ u8"café ☕" };

            REQUIRE(message_of(ex) == "caf\xC3\xA9 \xE2\x98\x95");
        }

        SECTION("a view that isn't null-terminated uses only its own characters")
        {
            const std::u8string      text = u8"abcdef";
            const std::u8string_view view{ text.data(), 3 };
            const print_exception    ex{ view };

            REQUIRE(message_of(ex) == "abc");
        }

        SECTION("empty message")
        {
            const print_exception ex{ std::u8string_view{} };

            REQUIRE(message_of(ex).empty());
        }
    }

    TEST_CASE("print_exception copies keep the message", "[print_exception]")
    {
        const print_exception original{ u8"original ✓" };

        SECTION("copy construction")
        {
            const print_exception copy{ original };

            REQUIRE(message_of(copy) == message_of(original));
        }

        SECTION("copy assignment")
        {
            print_exception copy{ "something else" };
            copy = original;

            REQUIRE(message_of(copy) == message_of(original));
        }
    }

    TEST_CASE("print_exception can be caught as its base types", "[print_exception]")
    {
        SECTION("as std::runtime_error")
        {
            try
            {
                throw print_exception{ "as runtime_error" };
            }
            catch (const std::runtime_error& ex)
            {
                REQUIRE(message_of(ex) == "as runtime_error");
            }
        }

        SECTION("as std::exception")
        {
            try
            {
                throw print_exception{ u8"as exception" };
            }
            catch (const std::exception& ex)
            {
                REQUIRE(message_of(ex) == "as exception");
            }
        }

        SECTION("not as std::logic_error")
        {
            bool caught_as_logic_error = false;
            bool caught_as_print_error = false;

            try
            {
                throw print_exception{ "not a logic_error" };
            }
            catch (const std::logic_error&)
            {
                caught_as_logic_error = true;
            }
            catch (const print_exception&)
            {
                caught_as_print_error = true;
            }

            REQUIRE(not caught_as_logic_error);
            REQUIRE(caught_as_print_error);
        }
    }

    TEST_CASE("print_exception thrown by a user printer reaches the caller", "[print_exception]")
    {
        auto printer = function_printer<int>(
            [](const int& value_arg, text_sink& sink_arg)
            {
                sink_arg.write(u8"value: ");
                if (value_arg < 0)
                {
                    throw print_exception{ u8"negative values aren't supported" };
                }
                sink_arg.write(std::to_string(value_arg));
            }
        );

        std::u8string              out;
        string_sink<std::u8string> sink{ out };

        SECTION("the exception and its message pass through")
        {
            try
            {
                printer.run_printer(-1, sink);
                FAIL("print_exception was not thrown");
            }
            catch (const print_exception& ex)
            {
                REQUIRE(message_of(ex) == "negative values aren't supported");
            }
        }

        SECTION("partial output remains in the sink")
        {
            REQUIRE_THROWS_AS(printer.run_printer(-1, sink), print_exception);
            REQUIRE(out == u8"value: ");
        }

        SECTION("valid values don't throw")
        {
            REQUIRE_NOTHROW(printer.run_printer(3, sink));
            REQUIRE(out == u8"value: 3");
        }
    }
}