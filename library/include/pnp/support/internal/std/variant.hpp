#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/tuple_like.hpp"
#include "pnp/support/internal/utility.hpp"

#include <charconv>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <unicode_bridge.hpp>
#include <utility>
#include <variant>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// ============================================================ std::monostate

// std::monostate prints as "monostate".
struct monostate_pnp_default
{
    static constexpr std::u8string_view monostate_id = u8"monostate";
};

template <typename Monostate_Pnp_Defaults>
struct monostate_pnp_options
{
    std::u8string monostate_id
        = std::u8string(Monostate_Pnp_Defaults::monostate_id);

    // The first rule these options break, or nullptr if they're valid.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;

    // Always the token, so always its length.
    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept
    {
        return monostate_id.size();
    }
};

template <typename Monostate_Pnp_Defaults>
concept valid_monostate_pnp_c
    = valid_default_options_c<monostate_pnp_options<Monostate_Pnp_Defaults>>;

// Prints and parses std::monostate as a single token.
template <typename Monostate_Pnp_Defaults = monostate_pnp_default>
requires valid_monostate_pnp_c<Monostate_Pnp_Defaults>
struct monostate_pnp
    : public pnp_configuration<monostate_pnp_options<Monostate_Pnp_Defaults>>
{
public:
    using base_t
        = pnp_configuration<monostate_pnp_options<Monostate_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;
    constexpr void
        run_printer(
            const std::same_as<std::monostate> auto& object_arg,
            text_sink&                               sink_obj_arg
        ) const;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&   parser_input_arg,
            std::monostate& object_answer,
            parser_run_key  run_key_arg
        ) const;

    // The text this pnp always prints, and the only text it parses.
    [[nodiscard]]
    constexpr std::u8string_view
        token() const noexcept
    {
        return _options.monostate_id;
    }
};

// ============================================================ the variant pnp

// The type alternative I of a variant is printed and parsed as.
template <typename V, std::size_t I>
using variant_alternative_value_t
    = std::remove_cv_t<std::variant_alternative_t<I, V>>;

PNP_INTERNAL_NS_BEGIN
template <typename V, typename Parts, std::size_t... Is>
constexpr bool
variant_parts_print(std::index_sequence<Is...>) noexcept
{
    return (
        printer_c<
            std::tuple_element_t<Is, Parts>,
            variant_alternative_value_t<V, Is>>
        && ...
    );
}

template <typename V, typename Parts, std::size_t... Is>
constexpr bool
variant_parts_parse(std::index_sequence<Is...>) noexcept
{
    return (
        (parser_c<
             std::tuple_element_t<Is, Parts>,
             variant_alternative_value_t<V, Is>>
         && parse_constructible_c<variant_alternative_value_t<V, Is>>)
        && ...
    );
}

PNP_INTERNAL_NS_END

// Parts is a std::tuple with one pnp per alternative, in order.
template <typename V, typename Parts>
concept variant_parts_c = PNP_INTERNAL_NAMESPACE::is_std_tuple_v<Parts>
                          && std::tuple_size_v<Parts> == std::variant_size_v<V>;

// Every part can print its alternative.
template <typename V, typename Parts>
concept variant_parts_print_c = variant_parts_c<V, Parts> && requires {
    requires PNP_INTERNAL_NAMESPACE::variant_parts_print<V, Parts>(
        std::make_index_sequence<std::variant_size_v<V>>{}
    );
};

// Every part can parse its alternative, and every alternative can be parsed
// into.
template <typename V, typename Parts>
concept variant_parts_parse_c = variant_parts_c<V, Parts> && requires {
    requires PNP_INTERNAL_NAMESPACE::variant_parts_parse<V, Parts>(
        std::make_index_sequence<std::variant_size_v<V>>{}
    );
};

// An alternative prints as its index, a colon, then its own text: 0:1, 1:{5}. A
// variant left valueless by an exception prints as "valueless".
struct variant_pnp_default
{
    static constexpr std::u8string_view index_separator = u8":";
    static constexpr std::u8string_view valueless_id    = u8"valueless";
};

// The separator after the index, one part per alternative, and the token for a
// valueless variant. The parts validated themselves when they were built;
// first_error() checks the variant's own text.
//
// The index is read as decimal digits, so neither the separator nor the
// valueless token may start with one: the index must end exactly where the
// separator begins, and the valueless token must never look like an index.
template <typename Parts, typename Variant_Pnp_Defaults>
struct variant_pnp_options
{
    using parts_type = Parts;

    std::u8string index_separator
        = std::u8string(Variant_Pnp_Defaults::index_separator);
    parts_type    alternatives = {};
    std::u8string valueless_id
        = std::u8string(Variant_Pnp_Defaults::valueless_id);

    // The first rule these options break, or nullptr if they're valid.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;
};

template <typename Parts, typename Variant_Pnp_Defaults>
concept valid_variant_pnp_c
    = valid_default_options_c<variant_pnp_options<Parts, Variant_Pnp_Defaults>>;

// Prints and parses a std::variant as its alternative's index, a separator, and
// the alternative's own text. Alternatives are chosen by index, so repeated
// types keep theirs.
//
//   using int_or_text = std::variant<int, std::string>;
//   pnp::generic_variant_pnp<int_or_text, std::tuple<pnp::default_printer<int>,
//                                                    pnp::default_printer<std::string>>>
//                                                    p;   // 0:1
template <typename V, typename Parts, typename Variant_Pnp_Defaults = variant_pnp_default>
    requires variant_parts_c<V, Parts>&& valid_variant_pnp_c<Parts, Variant_Pnp_Defaults>
&& (variant_parts_print_c<V, Parts> || variant_parts_parse_c<V, Parts>)
struct generic_variant_pnp : public pnp_configuration<variant_pnp_options<Parts, Variant_Pnp_Defaults>>
{
public:
    using base_t
        = pnp_configuration<variant_pnp_options<Parts, Variant_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    // The pnp for alternative I, for configuring it.
    template <std::size_t I>
    using part_type = std::tuple_element_t<I, Parts>;
    constexpr void
        run_printer(
            const std::same_as<V> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const
    requires variant_parts_print_c<V, Parts>;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            V&             object_answer,
            parser_run_key run_key_arg
        ) const
    requires variant_parts_parse_c<V, Parts>;
private:
    static constexpr std::size_t alternative_count = std::variant_size_v<V>;

    template <std::size_t I>
    constexpr void
        print_alternative(
            const V&                object_arg,
            text_sink& sink_obj_arg
        ) const;

    template <std::size_t I>
    constexpr basic_parser_return_value
        parse_alternative(parser_input& parser_input_arg, V& object_answer)
            const;
};

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

PNP_INTERNAL_NS_BEGIN

// The rules every single-token type's text follows.
constexpr const char*
    token_error(
        const std::u8string_view token_arg,
        const char*              empty_arg,
        const char*              utf8_arg,
        const char*              whitespace_arg
    ) noexcept
{
    if (token_arg.empty())
    {
        return empty_arg;
    }
    if (not valid_utf8(token_arg))
    {
        return utf8_arg;
    }
    if (not no_surrounding_whitespace(token_arg))
    {
        return whitespace_arg;
    }
    return nullptr;
}

constexpr bool
    starts_with_digit(
        const std::u8string_view text_arg
    ) noexcept
{
    return not text_arg.empty() && text_arg.front() >= u8'0'
           && text_arg.front() <= u8'9';
}

PNP_INTERNAL_NS_END

// ------------------------------------------------------------ std::monostate

template <typename Monostate_Pnp_Defaults>
constexpr const char*
    monostate_pnp_options<Monostate_Pnp_Defaults>::first_error() const noexcept
{
    return PNP_INTERNAL_NAMESPACE::token_error(
        monostate_id,
        "monostate_pnp_options monostate id must be non-empty",
        "monostate_pnp_options monostate id must be valid UTF-8",
        "monostate_pnp_options monostate id must not start or end with "
        "whitespace"
    );
}

template <typename Monostate_Pnp_Defaults>
requires valid_monostate_pnp_c<Monostate_Pnp_Defaults>
constexpr void
    monostate_pnp<Monostate_Pnp_Defaults>::run_printer(
        const std::same_as<std::monostate> auto&,
        text_sink& sink_obj_arg
    ) const
{
    sink_obj_arg.write(token());
}

template <typename Monostate_Pnp_Defaults>
requires valid_monostate_pnp_c<Monostate_Pnp_Defaults>
constexpr basic_parser_return_value
    monostate_pnp<Monostate_Pnp_Defaults>::run_parser(
        parser_input& parser_input_arg,
        std::monostate&,
        parser_run_key run_key_arg
    ) const
{
    if (not parser_input_arg.advance_if_starts_with(_options.monostate_id))
    {
        return parser_input_arg.fail(
            u8"Expected "
            + PNP_INTERNAL_NAMESPACE::quoted(_options.monostate_id) + u8"."
        );
    }
    return {};
}

// ------------------------------------------------------------ options

template <typename Parts, typename Variant_Pnp_Defaults>
constexpr const char*
    variant_pnp_options<Parts, Variant_Pnp_Defaults>::first_error(
    ) const noexcept
{
    using namespace PNP_INTERNAL_NAMESPACE;
    if (const char* error = token_error(
            index_separator,
            "variant_pnp_options index separator must be non-empty",
            "variant_pnp_options index separator must be valid UTF-8",
            "variant_pnp_options index separator must not start or end with "
            "whitespace"
        );
        error != nullptr)
    {
        return error;
    }
    if (const char* error = token_error(
            valueless_id,
            "variant_pnp_options valueless id must be non-empty",
            "variant_pnp_options valueless id must be valid UTF-8",
            "variant_pnp_options valueless id must not start or end with "
            "whitespace"
        );
        error != nullptr)
    {
        return error;
    }
    // The index is read as digits, which must end where the separator begins,
    // and the valueless token must never look like an index.
    if (starts_with_digit(index_separator))
    {
        return "variant_pnp_options index separator must not start with a "
               "digit";
    }
    if (starts_with_digit(valueless_id))
    {
        return "variant_pnp_options valueless id must not start with a digit";
    }
    return nullptr;
}

// ------------------------------------------------------------ the variant pnp

template <typename V, typename Parts, typename Variant_Pnp_Defaults>
    requires variant_parts_c<V, Parts>&& valid_variant_pnp_c<Parts, Variant_Pnp_Defaults>
&& (variant_parts_print_c<V, Parts> || variant_parts_parse_c<V, Parts>)
constexpr void
generic_variant_pnp<V, Parts, Variant_Pnp_Defaults>::run_printer(
    const std::same_as<V> auto& object_arg,
    text_sink& sink_obj_arg
) const requires variant_parts_print_c<V, Parts>
{
    if (object_arg.valueless_by_exception())
    {
        sink_obj_arg.write(std::u8string_view(_options.valueless_id));
        return;
    }
    // The alternative is known only at run time; each index has its own part,
    // so dispatch to the one whose index matches.
    [&]<std::size_t... Is>(std::index_sequence<Is...>)
    {
        ( void ) ((object_arg.index() == Is
                       ? (print_alternative<Is>(object_arg, sink_obj_arg), true)
                       : false)
                  || ...);
    }(std::make_index_sequence<alternative_count>{});
}

template <typename V, typename Parts, typename Variant_Pnp_Defaults>
    requires variant_parts_c<V, Parts>&& valid_variant_pnp_c<Parts, Variant_Pnp_Defaults>
&& (variant_parts_print_c<V, Parts> || variant_parts_parse_c<V, Parts>)
template <std::size_t I>
constexpr void
generic_variant_pnp<V, Parts, Variant_Pnp_Defaults>::print_alternative(
    const V& object_arg,
    text_sink& sink_obj_arg
) const
{
    sink_obj_arg.write(
        std::u8string_view(PNP_INTERNAL_NAMESPACE::size_to_u8string(I))
    );
    sink_obj_arg.write(
        std::u8string_view(_options.index_separator)
    );
    std::get<I>(_options.alternatives)
        .run_printer(std::get<I>(object_arg), sink_obj_arg);
}

template <typename V, typename Parts, typename Variant_Pnp_Defaults>
    requires variant_parts_c<V, Parts>&& valid_variant_pnp_c<Parts, Variant_Pnp_Defaults>
&& (variant_parts_print_c<V, Parts> || variant_parts_parse_c<V, Parts>)
constexpr basic_parser_return_value
generic_variant_pnp<V, Parts, Variant_Pnp_Defaults>::run_parser(parser_input& parser_input_arg, V& object_answer,
    parser_run_key run_key_arg) const
    requires variant_parts_parse_c<V, Parts>
{
    // A valueless variant can't be built deliberately, so its token can't be
    // parsed.
    if (parser_input_arg.starts_with(_options.valueless_id))
    {
        return parser_input_arg.fail(
            u8"A valueless variant ("
            + PNP_INTERNAL_NAMESPACE::quoted(_options.valueless_id)
            + u8") can't be parsed: it only arises from an exception."
        );
    }

    // The index: decimal digits, ending where the separator begins.
    const auto remaining_input_text = parser_input_arg.peek_while(
        [](auto char_arg)
        {
            return PNP_INTERNAL_NAMESPACE::is_digit(char_arg);
        }
    );
    const auto  remaining = remaining_input_text.get();
    const char* digits
        = UNICODE_BRIDGE_NAMESPACE::cast_char_ptr<char>(remaining.data());
    std::size_t index = 0;
    const auto [digits_end, error_code]
        = std::from_chars(digits, digits + remaining.size(), index, 10);
    if (error_code == std::errc::invalid_argument)
    {
        return parser_input_arg.fail(
            u8"Expected the index of a variant's alternative."
        );
    }
    if (error_code == std::errc::result_out_of_range
        || index >= alternative_count)
    {
        return parser_input_arg.fail(
            u8"The index "
            + std::u8string(remaining.substr(
                0, static_cast<std::size_t>(digits_end - digits)
            ))
            + u8" is out of range for a variant of "
            + PNP_INTERNAL_NAMESPACE::size_to_u8string(alternative_count)
            + u8" alternatives."
        );
    }
    parser_input_arg.advance(static_cast<std::size_t>(digits_end - digits));

    if (not parser_input_arg.advance_if_starts_with(_options.index_separator))
    {
        return parser_input_arg.fail(
            u8"Expected the index separator "
            + PNP_INTERNAL_NAMESPACE::quoted(_options.index_separator)
            + u8" after the alternative's index."
        );
    }

    // The alternative with that index, parsed by its own part.
    basic_parser_return_value result;
    [&]<std::size_t... Is>(std::index_sequence<Is...>)
    {
        ( void ) ((index == Is ? (result = parse_alternative<Is>(
                                      parser_input_arg, object_answer
                                  ),
                                  true)
                               : false)
                  || ...);
    }(std::make_index_sequence<alternative_count>{});
    return result;
}

template <typename V, typename Parts, typename Variant_Pnp_Defaults>
    requires variant_parts_c<V, Parts>&& valid_variant_pnp_c<Parts, Variant_Pnp_Defaults>
&& (variant_parts_print_c<V, Parts> || variant_parts_parse_c<V, Parts>)
template <std::size_t I>
constexpr basic_parser_return_value
generic_variant_pnp<V, Parts, Variant_Pnp_Defaults>::parse_alternative(
    parser_input& parser_input_arg,
    V& object_answer
) const
{
    // Parsed into a temporary, so a failed parse leaves the variant untouched.
    PNP_INTERNAL_NAMESPACE::parse_target<variant_alternative_value_t<V, I>>
        target;
    if (auto result = parser_input_arg.run_parser(
            std::get<I>(_options.alternatives), target.value
        );
        result.failed())
    {
        result.add_msg(
            u8"while parsing alternative "
            + PNP_INTERNAL_NAMESPACE::size_to_u8string(I) + u8" of a variant"
        );
        return result;
    }
    object_answer.template emplace<I>(std::move(target.value));
    return {};
}

PNP_SUPPORT_NS_END
PNP_NS_END