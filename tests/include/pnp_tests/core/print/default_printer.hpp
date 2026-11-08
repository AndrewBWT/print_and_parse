#pragma once
#include "pnp/core/concepts.hpp"
#include "pnp/core/print/default_printer.hpp"
#include "pnp/core/print/free_functions.hpp"
#include "pnp_tests_internal.hpp"

#include <string>
#include <string_view>
#include <type_traits>

namespace default_printer_tests
{
    // No default_printer is specialised for this.
    struct unspecialised
    {
    };

    // One is, below.
    struct specialised
    {
        int value = 0;
    };

} // namespace default_printer_tests

PNP_NS_BEGIN

template <>
struct default_printer<default_printer_tests::specialised>
{
    static constexpr bool is_specialized = true;

    void
        run_printer(
            const default_printer_tests::specialised& value_arg,
            text_sink& sink_arg
        ) const
    {
        sink_arg.write(std::string_view(std::to_string(value_arg.value)));
    }
};

PNP_NS_END

using namespace default_printer_tests;

TEST_CASE(
    "default_printer: the unspecialised template",
    "[print][default_printer]"
)
{
    using primary_t = pnp::default_printer<unspecialised>;
    // Marked unspecialised, and holds nothing.
    STATIC_CHECK(not primary_t::is_specialized);
    STATIC_CHECK(std::is_empty_v<primary_t>);
    STATIC_CHECK(std::is_default_constructible_v<primary_t>);
    // So it isn't a printer, and the type isn't default-printable.
    STATIC_CHECK(not pnp::printer_c<primary_t, unspecialised>);
    STATIC_CHECK(not pnp::default_printable_c<unspecialised>);
}

TEST_CASE(
    "default_printer: a specialisation makes a type default-printable",
    "[print][default_printer]"
)
{
    using specialised_t = pnp::default_printer<specialised>;
    STATIC_CHECK(specialised_t::is_specialized);
    STATIC_CHECK(pnp::printer_c<specialised_t, specialised>);
    STATIC_CHECK(pnp::default_printable_c<specialised>);

    // And the free functions find it.
    CHECK(pnp::to_u8string(specialised{ .value = 42 }) == u8"42");
}

TEST_CASE(
    "default_printer: specialising one type affects no other",
    "[print][default_printer]"
)
{
    // A specialisation for specialised says nothing about types that merely
    // look like it.
    struct lookalike
    {
        int value = 0;
    };

    STATIC_CHECK(not pnp::default_printable_c<lookalike>);
    STATIC_CHECK(not pnp::default_printer<lookalike>::is_specialized);
}