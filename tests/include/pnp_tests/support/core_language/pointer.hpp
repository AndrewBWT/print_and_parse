#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pnp_tests
{
    using int_pointer_printer = pnp::support::generic_pointer_printer<int*>;
    using c_string_printer = pnp::support::generic_c_string_pointer_printer<const char*>;

    // The text an address should print as, built independently of the printer.
    inline std::u8string
        expected_address(const void* pointer_arg, std::u8string_view prefix_arg = u8"0x", bool upper_arg = false,
            bool padded_arg = true)
    {
        char        buffer[2 * sizeof(std::uintptr_t)];
        const char* end = std::to_chars(buffer, buffer + sizeof(buffer), reinterpret_cast<std::uintptr_t>(pointer_arg), 16).ptr;

        std::u8string digits(unicode_bridge::cast_string_view<char8_t>(std::string_view(buffer, end)));
        if (upper_arg)
        {
            for (char8_t& digit : digits)
            {
                if (digit >= u8'a' && digit <= u8'f')
                {
                    digit = static_cast<char8_t>(digit - (u8'a' - u8'A'));
                }
            }
        }
        if (padded_arg)
        {
            digits.insert(0, sizeof(std::uintptr_t) * 2 - digits.size(), u8'0');
        }
        return std::u8string(prefix_arg) + digits;
    }

    void test_function_for_pointers() {}

    // Policies for the pointer printer.
    struct pointer_test_bad_null
    {
        static constexpr std::u8string_view null_id = u8"";
    };
    struct pointer_test_bad_address : pnp::support::fixed_width_pnp_unsigned_hex
    {
        static constexpr std::u8string_view prefix = u8" 0x";
    };
    struct pointer_test_null_word
    {
        static constexpr std::u8string_view null_id = u8"null";
    };

    struct pointer_policy_custom_null
    {
        using null_defaults = pointer_test_null_word;
        using address_defaults = pnp::support::pointer_address_default;
    };
    struct pointer_policy_bad_null
    {
        using null_defaults = pointer_test_bad_null;
        using address_defaults = pnp::support::pointer_address_default;
    };
    struct pointer_policy_bad_address
    {
        using null_defaults = pnp::support::nullptr_pnp_default;
        using address_defaults = pointer_test_bad_address;
    };

    // Policies for the C-string pointer printer.
    struct c_string_test_unescaped_close
    {
        static constexpr std::u8string_view open_delimiter = u8"<";
        static constexpr std::u8string_view close_delimiter = u8">";
    };
    struct c_string_policy_bad_text
    {
        using null_defaults = pnp::support::nullptr_pnp_default;
        using text_defaults = c_string_test_unescaped_close;
    };
} // namespace pnp_tests

using namespace pnp_tests;

// ============================================================ pointers as addresses

TEST_CASE("pointers: null pointers print the null token", "[pointers]")
{
    // Composed of a fixed-width null token and an address, but not fixed width itself.
    CHECK(not int_pointer_printer{}.fixed_output_width().has_value());
    CHECK(pnp::to_u8string(static_cast<int*>(nullptr)) == u8"nullptr");
    CHECK(pnp::to_u8string(static_cast<const int*>(nullptr)) == u8"nullptr");
    CHECK(pnp::to_u8string(static_cast<void*>(nullptr)) == u8"nullptr");
    CHECK(pnp::to_u8string(static_cast<void (*)()>(nullptr)) == u8"nullptr");

    // A custom null token, through options and through a policy.
    const int_pointer_printer null_word({ .null = int_pointer_printer::null_part_type({.null_id = u8"null"}) });
    CHECK(pnp::to_u8string(null_word, static_cast<int*>(nullptr)) == u8"null");

    const pnp::support::generic_pointer_printer<int*, pointer_policy_custom_null> policy_null;
    CHECK(pnp::to_u8string(policy_null, static_cast<int*>(nullptr)) == u8"null");
}

TEST_CASE("pointers: addresses print in padded hex", "[pointers]")
{
    int        value = 0;
    int* const pointer = &value;

    const auto text = pnp::to_u8string(pointer);
    CHECK(text == expected_address(pointer));
    CHECK(text.starts_with(u8"0x"));
    CHECK(text.size() == 2 + 2 * sizeof(void*));   // the prefix, then every digit of the address

    // Other pointer types print the same way.
    const int* const const_pointer = &value;
    CHECK(pnp::to_u8string(const_pointer) == expected_address(const_pointer));
    void* const void_pointer = &value;
    CHECK(pnp::to_u8string(void_pointer) == expected_address(void_pointer));

    // A function pointer prints its address too.
    void (* const function_pointer)() = &test_function_for_pointers;
    CHECK(pnp::to_u8string(function_pointer).size() == 2 + 2 * sizeof(function_pointer));
}

TEST_CASE("pointers: configuring the address part", "[pointers]")
{
    int        value = 0;
    int* const pointer = &value;

    const int_pointer_printer upper({ .address = int_pointer_printer::address_part_type({.upper = true}) });
    CHECK(pnp::to_u8string(upper, pointer) == expected_address(pointer, u8"0x", true));

    const int_pointer_printer unpadded(
        { .address = int_pointer_printer::address_part_type({.fixed_size_output = false}) });
    CHECK(pnp::to_u8string(unpadded, pointer) == expected_address(pointer, u8"0x", false, false));

    const int_pointer_printer custom_prefix({ .address = int_pointer_printer::address_part_type({.prefix = u8"@"}) });
    CHECK(pnp::to_u8string(custom_prefix, pointer) == expected_address(pointer, u8"@"));

    const int_pointer_printer no_prefix({ .address = int_pointer_printer::address_part_type({.prefix = u8""}) });
    CHECK(pnp::to_u8string(no_prefix, pointer) == expected_address(pointer, u8""));

    // Both parts at once; the null part doesn't affect non-null pointers, and vice versa.
    const int_pointer_printer both({
        .null = int_pointer_printer::null_part_type({.null_id = u8"NULL"}),
        .address = int_pointer_printer::address_part_type({.upper = true}),
        });
    CHECK(pnp::to_u8string(both, pointer) == expected_address(pointer, u8"0x", true));
    CHECK(pnp::to_u8string(both, static_cast<int*>(nullptr)) == u8"NULL");

    // The options read back, and a modified copy keeps the other part.
    CHECK(both.options().null.options().null_id == u8"NULL");
    CHECK(both.options().address.options().upper);
    auto modified = both.options();
    modified.null = int_pointer_printer::null_part_type({ .null_id = u8"nil" });
    const int_pointer_printer copy(modified);
    CHECK(pnp::to_u8string(copy, static_cast<int*>(nullptr)) == u8"nil");
    CHECK(pnp::to_u8string(copy, pointer) == expected_address(pointer, u8"0x", true));

    // The defaults inherit the options constructor.
    const pnp::default_printer<int*> default_upper(
        { .address = int_pointer_printer::address_part_type({.upper = true}) });
    CHECK(pnp::to_u8string(default_upper, pointer) == expected_address(pointer, u8"0x", true));
}

TEST_CASE("pointers: invalid parts throw when the part is built", "[pointers]")
{
    // Each part validates itself, with its own options type in the message.
    CHECK_THROWS_AS(int_pointer_printer::null_part_type({ .null_id = u8"" }), std::invalid_argument);
    CHECK_THROWS_AS(int_pointer_printer::null_part_type({ .null_id = half_a_character }), std::invalid_argument);
    CHECK_THROWS_AS(int_pointer_printer::address_part_type({ .prefix = u8" 0x" }), std::invalid_argument);
    CHECK_THROWS_AS(int_pointer_printer::address_part_type({ .base = 1 }), std::invalid_argument);

    CHECK(invalid_argument_message([] { int_pointer_printer::null_part_type p({ .null_id = u8"" }); })
        == "nullptr_pnp_options null id must be non-empty");
    CHECK(invalid_argument_message([] { int_pointer_printer::address_part_type p({ .prefix = u8"0x " }); })
        == "fixed_width_pnp_options prefix must not start or end with whitespace");
}

TEST_CASE("pointers: inside containers", "[pointers]")
{
    int a = 0;
    int b = 0;
    const std::vector<int*> pointers{ &a, nullptr, &b };
    const std::u8string expected = u8"[" + expected_address(&a) + u8",nullptr," + expected_address(&b) + u8"]";
    CHECK(pnp::to_u8string(pointers) == expected);
}

// ============================================================ pointers as C strings

TEST_CASE("c-string pointers: print as quoted strings", "[pointers][c_strings]")
{
    CHECK(pnp::to_u8string(static_cast<const char*>("abc")) == u8"\"abc\"");

    char mutable_text[] = "abc";
    CHECK(pnp::to_u8string(static_cast<char*>(mutable_text)) == u8"\"abc\"");

    CHECK(pnp::to_u8string(static_cast<const wchar_t*>(L"abc")) == u8"\"abc\"");
    CHECK(pnp::to_u8string(static_cast<const char8_t*>(u8"abc")) == u8"\"abc\"");
    CHECK(pnp::to_u8string(static_cast<const char16_t*>(u"abc")) == u8"\"abc\"");
    CHECK(pnp::to_u8string(static_cast<const char32_t*>(U"abc")) == u8"\"abc\"");

    // The empty string.
    CHECK(pnp::to_u8string(static_cast<const char*>("")) == u8"\"\"");
}

TEST_CASE("c-string pointers: print exactly as the equivalent string", "[pointers][c_strings]")
{
    for (const char* text : { "say \"hi\"", "back\\slash", "it's", "tab\there", "line\nbreak", "caf\xC3\xA9" })
    {
        CHECK(pnp::to_u8string(text) == pnp::to_u8string(std::string(text)));
    }
    const char8_t* u8_text = u8"日本語 😀";
    CHECK(pnp::to_u8string(u8_text) == pnp::to_u8string(std::u8string(u8_text)));
    const char16_t* u16_text = u"😀 surrogate pair";
    CHECK(pnp::to_u8string(u16_text) == pnp::to_u8string(std::u16string(u16_text)));
}

TEST_CASE("c-string pointers: text stops at the first null", "[pointers][c_strings]")
{
    // Content after an embedded null isn't printed.
    const char embedded[] = "ab\0cd";
    CHECK(pnp::to_u8string(static_cast<const char*>(embedded)) == u8"\"ab\"");

    // A pointer into the middle of a string prints from there to the terminator.
    const char* const whole = "prefix:value";
    CHECK(pnp::to_u8string(whole + 7) == u8"\"value\"");
}

TEST_CASE("c-string pointers: null pointers print the null token", "[pointers][c_strings]")
{
    CHECK(pnp::to_u8string(static_cast<const char*>(nullptr)) == u8"nullptr");
    CHECK(pnp::to_u8string(static_cast<char*>(nullptr)) == u8"nullptr");
    CHECK(pnp::to_u8string(static_cast<const char16_t*>(nullptr)) == u8"nullptr");

    const c_string_printer null_word({ .null = c_string_printer::null_part_type({.null_id = u8"NULL"}) });
    CHECK(pnp::to_u8string(null_word, static_cast<const char*>(nullptr)) == u8"NULL");
}

TEST_CASE("c-string pointers: configuring the text part", "[pointers][c_strings]")
{
    const char* const text = "abc";

    const c_string_printer angled(
        { .text = c_string_printer::text_part_type({.open_delimiter = u8"<'", .close_delimiter = u8"'>"}) });
    CHECK(pnp::to_u8string(angled, text) == u8"<'abc'>");

    // Both parts at once.
    const c_string_printer both({
        .null = c_string_printer::null_part_type({.null_id = u8"NULL"}),
        .text = c_string_printer::text_part_type({.open_delimiter = u8"«", .close_delimiter = u8"\"»"}),
        });
    CHECK(pnp::to_u8string(both, text) == u8"«abc\"»");
    CHECK(pnp::to_u8string(both, static_cast<const char*>(nullptr)) == u8"NULL");

    // The text part prints exactly as a string pnp with the same options.
    using string_pnp = pnp::support::generic_string_pnp<std::string, pnp::support::string_pnp_default>;
    const string_pnp matching({ .open_delimiter = u8"«", .close_delimiter = u8"\"»" });
    CHECK(pnp::to_u8string(both, text) == pnp::to_u8string(matching, std::string(text)));

    // The defaults inherit the options constructor.
    const pnp::default_printer<const char*> default_angled(
        { .text = c_string_printer::text_part_type({.open_delimiter = u8"<'", .close_delimiter = u8"'>"}) });
    CHECK(pnp::to_u8string(default_angled, text) == u8"<'abc'>");
}

TEST_CASE("c-string pointers: invalid parts throw when the part is built", "[pointers][c_strings]")
{
    CHECK_THROWS_AS(c_string_printer::text_part_type({ .close_delimiter = u8">" }), std::invalid_argument);
    CHECK_THROWS_AS(c_string_printer::text_part_type({ .open_delimiter = u8"" }), std::invalid_argument);
    CHECK_THROWS_AS(c_string_printer::null_part_type({ .null_id = u8" null" }), std::invalid_argument);

    CHECK(invalid_argument_message([] { c_string_printer::text_part_type p({ .open_delimiter = u8"" }); })
        == "string_pnp_options open delimiter must be non-empty");
}

TEST_CASE("c-string pointers: inside containers", "[pointers][c_strings]")
{
    const std::vector<const char*> words{ "a", nullptr, "b,c" };
    CHECK(pnp::to_u8string(words) == u8"[\"a\",nullptr,\"b,c\"]");
}

// ============================================================ compile-time checks

// --- Policies -------------------------------------------------------------------
static_assert(pnp::support::valid_pointer_printer_c<pnp::support::pointer_printer_default>);
static_assert(pnp::support::valid_pointer_printer_c<pointer_policy_custom_null>);
static_assert(not pnp::support::valid_pointer_printer_c<pointer_policy_bad_null>);
static_assert(not pnp::support::valid_pointer_printer_c<pointer_policy_bad_address>);

static_assert(pnp::support::valid_c_string_pointer_printer_c<pnp::support::c_string_pointer_printer_default>);
static_assert(not pnp::support::valid_c_string_pointer_printer_c<c_string_policy_bad_text>);

// --- Which default each pointer type gets --------------------------------------
template <typename Pointer>
inline constexpr bool prints_as_address
= std::is_base_of_v<pnp::support::generic_pointer_printer<Pointer>, pnp::default_printer<Pointer>>;

template <typename Pointer>
inline constexpr bool prints_as_c_string
= std::is_base_of_v<pnp::support::generic_c_string_pointer_printer<Pointer>, pnp::default_printer<Pointer>>;

static_assert(prints_as_address<int*>);
static_assert(prints_as_address<const int*>);
static_assert(prints_as_address<void*>);
static_assert(prints_as_address<const void*>);
static_assert(prints_as_address<void (*)()>);
static_assert(prints_as_address<signed char*>, "a byte buffer, not text");
static_assert(prints_as_address<unsigned char*>, "a byte buffer, not text");
static_assert(prints_as_address<std::byte*>);

static_assert(prints_as_c_string<char*>);
static_assert(prints_as_c_string<const char*>);
static_assert(prints_as_c_string<const wchar_t*>);
static_assert(prints_as_c_string<const char8_t*>);
static_assert(prints_as_c_string<const char16_t*>);
static_assert(prints_as_c_string<const char32_t*>);

// Every pointer has exactly one default.
static_assert(not (prints_as_address<const char*> || prints_as_c_string<int*>));

// --- Print-only ----------------------------------------------------------------
static_assert(pnp::default_printable_c<int*>);
static_assert(pnp::default_printable_c<const char*>);
static_assert(not pnp::default_parseable_c<int*>);
static_assert(not pnp::default_parseable_c<const char*>);

// --- The exact-type guard ----------------------------------------------------
static_assert(pnp::printer_c<pnp::default_printer<int*>, int*>);
static_assert(not pnp::printer_c<pnp::default_printer<int*>, const int*>);
static_assert(not pnp::printer_c<pnp::default_printer<const int*>, int*>);
static_assert(not pnp::printer_c<pnp::default_printer<int*>, std::nullptr_t>);
static_assert(not pnp::printer_c<pnp::default_printer<void*>, int*>, "no implicit conversion to void*");

static_assert(pnp::printer_c<pnp::default_printer<const char*>, const char*>);
static_assert(not pnp::printer_c<pnp::default_printer<const char*>, char*>);
static_assert(not pnp::printer_c<pnp::default_printer<char*>, const char*>);
static_assert(not pnp::printer_c<pnp::default_printer<const char*>, std::string>);
static_assert(not pnp::printer_c<pnp::default_printer<const char*>, std::string_view>);