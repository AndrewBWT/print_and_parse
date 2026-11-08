#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/sequences.hpp"
#include "pnp/support/internal/utility.hpp"

#include <concepts>
#include <string>
#include <string_view>
#include <unicode_bridge.hpp>
#include <version>

#if defined(__cpp_lib_stacktrace)
    #include <stacktrace>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// No options yet: an entry prints as its description, then its source position
// if known.
struct stacktrace_entry_printer_options
{
    // No rules of its own yet: rules for any field added here go in this
    // function.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        return nullptr;
    }
};

// Prints a std::stacktrace_entry: its description, then " at file:line" when
// the source is known. Print-only.
struct stacktrace_entry_printer
    : public pnp_configuration<stacktrace_entry_printer_options>
{
public:
    using base_t = pnp_configuration<stacktrace_entry_printer_options>;
    using base_t::base_t;

    void
        run_printer(
            const std::same_as<std::stacktrace_entry> auto& object_arg,
            text_sink&                                      sink_obj_arg
        ) const
    {
        const std::string description = object_arg.description();
        sink_obj_arg.write(UNICODE_BRIDGE_NAMESPACE::cast_string_view<char8_t>(
            std::string_view(description)
        ));
        const std::string file = object_arg.source_file();
        if (not file.empty())
        {
            sink_obj_arg.write(std::u8string_view(u8" at "));
            sink_obj_arg.write(
                UNICODE_BRIDGE_NAMESPACE::cast_string_view<char8_t>(
                    std::string_view(file)
                )
            );
            sink_obj_arg.write(std::u8string_view(u8":"));
            sink_obj_arg.write(std::u8string_view(
                PNP_INTERNAL_NAMESPACE::size_to_u8string(object_arg.source_line(
                ))
            ));
        }
    }
};

// A stack trace prints one entry per line, inside brackets:
//   [
//   main at main.cpp:12,
//   ...
//   ]
struct stacktrace_pnp_default : sequence_pnp_default_base
{
    static constexpr std::u8string_view open_delimiter    = u8"[";
    static constexpr std::u8string_view close_delimiter   = u8"]";
    static constexpr std::u8string_view separator         = u8",";
    static constexpr std::u8string_view separator_spacing = u8"";
    static constexpr std::u8string_view delimiter_spacing = u8"";
};

// A stack trace is a sequence of entries. Print-only: it has no appender.
template <
    typename S,
    typename Entry_Pnp               = stacktrace_entry_printer,
    typename Stacktrace_Pnp_Defaults = stacktrace_pnp_default>
using generic_stacktrace_printer
    = generic_sequence_pnp<S, Entry_Pnp, Stacktrace_Pnp_Defaults>;

PNP_SUPPORT_NS_END
PNP_NS_END

#endif