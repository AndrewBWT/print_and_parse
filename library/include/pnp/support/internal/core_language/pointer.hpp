#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/core_language/nullptr.hpp"
#include "pnp/support/internal/fixed_width_pnp.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/std/string.hpp"
#include "pnp/support/internal/std/string_view.hpp"

#include "pnp/support/internal/token_or_value.hpp"

#include <concepts>
#include <string_view>
#include <type_traits>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN
PNP_INTERNAL_NS_BEGIN

// A pointer is its null token, or its value written straight after nothing: the
// value's own form (an address's 0x, a string's quote) is what tells them
// apart. Pointers are print-only, so this is never parsed; configure the value
// so it can't start like the null token anyway.
inline constexpr token_or_value_text
    pointer_text(
        const std::u8string_view token_arg,
        const std::u8string_view value_description_arg
    ) noexcept
{
    return {
        .token             = token_arg,
        .token_description = u8"null token",
        .open_delimiter    = u8"",
        .close_delimiter   = u8"",
        .value_description = value_description_arg,
    };
}

PNP_INTERNAL_NS_END

struct pointer_address_default : fixed_width_pnp_unsigned_hex
{
    static constexpr bool fixed_size_output = true;
};

struct pointer_printer_default
{
    using null_defaults    = nullptr_pnp_default;
    using address_defaults = pointer_address_default;
};

template <typename Pointer_Printer_Defaults>
concept valid_pointer_printer_c
    = valid_nullptr_pnp_c<typename Pointer_Printer_Defaults::null_defaults>
      && valid_fixed_width_pnp_c<
          typename Pointer_Printer_Defaults::address_defaults>;

template <typename Pointer, typename Pointer_Printer_Defaults>
requires std::is_pointer_v<Pointer>
         && valid_pointer_printer_c<Pointer_Printer_Defaults>
struct pointer_printer_options
{
    using null_part_type
        = nullptr_pnp<typename Pointer_Printer_Defaults::null_defaults>;
    using address_part_type = fixed_width_pnp<
        Pointer,
        typename Pointer_Printer_Defaults::address_defaults>;

    null_part_type    null    = {};
    address_part_type address = {};

    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        return nullptr;
    }
};

template <
    typename Pointer,
    typename Pointer_Printer_Defaults = pointer_printer_default>
requires std::is_pointer_v<Pointer>
         && valid_pointer_printer_c<Pointer_Printer_Defaults>
struct generic_pointer_printer
    : public pnp_configuration<
          pointer_printer_options<Pointer, Pointer_Printer_Defaults>>
{
public:
    using base_t = pnp_configuration<
        pointer_printer_options<Pointer, Pointer_Printer_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    using null_part_type    = typename base_t::options_type::null_part_type;
    using address_part_type = typename base_t::options_type::address_part_type;
    constexpr void
        run_printer(
            const std::same_as<Pointer> auto& object_arg,
            text_sink&           sink_obj_arg
        ) const;
};

template <typename Pointer>
concept c_string_pointer_c
    = std::is_pointer_v<Pointer>
      && unicode_bridge::is_char_type_c<
          std::remove_cv_t<std::remove_pointer_t<Pointer>>>
      && (not std::is_volatile_v<std::remove_pointer_t<Pointer>>);

struct c_string_pointer_printer_default
{
    using null_defaults = nullptr_pnp_default;
    using text_defaults = string_pnp_default;
};

template <typename C_String_Pointer_Printer_Defaults>
concept valid_c_string_pointer_printer_c
    = valid_nullptr_pnp_c<
          typename C_String_Pointer_Printer_Defaults::null_defaults>
      && valid_string_pnp_c<
          typename C_String_Pointer_Printer_Defaults::text_defaults>;

template <typename Pointer, typename C_String_Pointer_Printer_Defaults>
requires c_string_pointer_c<Pointer>
         && valid_c_string_pointer_printer_c<C_String_Pointer_Printer_Defaults>
struct c_string_pointer_printer_options
{
    using char_type      = std::remove_cv_t<std::remove_pointer_t<Pointer>>;
    using view_type      = std::basic_string_view<char_type>;
    using null_part_type = nullptr_pnp<
        typename C_String_Pointer_Printer_Defaults::null_defaults>;
    using text_part_type = generic_string_view_printer<
        view_type,
        typename C_String_Pointer_Printer_Defaults::text_defaults>;

    null_part_type null = {};
    text_part_type text = {};

    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        return nullptr;
    }
};

template <
    typename Pointer,
    typename C_String_Pointer_Printer_Defaults
    = c_string_pointer_printer_default>
requires c_string_pointer_c<Pointer>
         && valid_c_string_pointer_printer_c<C_String_Pointer_Printer_Defaults>
struct generic_c_string_pointer_printer
    : public pnp_configuration<c_string_pointer_printer_options<
          Pointer,
          C_String_Pointer_Printer_Defaults>>
{
public:
    using base_t = pnp_configuration<c_string_pointer_printer_options<
        Pointer,
        C_String_Pointer_Printer_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    using null_part_type = typename base_t::options_type::null_part_type;
    using text_part_type = typename base_t::options_type::text_part_type;
    constexpr void
        run_printer(
            const std::same_as<Pointer> auto& object_arg,
            text_sink&           sink_obj_arg
        ) const;
private:
    using view_type = typename base_t::options_type::view_type;
};

PNP_SUPPORT_NS_END
PNP_NS_END

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename Pointer, typename Pointer_Printer_Defaults>
requires std::is_pointer_v<Pointer>
         && valid_pointer_printer_c<Pointer_Printer_Defaults>
constexpr void
    generic_pointer_printer<Pointer, Pointer_Printer_Defaults>::run_printer(
        const std::same_as<Pointer> auto& object_arg,
        text_sink&           sink_obj_arg
    ) const
{
    PNP_INTERNAL_NAMESPACE::print_token_or_value(
        PNP_INTERNAL_NAMESPACE::pointer_text(
            _options.null.token(), u8"the address of a pointer"
        ),
        object_arg != nullptr,
        [&]
        {
            _options.address.run_printer(object_arg, sink_obj_arg);
        },
        sink_obj_arg
    );
}

template <typename Pointer, typename C_String_Pointer_Printer_Defaults>
requires c_string_pointer_c<Pointer>
         && valid_c_string_pointer_printer_c<C_String_Pointer_Printer_Defaults>
constexpr void
    generic_c_string_pointer_printer<
        Pointer,
        C_String_Pointer_Printer_Defaults>::
        run_printer(
            const std::same_as<Pointer> auto& object_arg,
            text_sink&           sink_obj_arg
        ) const
{
                 PNP_INTERNAL_NAMESPACE::print_token_or_value(
                     PNP_INTERNAL_NAMESPACE::pointer_text(_options.null.token(), u8"the text of a C string"),
                     object_arg != nullptr,
                     [&] { _options.text.run_printer(view_type(object_arg), sink_obj_arg); },
                     sink_obj_arg);
}

PNP_SUPPORT_NS_END
PNP_NS_END