#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"

#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// One part: the pnp for the object referred to.
template <typename Referent_Pnp>
struct reference_wrapper_printer_options
{
    using referent_part_type    = Referent_Pnp;

    referent_part_type referent = {};

    // No rules of its own yet: rules for any field added here go in this
    // function.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        return nullptr;
    }

    // A reference prints exactly as what it refers to.
    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept
    {
        return fixed_output_width_of(referent);
    }
};

// Prints a std::reference_wrapper as the object it refers to, with nothing
// around it: a reference has no state of its own to show. Print-only: parsing
// has no object to refer to.
template <typename R, typename Referent_Pnp>
requires printer_c<Referent_Pnp, std::remove_cv_t<typename R::type>>
struct generic_reference_wrapper_printer
    : public pnp_configuration<reference_wrapper_printer_options<Referent_Pnp>>
{
public:
    using base_t
        = pnp_configuration<reference_wrapper_printer_options<Referent_Pnp>>;
    using base_t::_options;
    using base_t::base_t;

    using referent_part_type = Referent_Pnp;

    constexpr void
        run_printer(
            const std::same_as<R> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const
    {
        _options.referent.run_printer(object_arg.get(), sink_obj_arg);
    }
};

PNP_SUPPORT_NS_END
PNP_NS_END