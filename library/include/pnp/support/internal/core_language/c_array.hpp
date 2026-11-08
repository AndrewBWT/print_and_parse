#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/sequences.hpp"
#include "pnp/support/internal/std/string.hpp" // generic_string_pnp, string_pnp_default
#include "pnp/support/internal/utility.hpp"

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// ============================================================ arrays as
// sequences

// A built-in array is a sequence of exactly N elements: [1,2,3]. The sequence
// pnp already handles arrays (the fixed-size appender checks the count), so
// this only names the case.
//
//   pnp::generic_c_array_pnp<int[3], pnp::default_printer<int>> p; // [1,2,3]
template <
    typename Array,
    typename Element_Pnp,
    typename Sequence_Pnp_Defaults = sequence_pnp_default>
requires std::is_bounded_array_v<Array>
using generic_c_array_pnp
    = generic_sequence_pnp<Array, Element_Pnp, Sequence_Pnp_Defaults>;

// ============================================================ character arrays
// as strings

// An array of (non-volatile) characters, read and written as text.
template <typename Array>
concept c_string_array_c
    = std::is_bounded_array_v<Array> && std::rank_v<Array> == 1
      && unicode_bridge::is_char_type_c<std::remove_extent_t<Array>>
      && std::same_as<
          std::remove_extent_t<Array>,
          std::remove_cv_t<std::remove_extent_t<Array>>>;

// C-string semantics by default: the text ends at the first null, in quotes:
// "abc".
struct c_string_array_pnp_default
{
    using text_defaults                = string_pnp_default;
    static constexpr bool stop_at_null = true;
};

template <typename C_String_Array_Pnp_Defaults>
concept valid_c_string_array_pnp_c
    = valid_string_pnp_c<typename C_String_Array_Pnp_Defaults::text_defaults>
      && std::convertible_to<
          decltype(C_String_Array_Pnp_Defaults::stop_at_null),
          bool>;

// The text part is a string pnp, validated when it was built; stop_at_null
// can't be invalid. So there's no first_error().
//
//   stop_at_null = true   the text ends at the first null, and parsing leaves
//   room for a
//                         terminator: a char[8] holds at most 7 characters, and
//                         never an embedded null
//   stop_at_null = false  all N elements are the text, nulls included; parsing
//   fills any
//                         elements after the text with nulls
template <typename Array, typename C_String_Array_Pnp_Defaults>
requires c_string_array_c<Array>
         && valid_c_string_array_pnp_c<C_String_Array_Pnp_Defaults>
struct c_string_array_pnp_options
{
    using char_type      = std::remove_cv_t<std::remove_extent_t<Array>>;
    using string_type    = std::basic_string<char_type>;
    using text_part_type = generic_string_pnp<
        string_type,
        typename C_String_Array_Pnp_Defaults::text_defaults>;

    text_part_type text         = {};
    bool           stop_at_null = C_String_Array_Pnp_Defaults::stop_at_null;

    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        return nullptr;
    }
};

// Prints and parses a character array as a quoted string.
//
//   using array_pnp = pnp::generic_c_string_array_pnp<char[8]>;
//   array_pnp p;                                        // "abc" (stops at the
//   first null) array_pnp q({.stop_at_null = false});               //
//   "abc\0\0\0\0\0" (all 8 elements)
template <
    typename Array,
    typename C_String_Array_Pnp_Defaults = c_string_array_pnp_default>
requires c_string_array_c<Array>
         && valid_c_string_array_pnp_c<C_String_Array_Pnp_Defaults>
struct generic_c_string_array_pnp
    : public pnp_configuration<
          c_string_array_pnp_options<Array, C_String_Array_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<
        c_string_array_pnp_options<Array, C_String_Array_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    using text_part_type = typename base_t::options_type::text_part_type;

    constexpr void
        run_printer(
            const std::same_as<Array> auto& object_arg,
            text_sink&         sink_obj_arg
        ) const;

    // Not for arrays of const characters, which can't be written to.
    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            Array&         object_answer,
            parser_run_key run_key_arg
        ) const
    requires (not std::is_const_v<std::remove_extent_t<Array>>);
private:
    using char_type   = typename base_t::options_type::char_type;
    using string_type = typename base_t::options_type::string_type;

    static constexpr std::size_t array_size = std::extent_v<Array>;
};

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename Array, typename C_String_Array_Pnp_Defaults>
requires c_string_array_c<Array>
         && valid_c_string_array_pnp_c<C_String_Array_Pnp_Defaults>
constexpr void
    generic_c_string_array_pnp<Array, C_String_Array_Pnp_Defaults>::run_printer(
        const std::same_as<Array> auto& object_arg,
        text_sink&         sink_obj_arg
    ) const
{
    std::size_t length = array_size;
    if (_options.stop_at_null)
    {
        // Searches only within the array, so an array without a terminator is
        // safe too.
        length = 0;
        while (length < array_size && object_arg[length] != char_type{})
        {
            ++length;
        }
    }
    PNP_INTERNAL_NAMESPACE::print_delimited_text(
        _options.text.options(),
        std::basic_string_view<char_type>(object_arg, length),
        sink_obj_arg
    );
}

template <typename Array, typename C_String_Array_Pnp_Defaults>
requires c_string_array_c<Array>
         && valid_c_string_array_pnp_c<C_String_Array_Pnp_Defaults>
         constexpr basic_parser_return_value
         generic_c_string_array_pnp<Array, C_String_Array_Pnp_Defaults>::
             run_parser(
                 parser_input&  parser_input_arg,
                 Array&         object_answer,
                 parser_run_key run_key_arg
             ) const
         requires (not std::is_const_v<std::remove_extent_t<Array>>)
{
    // Recorded so size and null errors point at the string, not past it.
    const std::size_t text_start = parser_input_arg.position();

    string_type       parsed;
    if (auto text_result = parser_input_arg.run_parser(_options.text, parsed);
        text_result.failed())
    {
        return text_result;
    }

    if (_options.stop_at_null)
    {
        if (parsed.find(char_type{}) != string_type::npos)
        {
            return parser_input_arg.fail(
                u8"A C string can't contain an embedded null character. To "
                u8"keep nulls, set stop_at_null to false.",
                text_start
            );
        }
        if (parsed.size() >= array_size)
        {
            return parser_input_arg.fail(
                u8"The string has "
                    + PNP_INTERNAL_NAMESPACE::size_to_u8string(parsed.size())
                    + u8" characters, but an array of "
                    + PNP_INTERNAL_NAMESPACE::size_to_u8string(array_size)
                    + u8" holds at most "
                    + PNP_INTERNAL_NAMESPACE::size_to_u8string(array_size - 1)
                    + u8" characters and its null terminator.",
                text_start
            );
        }
    }
    else if (parsed.size() > array_size)
    {
        return parser_input_arg.fail(
            u8"The string has "
                + PNP_INTERNAL_NAMESPACE::size_to_u8string(parsed.size())
                + u8" characters, but the array holds "
                + PNP_INTERNAL_NAMESPACE::size_to_u8string(array_size) + u8".",
            text_start
        );
    }

    // The text, then nulls for the rest: always terminated in stop-at-null
    // mode.
    std::size_t idx = 0;
    for (; idx < parsed.size(); ++idx)
    {
        object_answer[idx] = parsed[idx];
    }
    for (; idx < array_size; ++idx)
    {
        object_answer[idx] = char_type{};
    }
    return {};
}

PNP_SUPPORT_NS_END
PNP_NS_END