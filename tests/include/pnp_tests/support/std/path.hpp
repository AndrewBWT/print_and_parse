#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <filesystem>
#include <string_view>
#include <vector>

namespace pnp_tests
{
    using path_pnp = pnp::support::generic_path_pnp<>;

    template <typename T>
    void
        check_path_round_trip(const T& value_arg, std::u8string_view expected_arg)
    {
        CHECK(pnp::to_u8string(value_arg) == expected_arg);
        const auto parsed = pnp::try_parse<T>(expected_arg);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value_arg);
    }
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("path: a quoted string", "[path]")
{
    check_path_round_trip(std::filesystem::path(u8"docs/report.txt"), u8"\"docs/report.txt\"");
    check_path_round_trip(std::filesystem::path(u8"/usr/local/bin"), u8"\"/usr/local/bin\"");
    check_path_round_trip(std::filesystem::path{}, u8"\"\"");

    // Spaces need no escaping: the quotes mark the end.
    check_path_round_trip(std::filesystem::path(u8"my docs/annual report.txt"), u8"\"my docs/annual report.txt\"");

    // Non-ASCII names.
    check_path_round_trip(std::filesystem::path(u8"données/résumé.txt"), u8"\"données/résumé.txt\"");

    // Inside containers.
    check_path_round_trip(std::vector<std::filesystem::path>{u8"a", u8"b/c"}, u8"[\"a\",\"b/c\"]");
}

TEST_CASE("path: generic and native formats", "[path]")
{
#if defined(_WIN32)
    // On Windows, the native separator is a backslash; the generic format avoids escaping it.
    const std::filesystem::path windows(u8"C:\\docs\\report.txt");
    CHECK(pnp::to_u8string(windows) == u8"\"C:/docs/report.txt\"");
    CHECK(pnp::to_u8string(path_pnp({ .generic_format = false }), windows) == u8"\"C:\\\\docs\\\\report.txt\"");

    // Either form parses back to an equal path.
    const auto from_generic = pnp::try_parse<std::filesystem::path>(u8"\"C:/docs/report.txt\"");
    REQUIRE(succeeded(from_generic));
    CHECK(from_generic.value == windows);
#else
    // Elsewhere, the two formats are the same.
    const std::filesystem::path posix(u8"/home/user/report.txt");
    CHECK(pnp::to_u8string(path_pnp({ .generic_format = false }), posix) == pnp::to_u8string(posix));
#endif
}

TEST_CASE("path: configuring the text part", "[path]")
{
    const path_pnp angled({ .text = path_pnp::text_part_type({.open_delimiter = u8"<'", .close_delimiter = u8"'>"}) });
    CHECK(pnp::to_u8string(angled, std::filesystem::path(u8"a/b")) == u8"<'a/b'>");
    const auto parsed = pnp::try_parse<std::filesystem::path>(u8"<'a/b'>", angled);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == std::filesystem::path(u8"a/b"));
}

TEST_CASE("path: parse errors come from the string", "[path]")
{
    CHECK(not succeeded(pnp::try_parse<std::filesystem::path>(u8"docs/report.txt")));   // unquoted
    CHECK(not succeeded(pnp::try_parse<std::filesystem::path>(u8"\"docs")));            // unclosed
    CHECK(not succeeded(pnp::try_parse<std::filesystem::path>(u8"\"a\"x")));
}

static_assert(pnp::default_printable_c<std::filesystem::path>);
static_assert(pnp::default_parseable_c<std::filesystem::path>);
static_assert(not pnp::printer_c<pnp::default_printer<std::filesystem::path>, std::u8string>);