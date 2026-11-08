#pragma once

#include "pnp/core.hpp"

#include <concepts>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN
template <typename Options>
concept checkable_options_c = requires (const Options& options_arg) {
    { options_arg.first_error() } noexcept -> std::convertible_to<const char*>;
};

// Default-constructed options are valid. For options without first_error(),
// every configuration is valid, so this only requires default construction.
template <typename Options>
concept valid_default_options_c = std::default_initializable<Options>
                                  && checkable_options_c<Options> && requires {
                                         requires Options {}
                                         .first_error() == nullptr;
                                     };

template <typename Options>
concept options_with_fixed_output_width_c
    = requires (const Options& options_arg) {
          {
              options_arg.fixed_output_width()
          } -> std::same_as<std::optional<std::size_t>>;
      };

// Storage, construction and read access for a pnp configured by an options
// struct.
//
// A pnp derives from this, inherits its constructors and reads _options:
//
//   struct my_pnp : public pnp_configuration<my_pnp_options>
//   {
//       using base_t = pnp_configuration<my_pnp_options>;
//       using base_t::base_t;
//       using base_t::_options;
//       ...
//   };
//
// The default constructor uses the options' default member initialisers, which
// come from a policy already checked at compile time, so it does no runtime
// validation. The other constructors validate the options they are given, so
// every pnp that exists holds a valid configuration, and the options can't be
// changed afterwards.
template <typename Options>
requires valid_default_options_c<Options> && std::copy_constructible<Options>
         && std::move_constructible<Options>
struct pnp_configuration
{
public:
    using options_type            = Options;

    constexpr pnp_configuration() = default;

    // Throw std::invalid_argument if the options are invalid.
    constexpr explicit pnp_configuration(const options_type& options_arg);
    constexpr explicit pnp_configuration(options_type&& options_arg);
    constexpr pnp_configuration(const pnp_configuration&) = default;
    constexpr pnp_configuration&
        operator=(const pnp_configuration&)
        = default;

    // A move leaves the source with the default options, which are always
    // valid, rather than moved-from strings that never went through
    // first_error().
    constexpr pnp_configuration(pnp_configuration&& other_arg) noexcept(
        std::is_nothrow_move_constructible_v<options_type>&& std::is_nothrow_default_constructible_v<options_type>
        && std::is_nothrow_move_assignable_v<options_type>)
        : _options(std::move(other_arg._options))
    {
        other_arg._options = options_type{};
    }

    constexpr pnp_configuration&
        operator=(pnp_configuration&& other_arg) noexcept(
            std::is_nothrow_move_assignable_v<options_type>&& std::is_nothrow_default_constructible_v<options_type>)
    {
        if (this != &other_arg)
        {
            _options           = std::move(other_arg._options);
            other_arg._options = options_type{};
        }
        return *this;
    }

    [[nodiscard]]
    constexpr const options_type&
        options() const noexcept;
    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept;
protected:
    options_type _options{};
private:
    constexpr void
        validate_options() const;
};

// A pnp that can say whether its output always has the same width. Such a pnp
// prints exactly that many code units every time, and its parser never consumes
// more than that, so its values can be written back to back.
template <typename P>
concept fixed_output_width_c = requires (const P& pnp_arg) {
    {
        pnp_arg.fixed_output_width()
    } -> std::same_as<std::optional<std::size_t>>;
};

// The pnp's fixed output width, or std::nullopt if it hasn't got one, or can't
// say.
template <typename P>
[[nodiscard]]
constexpr std::optional<std::size_t>
    fixed_output_width_of(
        const P& pnp_arg
    ) noexcept
{
    if constexpr (fixed_output_width_c<P>)
    {
        return pnp_arg.fixed_output_width();
    }
    else
    {
        return std::nullopt;
    }
}

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename Options>
requires valid_default_options_c<Options> && std::copy_constructible<Options>
         && std::move_constructible<Options>
constexpr pnp_configuration<Options>::pnp_configuration(
    const options_type& options_arg
)
    : _options(options_arg)
{
    validate_options();
}

template <typename Options>
requires valid_default_options_c<Options> && std::copy_constructible<Options>
         && std::move_constructible<Options>
constexpr pnp_configuration<Options>::pnp_configuration(
    options_type&& options_arg
)
    : _options(std::move(options_arg))
{
    validate_options();
}

template <typename Options>
requires valid_default_options_c<Options> && std::copy_constructible<Options>
         && std::move_constructible<Options>
constexpr const typename pnp_configuration<Options>::options_type&
    pnp_configuration<Options>::options() const noexcept
{
    return _options;
}

template <typename Options>
requires valid_default_options_c<Options> && std::copy_constructible<Options>
         && std::move_constructible<Options>
constexpr std::optional<std::size_t>
    pnp_configuration<Options>::fixed_output_width() const noexcept
{
    if constexpr (options_with_fixed_output_width_c<Options>)
    {
        return _options.fixed_output_width();
    }
    else
    {
        return std::nullopt;
    }
}

template <typename Options>
requires valid_default_options_c<Options> && std::copy_constructible<Options>
         && std::move_constructible<Options>
constexpr void
    pnp_configuration<Options>::validate_options() const
{
    if constexpr (checkable_options_c<Options>)
    {
        if (const char* error = _options.first_error(); error != nullptr)
        {
            throw std::invalid_argument(error);
        }
    }
}

PNP_SUPPORT_NS_END
PNP_NS_END