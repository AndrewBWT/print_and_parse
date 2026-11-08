#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/core_language/bool.hpp"
#include "pnp/support/internal/core_language/integers.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN

template <typename E>
requires std::is_enum_v<E>
struct enum_element
{
    E                  value;
    std::u8string_view name;
};
template <typename E>
struct enum_list;

template <typename E>
constexpr const char*
    enum_list_error() noexcept;

template <typename E>
concept listed_enum_c = std::is_enum_v<E> && requires {
    std::size(enum_list<E>::values);
    requires std::same_as<
        std::remove_cvref_t<decltype(enum_list<E>::values[0])>,
        enum_element<E>>;
    requires enum_list_error<E>() == nullptr;
};

PNP_SUPPORT_NS_BEGIN

PNP_INTERNAL_NS_BEGIN
template <typename E>
requires listed_enum_c<E>
[[nodiscard]]
constexpr std::size_t
    longest_name_size() noexcept
{
    std::size_t longest = 0;
    const auto& values  = enum_list<E>::values;
    for (const auto& [value, name] : values) // however the names are stored
    {
        longest = std::max(longest, std::u8string_view(name).size());
    }
    return longest;
}

template <typename E>
requires listed_enum_c<E>
inline constexpr auto enum_parse_order = []
{
    using namespace std;
    const auto&      values = enum_list<E>::values;
    constexpr size_t count  = std::size(enum_list<E>::values);
    auto             order  = PNP_INTERNAL_NAMESPACE::make_index_array<count>();
    PNP_INTERNAL_NAMESPACE::stable_sort(
        order,
        [&values](const std::size_t left_arg, const std::size_t right_arg)
        {
            return values[left_arg].name.size() > values[right_arg].name.size();
        }
    );
    return order;
}();

// This is all because of bool.
// enum pnp uses an underlying pnp for printing/parsing the underlying type.
// Most of the time this is integer, but bool is a possible underlying type for
// an enum, so this is written generically to ensure that's taken care of
// correctly.
template <typename Underlying>
struct underlying_value_part
{
    using type = integer_pnp<Underlying>;
};

template <>
struct underlying_value_part<bool>
{
    using type = bool_pnp<>;
};

template <typename Underlying>
using underlying_value_part_t =
    typename underlying_value_part<Underlying>::type;
PNP_INTERNAL_NS_END

/*!
 * @brief The default struct defining how to print/parse enums.
 */
struct listed_enum_pnp_default
{
    static constexpr bool               allow_unlisted  = true;
    static constexpr std::u8string_view unlisted_prefix = u8"#";
};

/*!
 * @brief The options object, which does all the heavy lifting ensuring checks
 * are performed.
 * @tparam E
 * @tparam Listed_Enum_Pnp_Defaults
 */
template <typename E, typename Listed_Enum_Pnp_Defaults>
requires listed_enum_c<E>
struct listed_enum_pnp_options
{
    using underlying_part_type
        = PNP_INTERNAL_NAMESPACE::underlying_value_part_t<
            std::underlying_type_t<E>>;

    bool          allow_unlisted = Listed_Enum_Pnp_Defaults::allow_unlisted;
    std::u8string unlisted_prefix
        = std::u8string(Listed_Enum_Pnp_Defaults::unlisted_prefix);
    underlying_part_type underlying = {};

    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;

    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept
    {
        if (allow_unlisted)
        {
            return std::nullopt;
        }
        const auto&       values = enum_list<E>::values;
        const std::size_t width  = values[0].name.size();
        for (const auto& element : values)
        {
            if (element.name.size() != width)
            {
                return std::nullopt;
            }
        }
        return width;
    }
};

// Concept which ensures that first_error is called at compile time for enum
// constants.
template <typename E, typename Listed_Enum_Pnp_Defaults>
concept valid_listed_enum_pnp_c = valid_default_options_c<
    listed_enum_pnp_options<E, Listed_Enum_Pnp_Defaults>>;

// The actual pnp for enums.
template <
    typename E,
    typename Listed_Enum_Pnp_Defaults = listed_enum_pnp_default>
requires listed_enum_c<E>
         && valid_listed_enum_pnp_c<E, Listed_Enum_Pnp_Defaults>
struct listed_enum_pnp
    : public pnp_configuration<
          listed_enum_pnp_options<E, Listed_Enum_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<
        listed_enum_pnp_options<E, Listed_Enum_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;

    using underlying_part_type =
        typename base_t::options_type::underlying_part_type;
    constexpr void
        run_printer(
            const std::same_as<E> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            E&             object_answer,
            parser_run_key run_key_arg
        ) const;
private:
    using underlying_type = std::underlying_type_t<E>;
};

PNP_SUPPORT_NS_END
template <typename T>
inline constexpr bool enable_default_enum_pnp = false;

// A listed enum's defaults: user has to explicitly confirm they want, using the
// template above.
template <typename E>
requires listed_enum_c<E> && enable_default_enum_pnp<E>
struct default_printer<E> : public PNP_SUPPORT_NAMESPACE::listed_enum_pnp<E>
{
    using base_t = PNP_SUPPORT_NAMESPACE::listed_enum_pnp<E>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename E>
requires listed_enum_c<E> && enable_default_enum_pnp<E>
struct default_parser<E> : public PNP_SUPPORT_NAMESPACE::listed_enum_pnp<E>
{
    using base_t = PNP_SUPPORT_NAMESPACE::listed_enum_pnp<E>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END

PNP_NS_BEGIN

template <typename E>
constexpr const char*
    enum_list_error() noexcept
{
    using namespace std;
    const auto&  enum_list_var = enum_list<E>::values;
    const size_t count         = std::size(enum_list_var);
    if (count == 0)
    {
        return "enum_list must contain at least one entry";
    }
    for (size_t idx = 0; idx < count; ++idx)
    {
        const u8string_view name = enum_list_var[idx].name;
        if (not PNP_SUPPORT_NAMESPACE::PNP_INTERNAL_NAMESPACE::valid_utf8(name))
        {
            return "enum_list names must be valid UTF-8";
        }
        if (not PNP_SUPPORT_NAMESPACE::PNP_INTERNAL_NAMESPACE::
                no_surrounding_whitespace(name))
        {
            return "enum_list names must not start or end with whitespace";
        }
        if (name.empty())
        {
            return "enum_list names must be non-empty";
        }
        for (size_t jdx = idx + 1; jdx < count; ++jdx)
        {
            if (enum_list_var[idx].value == enum_list_var[jdx].value)
            {
                return "enum_list values must be unique";
            }
            if (name == enum_list_var[jdx].name)
            {
                return "enum_list names must be unique";
            }
        }
    }
    return nullptr;
}

PNP_SUPPORT_NS_BEGIN

template <typename E, typename Listed_Enum_Pnp_Defaults>
requires listed_enum_c<E>
constexpr const char*
    listed_enum_pnp_options<E, Listed_Enum_Pnp_Defaults>::first_error(
    ) const noexcept
{
    using namespace PNP_INTERNAL_NAMESPACE;
    // The prefix is only used, and so only checked, when unlisted values are
    // allowed.
    if (not allow_unlisted)
    {
        return nullptr;
    }
    if (not valid_utf8(unlisted_prefix))
    {
        return "listed_enum_pnp_options unlisted prefix must be valid UTF-8";
    }
    if (not no_surrounding_whitespace(unlisted_prefix))
    {
        return "listed_enum_pnp_options unlisted prefix must not start or end "
               "with whitespace";
    }
    if (unlisted_prefix.empty())
    {
        return "listed_enum_pnp_options unlisted prefix must be non-empty";
    }
    for (const auto& element : enum_list<E>::values)
    {
        if (element.name.starts_with(unlisted_prefix))
        {
            return "listed_enum_pnp_options unlisted prefix must not be the "
                   "start of any enum_list name";
        }
    }
    return nullptr;
}

template <typename E, typename Listed_Enum_Pnp_Defaults>
requires listed_enum_c<E>
         && valid_listed_enum_pnp_c<E, Listed_Enum_Pnp_Defaults>
constexpr void
    listed_enum_pnp<E, Listed_Enum_Pnp_Defaults>::run_printer(
        const std::same_as<E> auto& object_arg,
        text_sink&                  sink_obj_arg
    ) const
{
    for (const auto& element : enum_list<E>::values)
    {
        if (element.value == object_arg)
        {
            sink_obj_arg.write(element.name);
            return;
        }
    }

    if (not _options.allow_unlisted)
    {
        throw print_exception("listed_enum_pnp: the value is not in "
                                "enum_list, and allow_unlisted is false");
    }
    else
    {
        sink_obj_arg.write(std::u8string_view(_options.unlisted_prefix));
        _options.underlying.run_printer(
            static_cast<underlying_type>(object_arg), sink_obj_arg
        );
    }
}

template <typename E, typename Listed_Enum_Pnp_Defaults>
requires listed_enum_c<E>
         && valid_listed_enum_pnp_c<E, Listed_Enum_Pnp_Defaults>
constexpr basic_parser_return_value
    listed_enum_pnp<E, Listed_Enum_Pnp_Defaults>::run_parser(
        parser_input&  parser_input_arg,
        E&             object_answer,
        parser_run_key run_key_arg
    ) const
{
    using namespace std;
    if (_options.allow_unlisted
        && parser_input_arg.advance_if_starts_with(_options.unlisted_prefix))
    {
        underlying_type underlying{};
        if (auto number_result
            = parser_input_arg.run_parser(_options.underlying, underlying);
            number_result.failed())
        {
            number_result.add_msg(
                u8"while parsing the number of an unlisted enum value"
            );
            return number_result;
        }
        object_answer = static_cast<E>(underlying);
        return {};
    }

    // A name, longest first, so the first match is the longest. Each check
    // reads only until the input stops matching, so on interactive input,
    // telling red from reddish needs just the character after red, not enough
    // for the longest name.
    const auto& values = enum_list<E>::values;
    for (const size_t idx : PNP_INTERNAL_NAMESPACE::enum_parse_order<E>)
    {
        if (parser_input_arg.advance_if_starts_with(values[idx].name))
        {
            object_answer = values[idx].value;
            return {};
        }
    }
    // Nothing matched, so create an error message.
    const std::size_t count = std::size(values);
    std::u8string     msg   = u8"Expected one of ";
    for (std::size_t idx = 0; idx < count; ++idx)
    {
        if (idx != 0)
        {
            msg.append(idx + 1 == count ? u8" or " : u8", ");
        }
        msg.append(u8"\"");
        msg.append(values[idx].name);
        msg.append(u8"\"");
    }
    if (_options.allow_unlisted)
    {
        msg.append(
            u8", or a value after \"" + _options.unlisted_prefix + u8"\""
        );
    }
    msg.append(u8".");
    return parser_input_arg.fail(std::move(msg));
}

PNP_SUPPORT_NS_END
PNP_NS_END