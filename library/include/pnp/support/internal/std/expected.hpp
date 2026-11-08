#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/alternatives.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <unicode_bridge.hpp>
#include <utility>
#include <version>

#if defined(__cpp_lib_expected)
    #include <expected>

PNP_NS_BEGIN

// std::unexpected has no default constructor: parsing starts from its error
// type's seed.
template <typename E>
requires parse_constructible_c<E>
struct placeholder_value<std::unexpected<E>>
{
    static constexpr std::unexpected<E>
        value()
    {
        if constexpr (std::default_initializable<E>)
        {
            return std::unexpected<E>(E{});
        }
        else
        {
            return std::unexpected<E>(placeholder_value<E>::value());
        }
    }
};

PNP_SUPPORT_NS_BEGIN

// The types an expected's value and error are printed and parsed as. The value
// type may be void.
template <typename X>
using expected_value_t = std::remove_cv_t<typename X::value_type>;

template <typename X>
using expected_error_t = std::remove_cv_t<typename X::error_type>;

// The value part of an expected<void, E>: there's no value, so nothing to
// configure.
struct no_value_pnp
{};

PNP_INTERNAL_NS_BEGIN
// Whether a value part can print, or parse, an expected's value; for void, the
// placeholder can.
template <typename Value_Pnp, typename Value>
concept expected_value_printer_c
    = ( std::is_void_v<Value> && std::same_as<Value_Pnp, no_value_pnp> )
      || printer_c<Value_Pnp, Value>;

template <typename Value_Pnp, typename Value>
concept expected_value_parser_c
    = ( std::is_void_v<Value> && std::same_as<Value_Pnp, no_value_pnp> )
      || (parser_c<Value_Pnp, Value> && parse_constructible_c<Value>);

// What an expected's value is parsed into: a parse target, or nothing for void.
// A specialisation rather than std::conditional_t, which would name
// parse_target<void> (and fail its constraint) even for void.
struct no_parse_target
{};

template <typename T>
struct expected_value_target
{
    using type = parse_target<T>;
};

template <>
struct expected_value_target<void>
{
    using type = no_parse_target;
};

PNP_INTERNAL_NS_END

// A value prints as expected(1), an error as unexpected("e"). Both are marked,
// so a nested expected stays unambiguous: expected(expected(1)),
// expected(unexpected(1)), unexpected(1).
struct expected_pnp_default
{
    static constexpr std::u8string_view value_open_delimiter  = u8"expected(";
    static constexpr std::u8string_view value_close_delimiter = u8")";
    static constexpr std::u8string_view error_open_delimiter  = u8"unexpected(";
    static constexpr std::u8string_view error_close_delimiter = u8")";
};

// The value part and the error part, each between its own delimiters. The parts
// validated themselves when they were built; first_error() checks the
// delimiters, and that the two opening delimiters can't be confused.
//
// The parser decides between a value and an error by which opening delimiter
// the input starts with, so the two must not be prefixes of each other. One of
// them may be empty, making that alternative the one read when the other's
// delimiter isn't there, but then its own text must never start with the
// other's delimiter: an expected holding an expected would be ambiguous.
template <
    typename Value_Pnp,
    typename Error_Pnp,
    typename Expected_Pnp_Defaults>
struct expected_pnp_options
{
    using value_part_type = Value_Pnp;
    using error_part_type = Error_Pnp;

    std::u8string value_open_delimiter
        = std::u8string(Expected_Pnp_Defaults::value_open_delimiter);
    value_part_type value = {};
    std::u8string   value_close_delimiter
        = std::u8string(Expected_Pnp_Defaults::value_close_delimiter);
    std::u8string error_open_delimiter
        = std::u8string(Expected_Pnp_Defaults::error_open_delimiter);
    error_part_type error = {};
    std::u8string   error_close_delimiter
        = std::u8string(Expected_Pnp_Defaults::error_close_delimiter);

    // The first rule these options break, or nullptr if they're valid.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;

    // The two alternatives' text, value first, for the shared alternatives
    // functions.
    [[nodiscard]]
    constexpr std::array<PNP_INTERNAL_NAMESPACE::alternative_text, 2>
        alternatives() const noexcept;
};

// The policy is valid for these parts: its default options pass first_error().
template <
    typename Value_Pnp,
    typename Error_Pnp,
    typename Expected_Pnp_Defaults>
concept valid_expected_pnp_c = valid_default_options_c<
    expected_pnp_options<Value_Pnp, Error_Pnp, Expected_Pnp_Defaults>>;

// Prints and parses a std::expected: its value or its error, each between its
// own delimiters. For std::expected<void, E>, the value part is no_value_pnp,
// and a value prints as expected().
//
//   pnp::generic_expected_pnp<std::expected<int, std::string>,
//   pnp::default_printer<int>,
//                             pnp::default_printer<std::string>> p;   //
//                             expected(1)
template <
    typename X,
    typename Value_Pnp,
    typename Error_Pnp,
    typename Expected_Pnp_Defaults = expected_pnp_default>
requires valid_expected_pnp_c<Value_Pnp, Error_Pnp, Expected_Pnp_Defaults>
         && (
             ( PNP_INTERNAL_NAMESPACE::
                   expected_value_printer_c<Value_Pnp, expected_value_t<X>>
               && printer_c<Error_Pnp, expected_error_t<X>> )
             || ( PNP_INTERNAL_NAMESPACE::
                      expected_value_parser_c<Value_Pnp, expected_value_t<X>>
                  && parser_c<Error_Pnp, expected_error_t<X>> )
         )
struct generic_expected_pnp
    : public pnp_configuration<
          expected_pnp_options<Value_Pnp, Error_Pnp, Expected_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<
        expected_pnp_options<Value_Pnp, Error_Pnp, Expected_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    using value_part_type = Value_Pnp;
    using error_part_type = Error_Pnp;
    constexpr void
        run_printer(
            const std::same_as<X> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const
    requires PNP_INTERNAL_NAMESPACE::
                 expected_value_printer_c<Value_Pnp, expected_value_t<X>>
             && printer_c<Error_Pnp, expected_error_t<X>>;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            X&             object_answer,
            parser_run_key run_key_arg
        ) const
    requires PNP_INTERNAL_NAMESPACE::
                 expected_value_parser_c<Value_Pnp, expected_value_t<X>>
             && parser_c<Error_Pnp, expected_error_t<X>>
             && parse_constructible_c<expected_error_t<X>>;
};

// The type a std::unexpected's error is printed and parsed as.
template <typename U>
using unexpected_error_t
    = std::remove_cvref_t<decltype(std::declval<const U&>().error())>;

// An error on its own prints exactly as the error side of an expected:
// unexpected("e").
struct unexpected_pnp_default
{
    static constexpr std::u8string_view error_open_delimiter  = u8"unexpected(";
    static constexpr std::u8string_view error_close_delimiter = u8")";
};

template <typename Error_Pnp, typename Unexpected_Pnp_Defaults>
struct unexpected_pnp_options
{
    using error_part_type = Error_Pnp;

    std::u8string error_open_delimiter
        = std::u8string(Unexpected_Pnp_Defaults::error_open_delimiter);
    error_part_type error = {};
    std::u8string   error_close_delimiter
        = std::u8string(Unexpected_Pnp_Defaults::error_close_delimiter);

    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        using namespace PNP_INTERNAL_NAMESPACE;
        for (const std::u8string_view token :
             {std::u8string_view(error_open_delimiter),
              std::u8string_view(error_close_delimiter)})
        {
            if (not valid_utf8(token))
            {
                return "unexpected_pnp_options delimiters must be valid UTF-8";
            }
            if (not no_surrounding_whitespace(token))
            {
                return "unexpected_pnp_options delimiters must not start or "
                       "end with whitespace";
            }
        }
        return nullptr;
    }

    [[nodiscard]]
    constexpr std::array<PNP_INTERNAL_NAMESPACE::alternative_text, 1>
        alternatives() const noexcept
    {
        return {
            {{.marker_description = u8"error's opening delimiter",
              .open_delimiter     = error_open_delimiter,
              .close_delimiter    = error_close_delimiter,
              .value_description  = u8"the error of an unexpected"}}
        };
    }
};

template <typename Error_Pnp, typename Unexpected_Pnp_Defaults>
concept valid_unexpected_pnp_c = valid_default_options_c<
    unexpected_pnp_options<Error_Pnp, Unexpected_Pnp_Defaults>>;

// Prints and parses a std::unexpected as its error between the delimiters:
// unexpected("e").
template <typename U, typename Error_Pnp, typename Unexpected_Pnp_Defaults = unexpected_pnp_default>
    requires valid_unexpected_pnp_c<Error_Pnp, Unexpected_Pnp_Defaults>
&& (printer_c<Error_Pnp, unexpected_error_t<U>> || parser_c<Error_Pnp, unexpected_error_t<U>>)
struct generic_unexpected_pnp : public pnp_configuration<unexpected_pnp_options<Error_Pnp, Unexpected_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<
        unexpected_pnp_options<Error_Pnp, Unexpected_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    using error_part_type = Error_Pnp;
    constexpr void
        run_printer(
            const std::same_as<U> auto& object_arg,
            text_sink&     sink_obj_arg
        ) const requires printer_c<Error_Pnp, unexpected_error_t<U>>
    {
        PNP_INTERNAL_NAMESPACE::print_alternative(
            _options.alternatives()[0],
            [&]
            {
                _options.error.run_printer(object_arg.error(), sink_obj_arg);
            },
            sink_obj_arg
        );
    }

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            U&             object_answer,
            parser_run_key run_key_arg
        ) const
    requires parser_c<Error_Pnp, unexpected_error_t<U>>
             && parse_constructible_c<unexpected_error_t<U>>
    {
        PNP_INTERNAL_NAMESPACE::parse_target<unexpected_error_t<U>> target;
        std::size_t                                                 chosen = 0;
        if (auto result = PNP_INTERNAL_NAMESPACE::parse_alternatives(
                _options.alternatives(),
                parser_input_arg,
                chosen,
                [&](std::size_t, parser_input& input_arg)
                {
                    return input_arg.run_parser(_options.error, target.value);
                }
            );
            result.failed())
        {
            return result;
        }
        object_answer = U(std::move(target.value));
        return {};
    }
};

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// ------------------------------------------------------------ options

template <
    typename Value_Pnp,
    typename Error_Pnp,
    typename Expected_Pnp_Defaults>
constexpr const char*
    expected_pnp_options<Value_Pnp, Error_Pnp, Expected_Pnp_Defaults>::
        first_error() const noexcept
{
    using namespace PNP_INTERNAL_NAMESPACE;
    for (const std::u8string_view token :
         {std::u8string_view(value_open_delimiter),
          std::u8string_view(value_close_delimiter),
          std::u8string_view(error_open_delimiter),
          std::u8string_view(error_close_delimiter)})
    {
        if (not valid_utf8(token))
        {
            return "expected_pnp_options delimiters must be valid UTF-8";
        }
        if (not no_surrounding_whitespace(token))
        {
            return "expected_pnp_options delimiters must not start or end with "
                   "whitespace";
        }
    }

    // The parser decides between a value and an error by which one the input
    // starts with.
    if (not alternatives_distinguishable(std::array<std::u8string_view, 2>{
            value_open_delimiter, error_open_delimiter
        }))
    {
        return "expected_pnp_options value and error opening delimiters must "
               "not be prefixes of each other, and "
               "at most one may be empty";
    }
    return nullptr;
}

template <
    typename Value_Pnp,
    typename Error_Pnp,
    typename Expected_Pnp_Defaults>
constexpr std::array<PNP_INTERNAL_NAMESPACE::alternative_text, 2>
    expected_pnp_options<Value_Pnp, Error_Pnp, Expected_Pnp_Defaults>::
        alternatives() const noexcept
{
    return {
        {
         {.marker_description = u8"value's opening delimiter",
             .open_delimiter     = value_open_delimiter,
             .close_delimiter    = value_close_delimiter,
             .value_description  = u8"the value of an expected"},
         {.marker_description = u8"error's opening delimiter",
             .open_delimiter     = error_open_delimiter,
             .close_delimiter    = error_close_delimiter,
             .value_description  = u8"the error of an expected"},
         }
    };
}

// ------------------------------------------------------------ the expected pnp

template <
    typename X,
    typename Value_Pnp,
    typename Error_Pnp,
    typename Expected_Pnp_Defaults>
requires valid_expected_pnp_c<Value_Pnp, Error_Pnp, Expected_Pnp_Defaults>
         && (
             ( PNP_INTERNAL_NAMESPACE::
                   expected_value_printer_c<Value_Pnp, expected_value_t<X>>
               && printer_c<Error_Pnp, expected_error_t<X>> )
             || ( PNP_INTERNAL_NAMESPACE::
                      expected_value_parser_c<Value_Pnp, expected_value_t<X>>
                  && parser_c<Error_Pnp, expected_error_t<X>> )
         )
         constexpr void generic_expected_pnp<
             X,
             Value_Pnp,
             Error_Pnp,
             Expected_Pnp_Defaults>::
             run_printer(
                 const std::same_as<X> auto& object_arg,
                 text_sink&     sink_obj_arg
             ) const
         requires PNP_INTERNAL_NAMESPACE::
                      expected_value_printer_c<Value_Pnp, expected_value_t<X>>
                  && printer_c<Error_Pnp, expected_error_t<X>>
{
    const auto alternatives = _options.alternatives();
    if (object_arg.has_value())
    {
        PNP_INTERNAL_NAMESPACE::print_alternative(
            alternatives[0],
            [&]
            {
                if constexpr (not std::is_void_v<expected_value_t<X>>)
                {
                    _options.value.run_printer(*object_arg, sink_obj_arg);
                }
            },
            sink_obj_arg
        );
    }
    else
    {
        PNP_INTERNAL_NAMESPACE::print_alternative(
            alternatives[1],
            [&]
            {
                _options.error.run_printer(object_arg.error(), sink_obj_arg);
            },
            sink_obj_arg
        );
    }
}

template <
    typename X,
    typename Value_Pnp,
    typename Error_Pnp,
    typename Expected_Pnp_Defaults>
requires valid_expected_pnp_c<Value_Pnp, Error_Pnp, Expected_Pnp_Defaults>
         && (
             ( PNP_INTERNAL_NAMESPACE::
                   expected_value_printer_c<Value_Pnp, expected_value_t<X>>
               && printer_c<Error_Pnp, expected_error_t<X>> )
             || ( PNP_INTERNAL_NAMESPACE::
                      expected_value_parser_c<Value_Pnp, expected_value_t<X>>
                  && parser_c<Error_Pnp, expected_error_t<X>> )
         )
         constexpr basic_parser_return_value
         generic_expected_pnp<X, Value_Pnp, Error_Pnp, Expected_Pnp_Defaults>::
             run_parser(
                 parser_input&  parser_input_arg,
                 X&             object_answer,
                 parser_run_key run_key_arg
             ) const
         requires PNP_INTERNAL_NAMESPACE::
                      expected_value_parser_c<Value_Pnp, expected_value_t<X>>
                  && parser_c<Error_Pnp, expected_error_t<X>>
                  && parse_constructible_c<expected_error_t<X>>
{
    using value_type = expected_value_t<X>;
    using error_type = expected_error_t<X>;

    // Parsed into temporaries, so a failed parse leaves the expected untouched.
    // A void value has nothing to parse into.
    typename PNP_INTERNAL_NAMESPACE::expected_value_target<value_type>::type
                                                     value_target;
    PNP_INTERNAL_NAMESPACE::parse_target<error_type> error_target;

    std::size_t                                      chosen = 0;
    if (auto result = PNP_INTERNAL_NAMESPACE::parse_alternatives(
            _options.alternatives(),
            parser_input_arg,
            chosen,
            [&](const std::size_t index_arg,
                parser_input&     input_arg) -> basic_parser_return_value
            {
                if (index_arg == 1)
                {
                    return input_arg.run_parser(
                        _options.error, error_target.value
                    );
                }
                if constexpr (std::is_void_v<value_type>)
                {
                    return {}; // expected(): nothing between the delimiters
                }
                else
                {
                    return input_arg.run_parser(
                        _options.value, value_target.value
                    );
                }
            }
        );
        result.failed())
    {
        return result;
    }

    if (chosen == 1)
    {
        object_answer = X(std::unexpect, std::move(error_target.value));
    }
    else if constexpr (std::is_void_v<value_type>)
    {
        object_answer = X();
    }
    else
    {
        object_answer = X(std::in_place, std::move(value_target.value));
    }
    return {};
}

PNP_SUPPORT_NS_END
PNP_NS_END

#endif