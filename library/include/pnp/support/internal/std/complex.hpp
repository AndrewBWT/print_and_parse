#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/tuple_like.hpp"
#include "pnp/support/internal/utility.hpp"

#include <complex>
#include <concepts>
#include <cstddef>
#include <optional>
#include <tuple>
#include <unicode_bridge.hpp>
#include <utility>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename C>
concept std_complex_c = std::same_as<C, std::complex<typename C::value_type>>;

// As the standard's streams write it: (1,2), the real part then the imaginary
// part.
struct complex_pnp_default : tuple_pnp_default_base
{
    static constexpr std::u8string_view open_delimiter  = u8"(";
    static constexpr std::u8string_view close_delimiter = u8")";
    static constexpr std::u8string_view separator       = u8",";
};

// The pair a complex number is printed and parsed as: (real, imaginary), each
// part a pnp for the value type.
template <typename C, typename Value_Pnp, typename Complex_Pnp_Defaults>
using complex_parts_pnp = generic_tuple_pnp<
    std::pair<typename C::value_type, typename C::value_type>,
    std::tuple<Value_Pnp, Value_Pnp>,
    Complex_Pnp_Defaults>;

// One part: the pair pnp, which holds the delimiters, separator and spacing,
// and the real and imaginary parts as its elements. It validated itself, so
// there's nothing more to check here.
template <typename Parts_Pnp>
struct complex_pnp_options
{
    using parts_type = Parts_Pnp;

    parts_type parts = {};

    // No rules of its own yet: rules for any field added here go in this
    // function.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        return nullptr;
    }

    // A complex number prints exactly as its pair.
    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept
    {
        return fixed_output_width_of(parts);
    }
};

// Prints and parses a std::complex as a pair of its real and imaginary parts:
// (1,2).
//
//   using complex_pnp = pnp::generic_complex_pnp<std::complex<double>,
//   pnp::default_printer<double>>; complex_pnp p; // (1,2) complex_pnp
//   q({.parts = complex_pnp::parts_type({.separator_spacing = u8" "})});  //
//   (1, 2)
template <typename C, typename Value_Pnp, typename Complex_Pnp_Defaults = complex_pnp_default>
    requires std_complex_c<C>
&& (printer_c<Value_Pnp, typename C::value_type> || parser_c<Value_Pnp, typename C::value_type>)
struct generic_complex_pnp
    : public pnp_configuration<complex_pnp_options<complex_parts_pnp<C, Value_Pnp, Complex_Pnp_Defaults>>>
{
public:
    using base_t = pnp_configuration<complex_pnp_options<
        complex_parts_pnp<C, Value_Pnp, Complex_Pnp_Defaults>>>;
    using base_t::_options;
    using base_t::base_t;

    using parts_type = complex_parts_pnp<C, Value_Pnp, Complex_Pnp_Defaults>;
    constexpr void
        run_printer(
            const std::same_as<C> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const
    requires printer_c<Value_Pnp, typename C::value_type>;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            C&             object_answer,
            parser_run_key run_key_arg
        ) const
    requires parser_c<Value_Pnp, typename C::value_type>;
};

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename C, typename Value_Pnp, typename Complex_Pnp_Defaults>
    requires std_complex_c<C>
&& (printer_c<Value_Pnp, typename C::value_type> || parser_c<Value_Pnp, typename C::value_type>)
constexpr void
generic_complex_pnp<C, Value_Pnp, Complex_Pnp_Defaults>::run_printer(
    const std::same_as<C> auto& object_arg,
    text_sink& sink_obj_arg
) const requires printer_c<Value_Pnp, typename C::value_type>
{
    const std::pair<typename C::value_type, typename C::value_type> pair{
        object_arg.real(), object_arg.imag()
    };
    _options.parts.run_printer(pair, sink_obj_arg);
}

template <typename C, typename Value_Pnp, typename Complex_Pnp_Defaults>
    requires std_complex_c<C>
&& (printer_c<Value_Pnp, typename C::value_type> || parser_c<Value_Pnp, typename C::value_type>)
constexpr basic_parser_return_value
generic_complex_pnp<C, Value_Pnp, Complex_Pnp_Defaults>::run_parser(parser_input& parser_input_arg, C& object_answer, parser_run_key run_key_arg) const
    requires parser_c<Value_Pnp, typename C::value_type>
{
    // Parsed as a pair first, so a failed parse leaves the complex number
    // untouched.
    std::pair<typename C::value_type, typename C::value_type> pair{};
    if (auto result = parser_input_arg.run_parser(_options.parts, pair);
        result.failed())
    {
        result.add_msg(u8"while parsing a complex number");
        return result;
    }
    object_answer = C(pair.first, pair.second);
    return {};
}

PNP_SUPPORT_NS_END
PNP_NS_END