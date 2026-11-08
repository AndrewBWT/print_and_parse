#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/sequences.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <unicode_bridge.hpp>
#include <version>

#if defined(__cpp_lib_mdspan)
    #include <mdspan>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN
// declared before mdspan_slice:
template <typename MD, std::size_t Depth>
requires (Depth < MD::rank())
struct mdspan_slice;

PNP_INTERNAL_NS_BEGIN

// What iterating a slice yields: the next slice in, or at the last level, the
// element type. A specialisation rather than std::conditional_t, which would
// name the slice past the last level (and fail its constraint) even when it
// isn't chosen.
template <typename MD, std::size_t Depth>
struct mdspan_slice_value
{
    using type = mdspan_slice<MD, Depth + 1>;
};

template <typename MD, std::size_t Depth>
requires (Depth + 1 == MD::rank())
struct mdspan_slice_value<MD, Depth>
{
    using type = typename MD::value_type;
};

PNP_INTERNAL_NS_END

// ============================================================ slices

// An mdspan with its first Depth indices fixed: the whole mdspan at Depth 0, a
// row of a matrix at Depth 1, and so on. Iterating it fixes the next index,
// giving the slice one level deeper, or at the last level, the elements
// themselves. This makes each dimension an ordinary sequence, printed by the
// sequence pnp.
template <typename MD, std::size_t Depth>
requires (Depth < MD::rank())
struct mdspan_slice
{
    using index_type = typename MD::index_type;

    // What iterating yields: the next slice in, or at the last level, the
    // element type.
    using value_type =
        typename PNP_INTERNAL_NAMESPACE::mdspan_slice_value<MD, Depth>::type;

    struct iterator
    {
        const MD*                     md;
        std::array<index_type, Depth> fixed_indices;
        index_type                    index;

        constexpr decltype(auto)
            operator*() const;

        constexpr iterator&
            operator++() noexcept
        {
            ++index;
            return *this;
        }

        constexpr bool
            operator==(
                const iterator& other_arg
            ) const noexcept
        {
            return index == other_arg.index;
        }
    };

    const MD*                     md;
    std::array<index_type, Depth> fixed_indices;

    constexpr iterator
        begin() const noexcept
    {
        return {md, fixed_indices, index_type{0}};
    }

    constexpr iterator
        end() const noexcept
    {
        return {md, fixed_indices, md->extent(Depth)};
    }
};

// A static extent makes that level a fixed number of elements.
template <typename MD, std::size_t Depth>
requires (MD::static_extent(Depth) != std::dynamic_extent)
inline constexpr std::optional<std::size_t>
    sequence_fixed_count_v<mdspan_slice<MD, Depth>> = MD::static_extent(Depth);

// ============================================================ the pnp for each
// dimension

PNP_INTERNAL_NS_BEGIN

// The pnp for dimension Depth: a sequence pnp over that level's slices, whose
// element part is the next dimension's pnp; past the last dimension, the
// element pnp itself.
template <
    typename MD,
    std::size_t Depth,
    typename Element_Pnp,
    typename Sequence_Pnp_Defaults>
struct mdspan_dimension_pnp
{
    using type = generic_sequence_pnp<
        mdspan_slice<MD, Depth>,
        typename mdspan_dimension_pnp<
            MD,
            Depth + 1,
            Element_Pnp,
            Sequence_Pnp_Defaults>::type,
        Sequence_Pnp_Defaults>;
};

template <
    typename MD,
    std::size_t Depth,
    typename Element_Pnp,
    typename Sequence_Pnp_Defaults>
requires (Depth == MD::rank())
struct mdspan_dimension_pnp<MD, Depth, Element_Pnp, Sequence_Pnp_Defaults>
{
    using type = Element_Pnp;
};

PNP_INTERNAL_NS_END

// The pnp that prints dimension Depth of an MD, and everything inside it.
template <
    typename MD,
    std::size_t Depth,
    typename Element_Pnp,
    typename Sequence_Pnp_Defaults = sequence_pnp_default>
using mdspan_dimension_pnp_t = typename PNP_INTERNAL_NAMESPACE::
    mdspan_dimension_pnp<MD, Depth, Element_Pnp, Sequence_Pnp_Defaults>::type;

// ============================================================ the printer

// One part: the pnp for the outermost dimension, which holds the next one in as
// its element part, and so on down to the element pnp. Each dimension is
// configured separately through those nested parts. The parts validated
// themselves, so no first_error(); the width is the outermost dimension's.
template <typename Dimensions_Pnp>
struct mdspan_printer_options
{
    using dimensions_part_type      = Dimensions_Pnp;

    dimensions_part_type dimensions = {};

    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept
    {
        return fixed_output_width_of(dimensions);
    }
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        return nullptr;
    }
};

// Prints an mdspan as nested sequences, one per dimension, in index order
// whatever the layout: a 2x3 mdspan prints as [[1,2,3],[4,5,6]]. Each dimension
// is a sequence pnp of its own, so each can be configured separately, and a
// dimension with a static extent has a fixed number of elements, so fixed-width
// elements can print flat. A rank-0 mdspan prints just its element. Print-only:
// an mdspan refers to storage elsewhere.
//
//   using grid      = std::mdspan<int, std::extents<std::size_t, 2, 3>>;
//   using printer   = pnp::generic_mdspan_printer<grid,
//   pnp::default_printer<int>>; using rows_pnp  =
//   printer::dimension_pnp_type<0>; using cells_pnp =
//   printer::dimension_pnp_type<1>; printer p({.dimensions = rows_pnp({.element
//   = cells_pnp({.separator_spacing = u8" "})})});   // [[1, 2, 3],[4, 5, 6]]
template <
    typename MD,
    typename Element_Pnp,
    typename Sequence_Pnp_Defaults = sequence_pnp_default>
requires printer_c<Element_Pnp, typename MD::value_type>
         && valid_sequence_pnp_c<Sequence_Pnp_Defaults>
struct generic_mdspan_printer
    : public pnp_configuration<mdspan_printer_options<
          mdspan_dimension_pnp_t<MD, 0, Element_Pnp, Sequence_Pnp_Defaults>>>
{
public:
    using base_t = pnp_configuration<mdspan_printer_options<
        mdspan_dimension_pnp_t<MD, 0, Element_Pnp, Sequence_Pnp_Defaults>>>;
    using base_t::_options;
    using base_t::base_t;

    // The pnp for dimension Depth, for configuring that dimension.
    template <std::size_t Depth>
    using dimension_pnp_type
        = mdspan_dimension_pnp_t<MD, Depth, Element_Pnp, Sequence_Pnp_Defaults>;
    constexpr void
        run_printer(
            const std::same_as<MD> auto& object_arg,
            text_sink&      sink_obj_arg
        ) const;
};

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename MD, std::size_t Depth>
requires (Depth < MD::rank())
constexpr decltype(auto)
    mdspan_slice<MD, Depth>::iterator::operator*() const
{
    if constexpr (Depth + 1 < MD::rank())
    {
        // The next slice in: these indices, plus this one.
        std::array<index_type, Depth + 1> next_indices{};
        for (std::size_t idx = 0; idx < Depth; ++idx)
        {
            next_indices[idx] = fixed_indices[idx];
        }
        next_indices[Depth] = index;
        return mdspan_slice<MD, Depth + 1>{md, next_indices};
    }
    else
    {
        // The element itself. Converted to the value type when the accessor
        // yields something else (a proxy, or a different reference type), so
        // the element pnp's exact-type guard is met; otherwise returned by
        // reference, with no copy.
        std::array<index_type, MD::rank()> all_indices{};
        for (std::size_t idx = 0; idx < Depth; ++idx)
        {
            all_indices[idx] = fixed_indices[idx];
        }
        all_indices[Depth] = index;

        using value_type   = typename MD::value_type;
        if constexpr (std::same_as<
                          std::remove_cvref_t<typename MD::reference>,
                          value_type>)
        {
            return static_cast<typename MD::reference>((*md)[all_indices]);
        }
        else
        {
            return value_type((*md)[all_indices]);
        }
    }
}

template <typename MD, typename Element_Pnp, typename Sequence_Pnp_Defaults>
requires printer_c<Element_Pnp, typename MD::value_type>
         && valid_sequence_pnp_c<Sequence_Pnp_Defaults>
constexpr void
    generic_mdspan_printer<MD, Element_Pnp, Sequence_Pnp_Defaults>::run_printer(
        const std::same_as<MD> auto& object_arg,
        text_sink&      sink_obj_arg
    ) const
{
    if constexpr (MD::rank() == 0)
    {
        // No dimensions: the "dimensions" part is the element pnp itself.
        if constexpr (std::same_as<
                          std::remove_cvref_t<typename MD::reference>,
                          typename MD::value_type>)
        {
            _options.dimensions.run_printer(
                object_arg[std::array<typename MD::index_type, 0>{}],
                sink_obj_arg
            );
        }
        else
        {
            const typename MD::value_type value(
                object_arg[std::array<typename MD::index_type, 0>{}]
            );
            _options.dimensions.run_printer(value, sink_obj_arg);
        }
    }
    else
    {
        _options.dimensions.run_printer(
            mdspan_slice<MD, 0>{&object_arg, {}}, sink_obj_arg
        );
    }
}

PNP_SUPPORT_NS_END
PNP_NS_END

#endif