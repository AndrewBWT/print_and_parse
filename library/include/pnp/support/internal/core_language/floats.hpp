#pragma once
#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unicode_bridge.hpp>
#include <utility>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

PNP_INTERNAL_NS_BEGIN

// Has to be defined before its used in the concept.
constexpr bool
    valid_float_id(
        const std::u8string_view id_arg
    ) noexcept
{
    if (id_arg.empty())
    {
        return false;
    }
    const char8_t first_char = id_arg.front();
    // Ensure its not a digit, a decimal or a plus or minus. Parsing will fail
    // otherwise.
    return not (
        (first_char >= u8'0' && first_char <= u8'9') || first_char == u8'.'
        || first_char == u8'+' || first_char == u8'-'
    );
}

constexpr bool
    compatible_float_ids(
        const std::u8string_view nan_id_arg,
        const std::u8string_view inf_id_arg
    ) noexcept
{
    return not nan_id_arg.starts_with(inf_id_arg)
           && not inf_id_arg.starts_with(nan_id_arg);
}

constexpr bool
    supported_float_format(
        const std::chars_format format_arg
    ) noexcept
{
    return format_arg == std::chars_format::scientific
           || format_arg == std::chars_format::fixed
           || format_arg == std::chars_format::general
           || format_arg == std::chars_format::hex;
}

constexpr bool
    valid_hex_float_prefix(
        const std::u8string_view prefix_arg,
        const std::u8string_view nan_id_arg,
        const std::u8string_view inf_id_arg
    ) noexcept
{
    if (prefix_arg.empty() || prefix_arg.front() == u8'+'
        || prefix_arg.front() == u8'-')
    {
        return false;
    }
    return not prefix_arg.starts_with(nan_id_arg)
           && not nan_id_arg.starts_with(prefix_arg)
           && not prefix_arg.starts_with(inf_id_arg)
           && not inf_id_arg.starts_with(prefix_arg);
}

PNP_INTERNAL_NS_END

struct float_pnp_default_base
{
    static constexpr std::u8string_view nan_id                        = u8"NaN";
    static constexpr std::u8string_view inf_id                        = u8"inf";
    static constexpr std::u8string_view hex_prefix                    = u8"0x";
    static constexpr bool               use_plus_for_positive_numbers = false;
};

struct float_pnp_scientific : float_pnp_default_base
{
    static constexpr std::chars_format chars_format
        = std::chars_format::scientific;
};

struct float_pnp_general : float_pnp_default_base
{
    static constexpr std::chars_format chars_format
        = std::chars_format::general;
};

struct float_pnp_fixed : float_pnp_default_base
{
    static constexpr std::chars_format chars_format = std::chars_format::fixed;
};

struct float_pnp_hex : float_pnp_default_base
{
    static constexpr std::chars_format chars_format = std::chars_format::hex;
};

template <typename Float_Pnp_Defaults>
struct float_pnp_options
{
    std::chars_format format = Float_Pnp_Defaults::chars_format;
    std::u8string     nan_id = std::u8string(Float_Pnp_Defaults::nan_id);
    std::u8string     inf_id = std::u8string(Float_Pnp_Defaults::inf_id);
    std::u8string hex_prefix = std::u8string(Float_Pnp_Defaults::hex_prefix);
    bool          use_plus_for_positive_numbers
        = Float_Pnp_Defaults::use_plus_for_positive_numbers;

    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;
};

template <typename Float_Pnp_Defaults>
concept valid_float_pnp_c
    = valid_default_options_c<float_pnp_options<Float_Pnp_Defaults>>;

template <typename T, typename Float_Pnp_Defaults = float_pnp_general>
requires std::floating_point<T> && valid_float_pnp_c<Float_Pnp_Defaults>
struct generic_float_pnp
    : public pnp_configuration<float_pnp_options<Float_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<float_pnp_options<Float_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;
    constexpr void
        run_printer(
            const std::same_as<T> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            T&             object_answer,
            parser_run_key run_key_arg
        ) const;
private:
    static constexpr std::size_t max_chars
        = 4 + std::numeric_limits<T>::max_exponent10
          - std::numeric_limits<T>::min_exponent10
          + std::numeric_limits<T>::max_digits10;

    constexpr basic_parser_return_value
        parse_number(
            parser_input&     parser_input_arg,
            const input_text& text_arg,
            const std::size_t sign_chars_arg,
            const std::size_t prefix_chars_arg,
            const bool        negative_arg,
            T&                object_answer
        ) const;

    // Reads one of the ids, after the sign. Reads only as far as each check
    // needs.
    constexpr basic_parser_return_value
        parse_special_value(
            parser_input&     parser_input_arg,
            const std::size_t sign_chars_arg,
            const bool        negative_arg,
            T&                object_answer
        ) const;
};

template <typename T>
inline constexpr bool enable_default_float_pnp = false;
PNP_SUPPORT_NS_END

template <typename T>
requires PNP_SUPPORT_NAMESPACE::enable_default_float_pnp<T>
struct default_printer<T> : public PNP_SUPPORT_NAMESPACE::generic_float_pnp<T>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_float_pnp<T>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T>
requires PNP_SUPPORT_NAMESPACE::enable_default_float_pnp<T>
struct default_parser<T> : public PNP_SUPPORT_NAMESPACE::generic_float_pnp<T>
{
    using base_t = PNP_SUPPORT_NAMESPACE::generic_float_pnp<T>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END

// Definitions.
PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename Float_Pnp_Defaults>
constexpr const char*
    float_pnp_options<Float_Pnp_Defaults>::first_error() const noexcept
{
    using namespace PNP_INTERNAL_NAMESPACE;
    if (not supported_float_format(format))
    {
        return "float_pnp_options format must be scientific, fixed, general or "
               "hex";
    }
    if (not valid_utf8(nan_id))
    {
        return "float_pnp_options nan id must be valid UTF-8";
    }
    if (not valid_utf8(inf_id))
    {
        return "float_pnp_options inf id must be valid UTF-8";
    }
    if (not no_surrounding_whitespace(nan_id))
    {
        return "float_pnp_options nan id must not start or end with whitespace";
    }
    if (not no_surrounding_whitespace(inf_id))
    {
        return "float_pnp_options inf id must not start or end with whitespace";
    }
    if (not valid_float_id(nan_id))
    {
        return "float_pnp_options nan id must be non-empty, and must not start "
               "with a digit, '.', '+' or '-'";
    }
    if (not valid_float_id(inf_id))
    {
        return "float_pnp_options inf id must be non-empty, and must not start "
               "with a digit, '.', '+' or '-'";
    }
    if (not compatible_float_ids(nan_id, inf_id))
    {
        return "float_pnp_options nan and inf ids must not be prefixes of each "
               "other";
    }
    // The prefix is only used, and so only checked, in hex mode. Switching a
    // configuration to hex means constructing a new pnp, which checks it then.
    if (format == std::chars_format::hex)
    {
        if (not valid_utf8(hex_prefix))
        {
            return "float_pnp_options hex prefix must be valid UTF-8";
        }
        if (not no_surrounding_whitespace(hex_prefix))
        {
            return "float_pnp_options hex prefix must not start or end with "
                   "whitespace";
        }
        if (not valid_hex_float_prefix(hex_prefix, nan_id, inf_id))
        {
            return "float_pnp_options hex prefix must be non-empty, must not "
                   "start with '+' or '-', and must not "
                   "overlap either id";
        }
    }
    return nullptr;
}

template <typename T, typename Float_Pnp_Defaults>
requires std::floating_point<T> && valid_float_pnp_c<Float_Pnp_Defaults>
constexpr void
    generic_float_pnp<T, Float_Pnp_Defaults>::run_printer(
        const std::same_as<T> auto& object_arg,
        text_sink&                  sink_obj_arg
    ) const
{
    if (std::isnan(object_arg))
    {
        sink_obj_arg.write(std::u8string_view(_options.nan_id));
        return;
    }
    if (std::signbit(object_arg))
    {
        sink_obj_arg.put(u8'-');
    }
    else if (_options.use_plus_for_positive_numbers)
    {
        sink_obj_arg.put(u8'+');
    }
    if (std::isinf(object_arg))
    {
        sink_obj_arg.write(std::u8string_view(_options.inf_id));
        return;
    }
    if (_options.format == std::chars_format::hex)
    {
        sink_obj_arg.write(_options.hex_prefix);
    }
    char        buffer[max_chars];
    const char* ptr_to_end = std::to_chars(
                                 buffer,
                                 buffer + sizeof(buffer),
                                 std::abs(object_arg),
                                 _options.format
    )
                                 .ptr;
    sink_obj_arg.write(unicode_bridge::cast_string_view<char8_t>(
        std::string_view(buffer, ptr_to_end)
    ));
}

template <typename T, typename Float_Pnp_Defaults>
requires std::floating_point<T> && valid_float_pnp_c<Float_Pnp_Defaults>
constexpr basic_parser_return_value
    generic_float_pnp<T, Float_Pnp_Defaults>::run_parser(
        parser_input&  parser_input_arg,
        T&             object_answer,
        parser_run_key run_key_arg
    ) const
{
    using namespace std;

    // The sign, if any. starts_with stops at the first byte that differs, so
    // this reads at most the first character.
    const bool   negative = parser_input_arg.starts_with(u8"-");
    const size_t sign_chars
        = (negative || parser_input_arg.starts_with(u8"+")) ? 1 : 0;

    // The first character after the sign decides most cases on its own. It's
    // read whole, even if it's split across pieces, and nothing after it is
    // read: a value typed at the end of a line doesn't wait for the next.
    const auto head_opt = parser_input_arg.peek_chars(sign_chars + 1);
    if (not head_opt.has_value())
    {
        return parser_input_arg.fail(u8"Expected a number, but the input ended."
        );
    }
    const auto&         head     = head_opt.value();
    const u8string_view sign     = head.get().substr(0, sign_chars);
    // The first code unit of that character: enough to compare against digits,
    // '.', and the start of the prefix, all of which are ASCII.
    const char8_t first_char     = head.get()[sign_chars];
    const bool    first_is_digit = first_char >= u8'0' && first_char <= u8'9';

    // The whole number: the sign and prefix (whatever they contain), then every
    // character that could be part of a number, however it's split across
    // pieces.
    auto number_text             = [&](const size_t lead_code_units_arg)
    {
        size_t seen = 0;
        return parser_input_arg.peek_while(
            [&seen, lead_code_units_arg](const char8_t code_unit_arg)
            {
                return seen++ < lead_code_units_arg
                       || PNP_INTERNAL_NAMESPACE::is_number_code_unit(
                           code_unit_arg
                       );
            }
        );
    };

    if (_options.format == chars_format::hex)
    {
        auto is_hex_float_start = [](const char8_t char_arg) noexcept -> bool
        {
            return PNP_INTERNAL_NAMESPACE::is_base_digit(char_arg, 16)
                   || char_arg == u8'.';
        };

        // Hex numbers always start with the prefix, which no id can start
        // with. starts_with stops at the first byte that differs, so text that
        // isn't the prefix costs nothing more to rule out.
        if (parser_input_arg.starts_with({sign, _options.hex_prefix}))
        {
            const size_t lead_code_units
                = sign_chars + _options.hex_prefix.size();
            // The character after the prefix, read whole: whether it can start
            // a hexadecimal float.
            const auto through_next = parser_input_arg.peek_chars(
                sign_chars
                + UNICODE_BRIDGE_NAMESPACE::count_chars(_options.hex_prefix) + 1
            );
            if (not through_next.has_value()
                || not is_hex_float_start((through_next.value().get()
                )[lead_code_units]))
            {
                return parser_input_arg.fail(
                    u8"Expected hexadecimal digits after \""
                    + _options.hex_prefix + u8"\", such as \""
                    + _options.hex_prefix + u8"1.8p+1\"."
                );
            }
            return parse_number(
                parser_input_arg,
                number_text(lead_code_units),
                sign_chars,
                _options.hex_prefix.size(),
                negative,
                object_answer
            );
        }
        if (first_is_digit || first_char == u8'.')
        {
            return parser_input_arg.fail(
                u8"Expected a hexadecimal number starting with \""
                + _options.hex_prefix + u8"\", such as \"" + _options.hex_prefix
                + u8"1.8p+1\"."
            );
        }
        return parse_special_value(
            parser_input_arg, sign_chars, negative, object_answer
        );
    }
    // Ids can't start with a digit or '.', so ordinary numbers never consult
    // them.
    if (first_is_digit || first_char == u8'.')
    {
        return parse_number(
            parser_input_arg,
            number_text(sign_chars),
            sign_chars,
            0,
            negative,
            object_answer
        );
    }
    return parse_special_value(
        parser_input_arg, sign_chars, negative, object_answer
    );
}

template <typename T, typename Float_Pnp_Defaults>
requires std::floating_point<T> && valid_float_pnp_c<Float_Pnp_Defaults>
constexpr basic_parser_return_value
    generic_float_pnp<T, Float_Pnp_Defaults>::parse_number(
        parser_input&     parser_input_arg,
        const input_text& text_arg,
        const std::size_t sign_chars_arg,
        const std::size_t prefix_chars_arg,
        const bool        negative_arg,
        T&                object_answer
    ) const
{
    using namespace std;
    const size_t      lead_chars = sign_chars_arg + prefix_chars_arg;
    const string_view narrow     = unicode_bridge::cast_string_view<char>(
        text_arg.get().substr(lead_chars)
    );
    T parsed{};
    const auto [ptr, ec] = from_chars(
        narrow.data(), narrow.data() + narrow.size(), parsed, _options.format
    );
    const size_t consumed = static_cast<size_t>(ptr - narrow.data());

    if (ec == errc::invalid_argument)
    {
        if (_options.format == chars_format::scientific)
        {
            return parser_input_arg.fail(
                u8"Expected a number in scientific notation, such as "
                u8"\"1.5e+00\". "
                u8"Scientific mode requires an exponent."
            );
        }
        else if (_options.format == chars_format::hex)
        {
            return parser_input_arg.fail(
                u8"Expected hexadecimal digits after \"" + _options.hex_prefix
                + u8"\", such as \"" + _options.hex_prefix + u8"1.8p+1\"."
            );
        }
        else
        {
            return parser_input_arg.fail(
                u8"Expected a number, but no valid digits were found."
            );
        }
    }
    if (ec == errc::result_out_of_range)
    {
        u8string msg = u8"The value \"";
        msg.append(text_arg.get().substr(0, lead_chars + consumed));
        msg.append(u8"\" is out of range for a floating-point type of ");
        msg.append(PNP_INTERNAL_NAMESPACE::size_to_u8string(sizeof(T)));
        msg.append(u8" bytes.");
        return parser_input_arg.fail(std::move(msg));
    }

    object_answer = negative_arg ? -parsed : parsed;
    parser_input_arg.advance(lead_chars + consumed);
    return {};
}

template <typename T, typename Float_Pnp_Defaults>
requires std::floating_point<T> && valid_float_pnp_c<Float_Pnp_Defaults>
constexpr basic_parser_return_value
    generic_float_pnp<T, Float_Pnp_Defaults>::parse_special_value(
        parser_input&     parser_input_arg,
        const std::size_t sign_chars_arg,
        const bool        negative_arg,
        T&                object_answer
    ) const
{
    using namespace std;

    // Failures are reported at the start of the value, sign included.
    const size_t start = parser_input_arg.position();

    // The sign has already been read, so moving past it never asks for more.
    // Then each id is checked with advance_if_starts_with, which stops reading
    // as soon as the input stops matching. The ids are validated never to be
    // prefixes of each other, so at most one can match, and the order of the
    // checks doesn't matter.
    parser_input_arg.advance(sign_chars_arg);

    if (parser_input_arg.advance_if_starts_with(_options.inf_id))
    {
        constexpr T infinity = numeric_limits<T>::infinity();
        object_answer        = negative_arg ? -infinity : infinity;
        return {};
    }

    if (parser_input_arg.advance_if_starts_with(_options.nan_id))
    {
        if (sign_chars_arg != 0)
        {
            return parser_input_arg.fail(
                u8"\"" + _options.nan_id + u8"\" cannot have a sign.", start
            );
        }
        object_answer = numeric_limits<T>::quiet_NaN();
        return {};
    }

    // Neither a number nor a recognised id: report what was found, reading
    // exactly one whole character (its first code unit says how long it is; an
    // invalid first byte gives 0, which reads nothing and is reported as
    // invalid UTF-8). Neither a number nor a recognised id: report what was
    // found, reading exactly one whole character, even if it's split across
    // pieces. Invalid UTF-8 there throws invalid_utf8, as everywhere else.
    u8string msg = u8"Expected a number, \"" + _options.nan_id + u8"\" or \""
                   + _options.inf_id + u8"\", but found ";
    const auto found = parser_input_arg.peek_chars(1);
    if (not found.has_value())
    {
        msg.append(u8"the end of the input.");
    }
    else
    {
        msg.append(u8"\"");
        msg.append(unicode_bridge::to_formatted_unicode_string<char8_t>(
            found.value().get()
        ));
        msg.append(u8"\".");
    }
    return parser_input_arg.fail(std::move(msg), start);
}

PNP_SUPPORT_NS_END
PNP_NS_END