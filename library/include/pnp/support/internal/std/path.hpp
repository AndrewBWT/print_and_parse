#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/std/string.hpp"
#include "pnp/support/internal/utility.hpp"

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <unicode_bridge.hpp>
#include <utility>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// The text part a path prints and parses through: a string, quoted by default.
using path_text_pnp = generic_string_pnp<std::u8string, string_pnp_default>;

// A path is written as a string. By default it uses the generic format, with
// forward slashes on every platform, so a Windows path needs no escaped
// backslashes: "C:/docs/report.txt".
struct path_pnp_default
{
    static constexpr bool generic_format = true;
};

// generic_format  print with forward slashes (true), or in the platform's
// native format
template <typename Text_Pnp, typename Path_Pnp_Defaults>
struct path_pnp_options
{
    using text_part_type          = Text_Pnp;

    text_part_type text           = {};
    bool           generic_format = Path_Pnp_Defaults::generic_format;

    // No rules of its own yet: rules for any field added here go in this
    // function.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        return nullptr;
    }
};

template <typename Text_Pnp, typename Path_Pnp_Defaults>
concept valid_path_pnp_c
    = valid_default_options_c<path_pnp_options<Text_Pnp, Path_Pnp_Defaults>>;

// Prints and parses a std::filesystem::path as a string: "docs/report.txt".
template <typename Text_Pnp = path_text_pnp, typename Path_Pnp_Defaults = path_pnp_default>
    requires valid_path_pnp_c<Text_Pnp, Path_Pnp_Defaults>
&& (printer_c<Text_Pnp, std::u8string> || parser_c<Text_Pnp, std::u8string>)
struct generic_path_pnp : public pnp_configuration<path_pnp_options<Text_Pnp, Path_Pnp_Defaults>>
{
public:
    using base_t
        = pnp_configuration<path_pnp_options<Text_Pnp, Path_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    using text_part_type = Text_Pnp;
    void
        run_printer(
            const std::same_as<std::filesystem::path> auto& object_arg,
            text_sink&                                      sink_obj_arg
        ) const
    requires printer_c<Text_Pnp, std::u8string>;

    basic_parser_return_value
        run_parser(
            parser_input&          parser_input_arg,
            std::filesystem::path& object_answer,
            parser_run_key         run_key_arg
        ) const
    requires parser_c<Text_Pnp, std::u8string>;
};

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename Text_Pnp, typename Path_Pnp_Defaults>
    requires valid_path_pnp_c<Text_Pnp, Path_Pnp_Defaults>
&& (printer_c<Text_Pnp, std::u8string> || parser_c<Text_Pnp, std::u8string>)
void
generic_path_pnp<Text_Pnp, Path_Pnp_Defaults>::run_printer(
    const std::same_as<std::filesystem::path> auto& object_arg,
    text_sink& sink_obj_arg
) const requires printer_c<Text_Pnp, std::u8string>
{
    const std::u8string text = _options.generic_format
                                   ? object_arg.generic_u8string()
                                   : object_arg.u8string();
    _options.text.run_printer(text, sink_obj_arg);
}

template <typename Text_Pnp, typename Path_Pnp_Defaults>
    requires valid_path_pnp_c<Text_Pnp, Path_Pnp_Defaults>
&& (printer_c<Text_Pnp, std::u8string> || parser_c<Text_Pnp, std::u8string>)
basic_parser_return_value
generic_path_pnp<Text_Pnp, Path_Pnp_Defaults>::run_parser(
    parser_input& parser_input_arg,
    std::filesystem::path& object_answer,
    parser_run_key run_key_arg
) const
    requires parser_c<Text_Pnp, std::u8string>
{
    // Parsed as text first, so a failed parse leaves the path untouched. Either
    // format parses: a path accepts forward slashes on every platform.
    std::u8string text;
    if (auto result = parser_input_arg.run_parser(_options.text, text);
        result.failed())
    {
        result.add_msg(u8"while parsing a path");
        return result;
    }
    object_answer = std::filesystem::path(std::move(text));
    return {};
}

PNP_SUPPORT_NS_END
PNP_NS_END