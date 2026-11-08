#pragma once

#include "pnp/sinks/string.hpp"
#include "pnp/core/print/text_sink.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <memory>
#include <memory_resource>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace string_sink_tests
{
    using PNP_NAMESPACE::output_string_c;
    using PNP_NAMESPACE::string_sink;
    using PNP_NAMESPACE::text_sink;

    template <typename T>
    struct custom_allocator
    {
        using value_type = T;

        custom_allocator() = default;

        template <typename U>
        custom_allocator(const custom_allocator<U>&) noexcept
        {
        }

        T*
            allocate(const std::size_t count_arg)
        {
            return std::allocator<T>{}.allocate(count_arg);
        }

        void
            deallocate(
                T* ptr_arg,
                const std::size_t count_arg
            ) noexcept
        {
            std::allocator<T>{}.deallocate(ptr_arg, count_arg);
        }

        bool operator==(const custom_allocator&) const = default;
    };

    template <typename T>
    struct small_allocator
    {
        using value_type = T;

        small_allocator() = default;

        template <typename U>
        small_allocator(const small_allocator<U>&) noexcept
        {
        }

        T*
            allocate(const std::size_t count_arg)
        {
            return std::allocator<T>{}.allocate(count_arg);
        }

        void
            deallocate(
                T* ptr_arg,
                const std::size_t count_arg
            ) noexcept
        {
            std::allocator<T>{}.deallocate(ptr_arg, count_arg);
        }

        std::size_t
            max_size() const noexcept
        {
            return 64;
        }

        bool operator==(const small_allocator&) const = default;
    };

    struct custom_traits : std::char_traits<char>
    {
    };

    using custom_string = std::basic_string<char, std::char_traits<char>, custom_allocator<char>>;
    using custom_u8string = std::basic_string<char8_t, std::char_traits<char8_t>, custom_allocator<char8_t>>;
    using small_string = std::basic_string<char, std::char_traits<char>, small_allocator<char>>;
    using custom_traits_string = std::basic_string<char, custom_traits>;

    template <typename String>
    concept can_name_string_sink = requires { typename string_sink<String>; };

    // ---- output_string_c ----

    static_assert(output_string_c<std::string>);
    static_assert(output_string_c<std::u8string>);
    static_assert(output_string_c<std::pmr::string>);
    static_assert(output_string_c<std::pmr::u8string>);
    static_assert(output_string_c<custom_string>);
    static_assert(output_string_c<custom_u8string>);

    static_assert(not output_string_c<std::wstring>);
    static_assert(not output_string_c<std::u16string>);
    static_assert(not output_string_c<std::u32string>);
    static_assert(not output_string_c<custom_traits_string>);
    static_assert(not output_string_c<std::string_view>);
    static_assert(not output_string_c<std::u8string_view>);
    static_assert(not output_string_c<std::vector<char>>);
    static_assert(not output_string_c<const std::string>);
    static_assert(not output_string_c<std::string&>);
    static_assert(not output_string_c<int>);

    static_assert(can_name_string_sink<std::string>);
    static_assert(can_name_string_sink<std::pmr::u8string>);
    static_assert(not can_name_string_sink<std::wstring>);
    static_assert(not can_name_string_sink<std::vector<char>>);

    // ---- string_sink properties ----

    static_assert(std::is_base_of_v<text_sink, string_sink<std::string>>);
    static_assert(not std::is_default_constructible_v<string_sink<std::string>>);
    static_assert(std::is_nothrow_constructible_v<string_sink<std::string>, std::string&>);
    static_assert(not std::is_convertible_v<std::string&, string_sink<std::string>>);
    static_assert(not std::is_constructible_v<string_sink<std::string>, const std::string&>);
    static_assert(not std::is_constructible_v<string_sink<std::string>, std::string&&>);
    static_assert(not std::is_constructible_v<string_sink<std::string>, std::u8string&>);
    static_assert(not std::is_copy_constructible_v<string_sink<std::string>>);
    static_assert(not std::is_copy_assignable_v<string_sink<std::string>>);
    static_assert(not std::is_move_constructible_v<string_sink<std::string>>);

    // ---- Writing ----

    TEST_CASE("string_sink<std::u8string> writes UTF-8 text unchanged", "[string_sink]")
    {
        std::u8string              out;
        string_sink<std::u8string> sink{ out };

        SECTION("ASCII")
        {
            sink.write(u8"hello");

            REQUIRE(out == u8"hello");
        }

        SECTION("non-ASCII")
        {
            sink.write(u8"café ☕ 𝄞");

            REQUIRE(out == u8"café ☕ 𝄞");
        }

        SECTION("std::string_view text")
        {
            sink.write("caf\xC3\xA9");

            REQUIRE(out == u8"café");
        }

        SECTION("single code units")
        {
            sink.put(u8'x');
            sink.put(static_cast<char8_t>(0xC3));
            sink.put(static_cast<char8_t>(0xA9));

            REQUIRE(out == u8"xé");
        }
    }

    TEST_CASE("string_sink<std::string> writes the UTF-8 bytes", "[string_sink]")
    {
        std::string              out;
        string_sink<std::string> sink{ out };

        SECTION("ASCII")
        {
            sink.write(u8"hello");

            REQUIRE(out == "hello");
        }

        SECTION("non-ASCII")
        {
            sink.write(u8"café ☕ 𝄞");

            REQUIRE(out == "caf\xC3\xA9 \xE2\x98\x95 \xF0\x9D\x84\x9E");
        }

        SECTION("std::string_view text through the derived type")
        {
            sink.write("hello");

            REQUIRE(out == "hello");
        }

        SECTION("single code units")
        {
            sink.put(static_cast<char8_t>(0xC3));
            sink.put(static_cast<char8_t>(0xA9));

            REQUIRE(out == "\xC3\xA9");
        }
    }

    TEST_CASE("string_sink handles empty text and embedded nulls", "[string_sink]")
    {
        std::string              out;
        string_sink<std::string> sink{ out };

        SECTION("empty text")
        {
            sink.write(std::u8string_view{});
            sink.write(std::string_view{});

            REQUIRE(out.empty());
        }

        SECTION("embedded null characters")
        {
            sink.write(std::u8string_view{ u8"a\0b", 3 });
            sink.put(u8'\0');

            REQUIRE(out == std::string("a\0b\0", 4));
        }
    }

    TEST_CASE("string_sink keeps the order of mixed writes", "[string_sink]")
    {
        std::u8string              out;
        string_sink<std::u8string> sink{ out };
        text_sink& base = sink;

        base.write(u8"[");
        base.put(u8'1');
        base.write(", ");
        base.write(u8"é");
        base.put(u8']');

        REQUIRE(out == u8"[1, é]");
    }

    TEST_CASE("string_sink appends to the existing string", "[string_sink]")
    {
        std::string out = "before|";

        {
            string_sink<std::string> sink{ out };
            sink.write(u8"during");
        }

        out += "|after";

        REQUIRE(out == "before|during|after");
    }

    TEST_CASE("string_sink sees the string as it grows", "[string_sink]")
    {
        std::string              out;
        string_sink<std::string> sink{ out };

        sink.write(u8"first");
        REQUIRE(out == "first");

        for (int i = 0; i < 1000; ++i)
        {
            sink.put(u8'x');
        }

        REQUIRE(out.size() == 1005);
        REQUIRE(out.substr(0, 5) == "first");
        REQUIRE(out.back() == 'x');
    }

    TEST_CASE("string_sink sinks sharing a string interleave", "[string_sink]")
    {
        std::string              out;
        string_sink<std::string> first{ out };
        string_sink<std::string> second{ out };

        first.write(u8"a");
        second.write(u8"b");
        first.put(u8'c');

        REQUIRE(out == "abc");
    }

    // ---- Other allocators ----

    TEST_CASE("string_sink works with std::pmr strings", "[string_sink][allocator]")
    {
        std::byte                           buffer[256];
        std::pmr::monotonic_buffer_resource resource{ buffer, sizeof(buffer) };

        SECTION("std::pmr::string")
        {
            std::pmr::string              out{ &resource };
            string_sink<std::pmr::string> sink{ out };

            sink.write(u8"café");
            sink.put(u8'!');

            REQUIRE(out == "caf\xC3\xA9!");
            REQUIRE(out.get_allocator().resource() == &resource);
        }

        SECTION("std::pmr::u8string")
        {
            std::pmr::u8string              out{ &resource };
            string_sink<std::pmr::u8string> sink{ out };

            sink.write(u8"café");
            sink.put(u8'!');

            REQUIRE(out == u8"café!");
            REQUIRE(out.get_allocator().resource() == &resource);
        }
    }

    TEST_CASE("string_sink works with a custom allocator", "[string_sink][allocator]")
    {
        SECTION("char")
        {
            custom_string              out;
            string_sink<custom_string> sink{ out };

            sink.write(u8"custom ☕");

            REQUIRE(out == "custom \xE2\x98\x95");
        }

        SECTION("char8_t")
        {
            custom_u8string              out;
            string_sink<custom_u8string> sink{ out };

            sink.write(u8"custom ☕");

            REQUIRE(out == u8"custom ☕");
        }
    }

    // ---- Failures ----

    TEST_CASE("string_sink write is all or nothing on failure", "[string_sink][failure]")
    {
        small_string              out;
        string_sink<small_string> sink{ out };

        sink.write(u8"0123456789");
        REQUIRE(out == "0123456789");

        const std::u8string too_long(200, u8'z');

        REQUIRE_THROWS_AS(sink.write(too_long), std::length_error);
        REQUIRE(out == "0123456789");

        sink.write(u8"!");
        REQUIRE(out == "0123456789!");
    }

    // ---- Class template argument deduction ----

    TEST_CASE("string_sink deduces the string type", "[string_sink][ctad]")
    {
        std::string   narrow;
        std::u8string wide;

        string_sink narrow_sink{ narrow };
        string_sink wide_sink{ wide };

        STATIC_REQUIRE(std::same_as<decltype(narrow_sink), string_sink<std::string>>);
        STATIC_REQUIRE(std::same_as<decltype(wide_sink), string_sink<std::u8string>>);

        narrow_sink.write(u8"a");
        wide_sink.write(u8"b");

        REQUIRE(narrow == "a");
        REQUIRE(wide == u8"b");
    }
}