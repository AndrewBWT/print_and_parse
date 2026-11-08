#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/token_or_value.hpp"
#include "pnp/support/internal/utility.hpp"

#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unicode_bridge.hpp>
#include <utility>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// std::nullopt prints as "nullopt".
struct nullopt_pnp_default
{
    static constexpr std::u8string_view nullopt_id = u8"nullopt";
};

// The text std::nullopt prints as: non-empty, valid UTF-8, with no whitespace
// at either end.
template <typename Nullopt_Pnp_Defaults>
struct nullopt_pnp_options
{
    std::u8string nullopt_id = std::u8string(Nullopt_Pnp_Defaults::nullopt_id);

    // The first rule these options break, or nullptr if they're valid.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;

    // Always the token, so always its length.
    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept;
};

template <typename Nullopt_Pnp_Defaults>
concept valid_nullopt_pnp_c
    = valid_default_options_c<nullopt_pnp_options<Nullopt_Pnp_Defaults>>;

// Prints and parses std::nullopt as a single token. Also the token part of the
// optional pnp.
template <typename Nullopt_Pnp_Defaults = nullopt_pnp_default>
requires valid_nullopt_pnp_c<Nullopt_Pnp_Defaults>
struct nullopt_pnp
    : public pnp_configuration<nullopt_pnp_options<Nullopt_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<nullopt_pnp_options<Nullopt_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;
    constexpr void
        run_printer(
            const std::same_as<std::nullopt_t> auto& object_arg,
            text_sink&                               sink_obj_arg
        ) const;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&   parser_input_arg,
            std::nullopt_t& object_answer,
            parser_run_key  run_key_arg
        ) const;

    // The text this pnp always prints, and the only text it parses.
    [[nodiscard]]
    constexpr std::u8string_view
        token() const noexcept;
};

// The type an optional's value is printed and parsed as.
template <typename O>
using optional_value_t = std::remove_cv_t<typename O::value_type>;

// Empty optionals print as "nullopt"; others as their value in braces: {5}. The
// braces keep an optional holding something that can itself print "nullopt" (a
// nested optional, say) distinguishable from an empty one: nullopt, {nullopt},
// {{5}}.
struct optional_pnp_default
{
    using empty_defaults                                = nullopt_pnp_default;
    static constexpr std::u8string_view open_delimiter  = u8"{";
    static constexpr std::u8string_view close_delimiter = u8"}";
};

// The empty part, the value part, and the delimiters around the value. The
// parts validated themselves when they were built; first_error() checks the
// delimiters, and that the opening delimiter and the empty token can't be
// confused.
//
// The parser decides between an empty optional and a value by which one the
// input starts with: the empty token or the opening delimiter. So the two must
// not be prefixes of each other.
//
// Empty delimiters are allowed, for output like 5 rather than {5}, but then the
// parser can't tell an empty optional from a value whose own text starts with
// the empty token: a std::optional<std::optional<int>> holding an empty
// optional would read back as empty. Use them only when the value can never
// print that way.
template <typename Value_Pnp, typename Optional_Pnp_Defaults>
struct optional_pnp_options
{
    using empty_part_type
        = nullopt_pnp<typename Optional_Pnp_Defaults::empty_defaults>;
    using value_part_type = Value_Pnp;

    empty_part_type empty = {};
    std::u8string   open_delimiter
        = std::u8string(Optional_Pnp_Defaults::open_delimiter);
    value_part_type value = {};
    std::u8string   close_delimiter
        = std::u8string(Optional_Pnp_Defaults::close_delimiter);

    // The first rule these options break, or nullptr if they're valid.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;

    // This configuration's text, for the shared token-or-value functions.
    [[nodiscard]]
    constexpr PNP_INTERNAL_NAMESPACE::token_or_value_text
        text() const noexcept;
};

// The policy is valid for this value: its default options pass first_error().
template <typename Value_Pnp, typename Optional_Pnp_Defaults>
concept valid_optional_pnp_c = valid_default_options_c<
    optional_pnp_options<Value_Pnp, Optional_Pnp_Defaults>>;

// Prints and parses a std::optional (or anything with has_value(), * and
// reset()): its empty token, or its value between the delimiters.
//
//   pnp::generic_optional_pnp<std::optional<int>, pnp::default_printer<int>> p;
//   // {5}
template <typename O, typename Value_Pnp, typename Optional_Pnp_Defaults = optional_pnp_default>
    requires valid_optional_pnp_c<Value_Pnp, Optional_Pnp_Defaults>
&& (printer_c<Value_Pnp, optional_value_t<O>> || parser_c<Value_Pnp, optional_value_t<O>>)
struct generic_optional_pnp : public pnp_configuration<optional_pnp_options<Value_Pnp, Optional_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<
        optional_pnp_options<Value_Pnp, Optional_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    using empty_part_type = typename base_t::options_type::empty_part_type;
    using value_part_type = Value_Pnp;

    constexpr void
        run_printer(
            const std::same_as<O> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const
    requires printer_c<Value_Pnp, optional_value_t<O>>;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            O&             object_answer,
            parser_run_key run_key_arg
        ) const
    requires parser_c<Value_Pnp, optional_value_t<O>>
             && parse_constructible_c<optional_value_t<O>>;
};

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename Nullopt_Pnp_Defaults>
constexpr const char*
    nullopt_pnp_options<Nullopt_Pnp_Defaults>::first_error() const noexcept
{
    using namespace PNP_INTERNAL_NAMESPACE;
    if (nullopt_id.empty())
    {
        return "nullopt_pnp_options nullopt id must be non-empty";
    }
    if (not valid_utf8(nullopt_id))
    {
        return "nullopt_pnp_options nullopt id must be valid UTF-8";
    }
    if (not no_surrounding_whitespace(nullopt_id))
    {
        return "nullopt_pnp_options nullopt id must not start or end with "
               "whitespace";
    }
    return nullptr;
}

template <typename Nullopt_Pnp_Defaults>
constexpr std::optional<std::size_t>
    nullopt_pnp_options<Nullopt_Pnp_Defaults>::fixed_output_width(
    ) const noexcept
{
    return nullopt_id.size();
}

template <typename Nullopt_Pnp_Defaults>
requires valid_nullopt_pnp_c<Nullopt_Pnp_Defaults>
constexpr void
    nullopt_pnp<Nullopt_Pnp_Defaults>::run_printer(
        const std::same_as<std::nullopt_t> auto&,
        text_sink& sink_obj_arg
    ) const
{
    sink_obj_arg.write(token());
}

template <typename Nullopt_Pnp_Defaults>
requires valid_nullopt_pnp_c<Nullopt_Pnp_Defaults>
constexpr basic_parser_return_value
    nullopt_pnp<Nullopt_Pnp_Defaults>::run_parser(
        parser_input& parser_input_arg,
        std::nullopt_t&,
        parser_run_key run_key_arg
    ) const
{
    if (not parser_input_arg.advance_if_starts_with(_options.nullopt_id))
    {
        return parser_input_arg.fail(
            u8"Expected " + PNP_INTERNAL_NAMESPACE::quoted(_options.nullopt_id)
            + u8"."
        );
    }
    return {};
}

template <typename Nullopt_Pnp_Defaults>
requires valid_nullopt_pnp_c<Nullopt_Pnp_Defaults>
constexpr std::u8string_view
    nullopt_pnp<Nullopt_Pnp_Defaults>::token() const noexcept
{
    return _options.nullopt_id;
}

// ------------------------------------------------------------ options

template <typename Value_Pnp, typename Optional_Pnp_Defaults>
constexpr const char*
    optional_pnp_options<Value_Pnp, Optional_Pnp_Defaults>::first_error(
    ) const noexcept
{
    using namespace PNP_INTERNAL_NAMESPACE;
    for (const std::u8string_view token :
         {std::u8string_view(open_delimiter),
          std::u8string_view(close_delimiter)})
    {
        if (not valid_utf8(token))
        {
            return "optional_pnp_options delimiters must be valid UTF-8";
        }
        if (not no_surrounding_whitespace(token))
        {
            return "optional_pnp_options delimiters must not start or end with "
                   "whitespace";
        }
    }

    // A rule across the empty part and the optional's own field: the parser
    // decides which it's reading by which one the input starts with.
    if (not token_and_delimiter_distinguishable(empty.token(), open_delimiter))
    {
        return "optional_pnp_options open delimiter and empty token must not "
               "be prefixes of each other";
    }
    return nullptr;
}

template <typename Value_Pnp, typename Optional_Pnp_Defaults>
constexpr PNP_INTERNAL_NAMESPACE::token_or_value_text
    optional_pnp_options<Value_Pnp, Optional_Pnp_Defaults>::text(
    ) const noexcept
{
    return {
        .token             = empty.token(),
        .token_description = u8"empty token",
        .open_delimiter    = open_delimiter,
        .close_delimiter   = close_delimiter,
        .value_description = u8"the value of an optional",
    };
}

// ------------------------------------------------------------ the optional pnp

template <typename O, typename Value_Pnp, typename Optional_Pnp_Defaults>
    requires valid_optional_pnp_c<Value_Pnp, Optional_Pnp_Defaults>
&& (printer_c<Value_Pnp, optional_value_t<O>> || parser_c<Value_Pnp, optional_value_t<O>>)
constexpr void
generic_optional_pnp<O, Value_Pnp, Optional_Pnp_Defaults>::run_printer(
    const std::same_as<O> auto& object_arg,
    text_sink& sink_obj_arg
) const requires printer_c<Value_Pnp, optional_value_t<O>>
{
    PNP_INTERNAL_NAMESPACE::print_token_or_value(
        _options.text(),
        object_arg.has_value(),
        [&]
        {
            _options.value.run_printer(*object_arg, sink_obj_arg);
        },
        sink_obj_arg
    );
}

template <typename O, typename Value_Pnp, typename Optional_Pnp_Defaults>
    requires valid_optional_pnp_c<Value_Pnp, Optional_Pnp_Defaults>
&& (printer_c<Value_Pnp, optional_value_t<O>> || parser_c<Value_Pnp, optional_value_t<O>>)
constexpr basic_parser_return_value
generic_optional_pnp<O, Value_Pnp, Optional_Pnp_Defaults>::run_parser(
    parser_input& parser_input_arg,
    O& object_answer,
    parser_run_key run_key_arg
) const
    requires parser_c<Value_Pnp, optional_value_t<O>>&& parse_constructible_c<optional_value_t<O>>
{
    // The value is parsed into a temporary, so a failed parse leaves the
    // optional untouched.
    PNP_INTERNAL_NAMESPACE::parse_target<optional_value_t<O>> target;
    bool                                                      is_empty = false;
    if (auto result = PNP_INTERNAL_NAMESPACE::parse_token_or_value(
            _options.text(),
            parser_input_arg,
            is_empty,
            [&](parser_input& input_arg)
            {
                return input_arg.run_parser(_options.value, target.value);
            }
        );
        result.failed())
    {
        return result;
    }

    if (is_empty)
    {
        object_answer.reset();
    }
    else
    {
        object_answer.emplace(std::move(target.value));
    }
    return {};
}

PNP_SUPPORT_NS_END
PNP_NS_END