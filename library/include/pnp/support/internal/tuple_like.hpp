#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unicode_bridge.hpp>
#include <utility>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// ============================================================ parts

PNP_INTERNAL_NS_BEGIN
template <typename T>
inline constexpr bool is_std_tuple_v = false;

template <typename... Ts>
inline constexpr bool is_std_tuple_v<std::tuple<Ts...>> = true;

// Whether every part can print / parse its element. Only instantiated once the
// sizes are known to match (the concepts below check that first).
template <typename Tuple, typename Parts>
inline constexpr bool tuple_parts_print
    = []<std::size_t... Indices>(std::index_sequence<Indices...>)
{
    return (
        printer_c<
            std::tuple_element_t<Indices, Parts>,
            std::tuple_element_t<Indices, Tuple>>
        && ...
    );
}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});

template <typename Tuple, typename Parts>
inline constexpr bool tuple_parts_parse
    = []<std::size_t... Indices>(std::index_sequence<Indices...>)
{
    return (
        parser_c<
            std::tuple_element_t<Indices, Parts>,
            std::tuple_element_t<Indices, Tuple>>
        && ...
    );
}(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
PNP_INTERNAL_NS_END

// A tuple-like type (std::tuple, std::pair, ...), with a std::tuple of one part
// per element.
template <typename Tuple, typename Parts>
concept tuple_parts_c = requires { std::tuple_size<Tuple>::value; }
                        && PNP_INTERNAL_NAMESPACE::is_std_tuple_v<Parts>
                        && std::tuple_size_v<Parts> == std::tuple_size_v<Tuple>;

template <typename Tuple, typename Parts>
concept tuple_parts_print_c
    = tuple_parts_c<Tuple, Parts>
      && PNP_INTERNAL_NAMESPACE::tuple_parts_print<Tuple, Parts>;

template <typename Tuple, typename Parts>
concept tuple_parts_parse_c
    = tuple_parts_c<Tuple, Parts>
      && PNP_INTERNAL_NAMESPACE::tuple_parts_parse<Tuple, Parts>;

// ============================================================ policies and
// options

PNP_INTERNAL_NS_BEGIN
// The first rule a tuple configuration breaks, or nullptr. Shared by the policy
// concept (which has no parts to build options with) and the options
// themselves.
constexpr const char*
    tuple_options_error(
        const std::u8string_view open_delimiter_arg,
        const std::u8string_view close_delimiter_arg,
        const std::u8string_view separator_arg,
        const std::u8string_view separator_spacing_arg,
        const std::u8string_view delimiter_spacing_arg
    ) noexcept;
PNP_INTERNAL_NS_END

// The defaults every tuple policy shares: compact output, strict parsing.
struct tuple_pnp_default_base
{
    static constexpr std::u8string_view separator_spacing = u8"";
    static constexpr std::u8string_view delimiter_spacing = u8"";
    static constexpr bool               skip_whitespace   = false;
};

// Tuples in parentheses, separated by commas: (1,2.5,"a").
struct tuple_pnp_default : tuple_pnp_default_base
{
    static constexpr std::u8string_view open_delimiter  = u8"(";
    static constexpr std::u8string_view close_delimiter = u8")";
    static constexpr std::u8string_view separator       = u8",";
};

template <typename Tuple_Pnp_Defaults>
concept valid_tuple_pnp_c = requires {
    requires std::
        convertible_to<decltype(Tuple_Pnp_Defaults::skip_whitespace), bool>;
    requires PNP_INTERNAL_NAMESPACE::tuple_options_error(
                 Tuple_Pnp_Defaults::open_delimiter,
                 Tuple_Pnp_Defaults::close_delimiter,
                 Tuple_Pnp_Defaults::separator,
                 Tuple_Pnp_Defaults::separator_spacing,
                 Tuple_Pnp_Defaults::delimiter_spacing
             ) == nullptr;
};

// The tuple's own configuration, plus one part per element. The parts validated
// themselves when they were built, so first_error() only checks the tuple's own
// fields.
//
// The delimiters may be empty (a map entry is a pair with none: 1:"a"); the
// separator may not. A tuple knows how many elements it has, so after each
// element the parser knows which token comes next, and the separator and close
// delimiter can't be confused.
//
// Spacing and skip_whitespace behave exactly as for sequences:
//   separator_spacing  printed after each separator:            (1, 2.5)
//   delimiter_spacing  printed inside both delimiters:          ( 1, 2.5 )
//   skip_whitespace    parsing accepts any whitespace instead of exactly the
//   spacing
template <typename Parts, typename Tuple_Pnp_Defaults>
requires PNP_INTERNAL_NAMESPACE::is_std_tuple_v<Parts>
         && valid_tuple_pnp_c<Tuple_Pnp_Defaults>
struct tuple_pnp_options
{
    using parts_type = Parts;

    std::u8string open_delimiter
        = std::u8string(Tuple_Pnp_Defaults::open_delimiter);
    parts_type    elements  = {};
    std::u8string separator = std::u8string(Tuple_Pnp_Defaults::separator);
    std::u8string close_delimiter
        = std::u8string(Tuple_Pnp_Defaults::close_delimiter);
    std::u8string separator_spacing
        = std::u8string(Tuple_Pnp_Defaults::separator_spacing);
    std::u8string delimiter_spacing
        = std::u8string(Tuple_Pnp_Defaults::delimiter_spacing);
    bool skip_whitespace = Tuple_Pnp_Defaults::skip_whitespace;

    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;
    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept;
};

// ============================================================ the pnp

// Prints and parses a tuple-like type, one part per element. run_printer exists
// when every part can print its element; run_parser when every part can parse
// its element.
//
//   using pair_pnp = pnp::generic_tuple_pnp<std::pair<int, double>,
//                                           std::tuple<pnp::default_printer<int>,
//                                           pnp::default_printer<double>>>;
//   pair_pnp p;                                                         //
//   (1,2.5) pair_pnp q({.separator_spacing = u8" "}); // (1, 2.5)
template <typename Tuple, typename Parts, typename Tuple_Pnp_Defaults = tuple_pnp_default>
    requires tuple_parts_c<Tuple, Parts>&& valid_tuple_pnp_c<Tuple_Pnp_Defaults>
&& (tuple_parts_print_c<Tuple, Parts> || tuple_parts_parse_c<Tuple, Parts>)
struct generic_tuple_pnp : public pnp_configuration<tuple_pnp_options<Parts, Tuple_Pnp_Defaults>>
{
public:
    using base_t
        = pnp_configuration<tuple_pnp_options<Parts, Tuple_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    using parts_type = Parts;

    template <std::size_t Index>
    using part_type = std::tuple_element_t<Index, Parts>;
    constexpr void
        run_printer(
            const std::same_as<Tuple> auto& object_arg,
            text_sink&                      sink_obj_arg
        ) const
    requires tuple_parts_print_c<Tuple, Parts>;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            Tuple&         object_answer,
            parser_run_key run_key_arg
        ) const
    requires tuple_parts_parse_c<Tuple, Parts> && parse_constructible_c<Tuple>;
private:
    static constexpr std::size_t element_count = std::tuple_size_v<Tuple>;

    // Parses element Index into result_arg, preceded by the separator if it
    // isn't the first.
    template <std::size_t Index>
    constexpr basic_parser_return_value
        parse_element(parser_input& parser_input_arg, Tuple& result_arg) const;

    // Whitespace between pieces: skipped when lenient, exactly the spacing when
    // strict.
    constexpr basic_parser_return_value
        consume_spacing(
            parser_input&        parser_input_arg,
            const std::u8string& spacing_arg,
            std::u8string_view   where_arg
        ) const;
};

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

PNP_INTERNAL_NS_BEGIN
constexpr const char*
    tuple_options_error(
        const std::u8string_view open_delimiter_arg,
        const std::u8string_view close_delimiter_arg,
        const std::u8string_view separator_arg,
        const std::u8string_view separator_spacing_arg,
        const std::u8string_view delimiter_spacing_arg
    ) noexcept
{
    for (const std::u8string_view token :
         {open_delimiter_arg, close_delimiter_arg, separator_arg})
    {
        if (not valid_utf8(token))
        {
            return "tuple_pnp_options delimiters and separator must be valid "
                   "UTF-8";
        }
        if (not no_surrounding_whitespace(token))
        {
            return "tuple_pnp_options delimiters and separator must not start "
                   "or end with whitespace";
        }
    }
    if (not only_whitespace(separator_spacing_arg))
    {
        return "tuple_pnp_options separator spacing must contain only "
               "whitespace";
    }
    if (not only_whitespace(delimiter_spacing_arg))
    {
        return "tuple_pnp_options delimiter spacing must contain only "
               "whitespace";
    }
    return nullptr;
}

PNP_INTERNAL_NS_END

// ------------------------------------------------------------ options
template <typename Parts, typename Tuple_Pnp_Defaults>
requires PNP_INTERNAL_NAMESPACE::is_std_tuple_v<Parts>
         && valid_tuple_pnp_c<Tuple_Pnp_Defaults>
constexpr const char*
    tuple_pnp_options<Parts, Tuple_Pnp_Defaults>::first_error() const noexcept
{
    if (const char* error = PNP_INTERNAL_NAMESPACE::tuple_options_error(
            open_delimiter,
            close_delimiter,
            separator,
            separator_spacing,
            delimiter_spacing
        );
        error != nullptr)
    {
        return error;
    }

    // A rule across the parts: with nothing between them, each part's length
    // must be fixed.
    const bool every_part_fixed = std::apply(
        [](const auto&... parts_arg)
        {
            return (fixed_output_width_of(parts_arg).has_value() && ...);
        },
        elements
    );
    if (separator.empty() && not every_part_fixed)
    {
        return "tuple_pnp_options separator may only be empty when every part "
               "prints with the same width";
    }
    return nullptr;
}

template <typename Parts, typename Tuple_Pnp_Defaults>
requires PNP_INTERNAL_NAMESPACE::is_std_tuple_v<Parts>
         && valid_tuple_pnp_c<Tuple_Pnp_Defaults>
constexpr std::optional<std::size_t>
    tuple_pnp_options<Parts, Tuple_Pnp_Defaults>::fixed_output_width(
    ) const noexcept
{
    // Lenient parsing consumes a varying amount of whitespace.
    if (skip_whitespace)
    {
        return std::nullopt;
    }

    constexpr std::size_t count = std::tuple_size_v<Parts>;
    std::size_t width = open_delimiter.size() + close_delimiter.size();
    if constexpr (count != 0)
    {
        bool        every_part_fixed = true;
        std::size_t parts_width      = 0;
        std::apply(
            [&](const auto&... parts_arg)
            {
                (
                    [&]
                    {
                        const std::optional<std::size_t> part_width
                            = fixed_output_width_of(parts_arg);
                        every_part_fixed
                            = every_part_fixed && part_width.has_value();
                        parts_width += part_width.value_or(0);
                    }(),
                    ...
                );
            },
            elements
        );
        if (not every_part_fixed)
        {
            return std::nullopt;
        }
        // Spacing only goes inside a non-empty tuple.
        width += 2 * delimiter_spacing.size() + parts_width
                 + (count - 1) * (separator.size() + separator_spacing.size());
    }
    return width;
}

// ------------------------------------------------------------ the pnp

template <typename Tuple, typename Parts, typename Tuple_Pnp_Defaults>
    requires tuple_parts_c<Tuple, Parts>&& valid_tuple_pnp_c<Tuple_Pnp_Defaults>
&& (tuple_parts_print_c<Tuple, Parts> || tuple_parts_parse_c<Tuple, Parts>)
constexpr void
generic_tuple_pnp<Tuple, Parts, Tuple_Pnp_Defaults>::run_printer(
    const std::same_as<Tuple> auto& object_arg,
    text_sink& sink_obj_arg
) const requires tuple_parts_print_c<Tuple, Parts>
{
    sink_obj_arg.write(std::u8string_view(_options.open_delimiter));
    if constexpr (element_count != 0)
    {
        // Spacing only goes inside a non-empty tuple: an empty one prints as
        // ().
        sink_obj_arg.write(std::u8string_view(_options.delimiter_spacing));
        [&]<std::size_t... Indices>(std::index_sequence<Indices...>)
        {
            (
                [&]
                {
                    if constexpr (Indices != 0)
                    {
                        sink_obj_arg.write(std::u8string_view(_options.separator
                        ));
                        sink_obj_arg.write(
                            std::u8string_view(_options.separator_spacing)
                        );
                    }
                    std::get<Indices>(_options.elements)
                        .run_printer(
                            std::get<Indices>(object_arg), sink_obj_arg
                        );
                }(),
                ...
            );
        }(std::make_index_sequence<element_count>{});
        sink_obj_arg.write(std::u8string_view(_options.delimiter_spacing));
    }
    sink_obj_arg.write(std::u8string_view(_options.close_delimiter));
}

template <typename Tuple, typename Parts, typename Tuple_Pnp_Defaults>
    requires tuple_parts_c<Tuple, Parts>&& valid_tuple_pnp_c<Tuple_Pnp_Defaults>
&& (tuple_parts_print_c<Tuple, Parts> || tuple_parts_parse_c<Tuple, Parts>)
constexpr basic_parser_return_value
generic_tuple_pnp<Tuple, Parts, Tuple_Pnp_Defaults>::consume_spacing(
    parser_input& parser_input_arg,
    const std::u8string& spacing_arg,
    const std::u8string_view where_arg
) const
{
    if (_options.skip_whitespace)
    {
        parser_input_arg.skip_whitespace();
        return {};
    }
    if (not parser_input_arg.advance_if_starts_with(spacing_arg))
    {
        return parser_input_arg.fail(
            u8"Expected the spacing "
            + PNP_INTERNAL_NAMESPACE::quoted(spacing_arg) + u8" "
            + std::u8string(where_arg) + u8"."
        );
    }
    return {};
}

template <typename Tuple, typename Parts, typename Tuple_Pnp_Defaults>
    requires tuple_parts_c<Tuple, Parts>&& valid_tuple_pnp_c<Tuple_Pnp_Defaults>
&& (tuple_parts_print_c<Tuple, Parts> || tuple_parts_parse_c<Tuple, Parts>)
template <std::size_t Index>
constexpr basic_parser_return_value
generic_tuple_pnp<Tuple, Parts, Tuple_Pnp_Defaults>::parse_element(
    parser_input& parser_input_arg,
    Tuple& result_arg
) const
{
    if constexpr (Index != 0)
    {
        if (_options.skip_whitespace)
        {
            parser_input_arg.skip_whitespace();
        }
        if (not parser_input_arg.advance_if_starts_with(_options.separator))
        {
            return parser_input_arg.fail(
                u8"Expected separator "
                + PNP_INTERNAL_NAMESPACE::quoted(_options.separator)
                + u8" after element "
                + PNP_INTERNAL_NAMESPACE::size_to_u8string(Index - 1)
                + u8" of a tuple."
            );
        }
        if (auto spacing_result = consume_spacing(
                parser_input_arg,
                _options.separator_spacing,
                u8"after the separator"
            );
            spacing_result.failed())
        {
            return spacing_result;
        }
    }

    auto element_result = parser_input_arg.run_parser(
        std::get<Index>(_options.elements), std::get<Index>(result_arg)
    );
    if (element_result.failed())
    {
        element_result.add_msg(
            u8"while parsing element "
            + PNP_INTERNAL_NAMESPACE::size_to_u8string(Index) + u8" of a tuple"
        );
    }
    return element_result;
}

template <typename Tuple, typename Parts, typename Tuple_Pnp_Defaults>
    requires tuple_parts_c<Tuple, Parts>&& valid_tuple_pnp_c<Tuple_Pnp_Defaults>
&& (tuple_parts_print_c<Tuple, Parts> || tuple_parts_parse_c<Tuple, Parts>)
constexpr basic_parser_return_value
generic_tuple_pnp<Tuple, Parts, Tuple_Pnp_Defaults>::run_parser(
    parser_input& parser_input_arg,
    Tuple& object_answer,
    parser_run_key run_key_arg
) const
    requires tuple_parts_parse_c<Tuple, Parts>&& parse_constructible_c<Tuple>
{
    if (not parser_input_arg.advance_if_starts_with(_options.open_delimiter))
    {
        return parser_input_arg.fail(
            u8"Expected opening delimiter "
            + PNP_INTERNAL_NAMESPACE::quoted(_options.open_delimiter) + u8"."
        );
    }

    // Parsed into a temporary, so a failed parse leaves the caller's object
    // untouched.
    PNP_INTERNAL_NAMESPACE::parse_target<Tuple> target;
    Tuple&                                      result = target.value;

    if constexpr (element_count == 0)
    {
        // An empty tuple: just the delimiters (with any whitespace between,
        // when lenient).
        if (_options.skip_whitespace)
        {
            parser_input_arg.skip_whitespace();
        }
    }
    else
    {
        if (auto spacing_result = consume_spacing(
                parser_input_arg,
                _options.delimiter_spacing,
                u8"after the opening delimiter"
            );
            spacing_result.failed())
        {
            return spacing_result;
        }

        // Each element in turn; && stops at the first failure, leaving it in
        // error.
        basic_parser_return_value error;
        [&]<std::size_t... Indices>(std::index_sequence<Indices...>)
        {
            static_cast<void>(
                ((error = parse_element<Indices>(parser_input_arg, result))
                     .succeeded()
                 && ...)
            );
        }(std::make_index_sequence<element_count>{});
        if (error.failed())
        {
            return error;
        }

        if (auto spacing_result = consume_spacing(
                parser_input_arg,
                _options.delimiter_spacing,
                u8"before the closing delimiter"
            );
            spacing_result.failed())
        {
            return spacing_result;
        }
    }

    if (not parser_input_arg.advance_if_starts_with(_options.close_delimiter))
    {
        return parser_input_arg.fail(
            u8"Expected closing delimiter "
            + PNP_INTERNAL_NAMESPACE::quoted(_options.close_delimiter)
            + u8" after the last element of a tuple."
        );
    }

    object_answer = std::move(result);
    return {};
}

PNP_SUPPORT_NS_END
PNP_NS_END