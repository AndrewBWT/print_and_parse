#pragma once
#include "pnp_tests_internal.hpp"
#include "pnp/support.hpp"

#include <charconv>
#include <complex>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace pnp_tests
{
    using complex_pnp = pnp::support::generic_complex_pnp<std::complex<double>, pnp::support::generic_float_pnp<double>>;

    template <typename C>
    void
        check_complex_round_trip(const C& value_arg, std::u8string_view expected_arg)
    {
        CHECK(pnp::to_u8string(value_arg) == expected_arg);
        const auto parsed = pnp::try_parse<C>(expected_arg);
        REQUIRE(succeeded(parsed));
        CHECK(parsed.value == value_arg);
    }
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE("complex: the real part, then the imaginary part", "[complex]")
{
    check_complex_round_trip(std::complex<double>(1, 2), u8"(1,2)");
    check_complex_round_trip(std::complex<double>(1.5, -0.25), u8"(1.5,-0.25)");
    check_complex_round_trip(std::complex<double>{}, u8"(0,0)");
    check_complex_round_trip(std::complex<float>(-3, 0.5f), u8"(-3,0.5)");

    // From arithmetic.
    using namespace std::complex_literals;
    CHECK(pnp::to_u8string(2.0 + 3.0i) == u8"(2,3)");

    // Inside containers.
    check_complex_round_trip(std::vector<std::complex<double>>{{1, 2}, { 3, 4 }}, u8"[(1,2),(3,4)]");
}

TEST_CASE("complex: configured through its pair of parts", "[complex]")
{
    const std::complex<double> value(3, 1);

    // Spacing and delimiters, as for any tuple.
    CHECK(pnp::to_u8string(complex_pnp({ .parts = complex_pnp::parts_type({.separator_spacing = u8" "}) }), value)
        == u8"(3, 1)");
    CHECK(pnp::to_u8string(complex_pnp({ .parts = complex_pnp::parts_type({.open_delimiter = u8"<", .close_delimiter = u8">"}) }),
        value)
        == u8"<3,1>");

    // The real and imaginary parts configured separately: hex for the real part.
    using value_pnp = pnp::support::generic_float_pnp<double>;
    const complex_pnp hex_real({ .parts = complex_pnp::parts_type({.elements = {value_pnp({.format = std::chars_format::hex}), value_pnp{}}}) });
    CHECK(pnp::to_u8string(hex_real, value) == u8"(0x1.8p+1,1)");
    const auto parsed = pnp::try_parse<std::complex<double>>(u8"(0x1.8p+1,1)", hex_real);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == value);

    // Lenient whitespace.
    const complex_pnp lenient({ .parts = complex_pnp::parts_type({.skip_whitespace = true}) });
    const auto        spaced = pnp::try_parse<std::complex<double>>(u8"( 3 , 1 )", lenient);
    REQUIRE(succeeded(spaced));
    CHECK(spaced.value == value);
}

TEST_CASE("complex: parse errors come from the pair", "[complex]")
{
    CHECK(first_message(pnp::try_parse<std::complex<double>>(u8"(1)"))
        == u8"Expected separator \",\" after element 0 of a tuple.");
    CHECK(not succeeded(pnp::try_parse<std::complex<double>>(u8"1,2")));
    CHECK(not succeeded(pnp::try_parse<std::complex<double>>(u8"(1,2,3)")));
    CHECK(not succeeded(pnp::try_parse<std::complex<double>>(u8"(x,2)")));
    CHECK(not succeeded(pnp::try_parse<std::complex<double>>(u8"(1,2)x")));

    // Invalid pair options throw when the pair is built.
    CHECK_THROWS_AS(complex_pnp::parts_type({ .separator = u8"" }), std::invalid_argument);
}

TEST_CASE("complex: not fixed width with the default parts", "[complex][fixed_width]")
{
    CHECK(not complex_pnp{}.fixed_output_width().has_value());
}

static_assert(pnp::default_printable_c<std::complex<double>>);
static_assert(pnp::default_parseable_c<std::complex<float>>);
static_assert(pnp::default_parseable_c<std::complex<long double>>);
static_assert(not pnp::printer_c<pnp::default_printer<std::complex<double>>, std::complex<float>>);
static_assert(not pnp::printer_c<pnp::default_printer<std::complex<double>>, std::pair<double, double>>);