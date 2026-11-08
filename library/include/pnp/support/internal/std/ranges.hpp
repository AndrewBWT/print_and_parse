#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/sequences.hpp"

#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <unicode_bridge.hpp>
#include <utility>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// A range that can be printed: finite, and iterable either as const or through
// a copy.
template <typename R>
concept printable_range_c
    = std::ranges::input_range<R>
      && (std::ranges::input_range<const R> || std::copyable<R>)
      && (not std::
              same_as<std::ranges::sentinel_t<R>, std::unreachable_sentinel_t>);

// Prints a range (typically a view) in the sequence format, with the sequence
// pnp's options. Print-only: a view refers to data elsewhere, so there's
// nothing to parse into.
//
// Views whose begin() isn't const (filter_view, drop_while_view, ...) are
// printed through a copy. Elements whose reference type isn't the range's value
// type (transform_view's prvalues, zip_view's tuples of references) are
// converted to the value type first, so the element pnp sees exactly the type
// it was made for.
//
//   pnp::generic_range_printer<decltype(view), pnp::default_printer<int>> p; //
//   [2,4,6]
template <
    typename R,
    typename Element_Pnp,
    typename Sequence_Pnp_Defaults = sequence_pnp_default>
requires printable_range_c<R>
         && printer_c<Element_Pnp, std::ranges::range_value_t<R>>
         && valid_sequence_pnp_c<Sequence_Pnp_Defaults>
struct generic_range_printer
    : public pnp_configuration<
          sequence_pnp_options<Element_Pnp, Sequence_Pnp_Defaults, R>>
{
public:
    using base_t = pnp_configuration<
        sequence_pnp_options<Element_Pnp, Sequence_Pnp_Defaults, R>>;
    using base_t::_options;
    using base_t::base_t;

    using element_part_type = Element_Pnp;
    constexpr void
        run_printer(
            const std::same_as<R> auto& object_arg,
            text_sink&     sink_obj_arg
        ) const;
private:
    using value_type = std::ranges::range_value_t<R>;
};

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename R, typename Element_Pnp, typename Sequence_Pnp_Defaults>
requires printable_range_c<R>
         && printer_c<Element_Pnp, std::ranges::range_value_t<R>>
         && valid_sequence_pnp_c<Sequence_Pnp_Defaults>
constexpr void
    generic_range_printer<R, Element_Pnp, Sequence_Pnp_Defaults>::run_printer(
        const std::same_as<R> auto& object_arg,
        text_sink&     sink_obj_arg
    ) const
{
    const auto print_element = [&](auto&& element_arg)
    {
        if constexpr (std::same_as<
                          std::remove_cvref_t<decltype(element_arg)>,
                          value_type>)
        {
            _options.element.run_printer(element_arg, sink_obj_arg);
        }
        else
        {
            // A proxy or prvalue: converted, so the element pnp's exact-type
            // guard is met.
            const value_type value(
                std::forward<decltype(element_arg)>(element_arg)
            );
            _options.element.run_printer(value, sink_obj_arg);
        }
    };

    if constexpr (std::ranges::input_range<const R>)
    {
        PNP_INTERNAL_NAMESPACE::print_sequence(
            _options, object_arg, print_element, sink_obj_arg
        );
    }
    else
    {
        // begin() isn't const (filter_view caches its first element), so print
        // a copy.
        R copy = object_arg;
        PNP_INTERNAL_NAMESPACE::print_sequence(
            _options, copy, print_element, sink_obj_arg
        );
    }
}

PNP_SUPPORT_NS_END
template <typename R>
inline constexpr bool enable_default_range_printer = false;

template <typename R>
requires enable_default_range_printer<R>
         && PNP_SUPPORT_NAMESPACE::printable_range_c<R>
         && default_printable_c<std::ranges::range_value_t<R>>
struct default_printer<R>
    : public PNP_SUPPORT_NAMESPACE::generic_range_printer<
          R,
          default_printer<std::ranges::range_value_t<R>>>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_range_printer<
        R,
        default_printer<std::ranges::range_value_t<R>>>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END