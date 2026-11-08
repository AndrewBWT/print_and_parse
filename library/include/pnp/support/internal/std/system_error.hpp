#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <concepts>
#include <string>
#include <string_view>
#include <system_error>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename E>
concept std_error_code_like_c
    = std::same_as<E, std::error_code> || std::same_as<E, std::error_condition>;

// The category's name, a colon, and the value: system:2. Optionally followed by
// the message.
struct error_code_printer_default
{
    static constexpr std::u8string_view separator    = u8":";
    static constexpr bool               show_message = false;
};

// show_message  follow the value with a space and the category's message for it
template <typename Error_Code_Printer_Defaults>
struct error_code_printer_options
{
    std::u8string separator
        = std::u8string(Error_Code_Printer_Defaults::separator);
    bool show_message = Error_Code_Printer_Defaults::show_message;

    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        using namespace PNP_INTERNAL_NAMESPACE;
        if (not valid_utf8(separator))
        {
            return "error_code_printer_options separator must be valid UTF-8";
        }
        if (not no_surrounding_whitespace(separator))
        {
            return "error_code_printer_options separator must not start or end "
                   "with whitespace";
        }
        return nullptr;
    }
};

// Prints a std::error_code or std::error_condition as its category's name and
// its value. Print-only: categories are singletons, found by identity, so a
// name can't be turned back into one.
template <
    typename E,
    typename Error_Code_Printer_Defaults = error_code_printer_default>
requires std_error_code_like_c<E>
         && valid_default_options_c<
             error_code_printer_options<Error_Code_Printer_Defaults>>
struct generic_error_code_printer
    : public pnp_configuration<
          error_code_printer_options<Error_Code_Printer_Defaults>>
{
public:
    using base_t = pnp_configuration<
        error_code_printer_options<Error_Code_Printer_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    void
        run_printer(
            const std::same_as<E> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const
    {
        sink_obj_arg.write(UNICODE_BRIDGE_NAMESPACE::cast_string_view<char8_t>(
            std::string_view(object_arg.category().name())
        ));
        sink_obj_arg.write(std::u8string_view(_options.separator));
        sink_obj_arg.write(std::u8string_view(
            PNP_INTERNAL_NAMESPACE::int_to_u8string(object_arg.value())
        ));
        if (_options.show_message)
        {
            const std::string message = object_arg.message();
            sink_obj_arg.write(std::u8string_view(u8" "));
            sink_obj_arg.write(
                UNICODE_BRIDGE_NAMESPACE::cast_string_view<char8_t>(
                    std::string_view(message)
                )
            );
        }
    }
};

PNP_SUPPORT_NS_END
PNP_NS_END