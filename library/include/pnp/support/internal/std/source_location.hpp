#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <unicode_bridge.hpp>

#include <concepts>
#include <source_location>
#include <string>
#include <string_view>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// file:line:column, as compilers report positions: main.cpp:12:5.
struct source_location_printer_default
{
    static constexpr std::u8string_view separator = u8":";
    static constexpr bool               show_column = true;
    static constexpr bool               file_name_only = false;
    static constexpr bool               show_function = false;
};

//   show_column     follow the line with the column
//   file_name_only  drop the file's directories, leaving just its name
//   show_function   follow the position with " in " and the function's name
template <typename Source_Location_Printer_Defaults>
struct source_location_printer_options
{
    std::u8string separator = std::u8string(Source_Location_Printer_Defaults::separator);
    bool          show_column = Source_Location_Printer_Defaults::show_column;
    bool          file_name_only = Source_Location_Printer_Defaults::file_name_only;
    bool          show_function = Source_Location_Printer_Defaults::show_function;

    [[nodiscard]] constexpr const char*
        first_error() const noexcept
    {
        using namespace PNP_INTERNAL_NAMESPACE;
        if (separator.empty())
        {
            return "source_location_printer_options separator must be non-empty";
        }
        if (not valid_utf8(separator))
        {
            return "source_location_printer_options separator must be valid UTF-8";
        }
        if (not no_surrounding_whitespace(separator))
        {
            return "source_location_printer_options separator must not start or end with whitespace";
        }
        return nullptr;
    }
};

// Prints a std::source_location. Print-only: one can't be constructed with chosen values.
template <typename Source_Location_Printer_Defaults = source_location_printer_default>
    requires valid_default_options_c<source_location_printer_options<Source_Location_Printer_Defaults>>
struct source_location_printer : public pnp_configuration<source_location_printer_options<Source_Location_Printer_Defaults>>
{
public:
    using base_t = pnp_configuration<source_location_printer_options<Source_Location_Printer_Defaults>>;
    using base_t::_options;
    using base_t::base_t;
    void
        run_printer(const std::same_as<std::source_location> auto& object_arg, text_sink& sink_obj_arg) const
    {
        std::string_view file = object_arg.file_name();
        if (_options.file_name_only)
        {
            if (const auto slash = file.find_last_of("/\\"); slash != std::string_view::npos)
            {
                file.remove_prefix(slash + 1);
            }
        }
        sink_obj_arg.write(UNICODE_BRIDGE_NAMESPACE::cast_string_view<char8_t>(file));
        sink_obj_arg.write(std::u8string_view(_options.separator));
        sink_obj_arg.write(std::u8string_view(PNP_INTERNAL_NAMESPACE::size_to_u8string(object_arg.line())));
        if (_options.show_column)
        {
            sink_obj_arg.write(std::u8string_view(_options.separator));
            sink_obj_arg.write(std::u8string_view(PNP_INTERNAL_NAMESPACE::size_to_u8string(object_arg.column())));
        }
        if (_options.show_function)
        {
            sink_obj_arg.write(std::u8string_view(u8" in "));
            sink_obj_arg.write(
                UNICODE_BRIDGE_NAMESPACE::cast_string_view<char8_t>(std::string_view(object_arg.function_name())));
        }
    }
};

PNP_SUPPORT_NS_END
PNP_NS_END