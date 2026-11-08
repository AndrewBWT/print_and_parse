#pragma once

#include "pnp/core.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <unicode_bridge.hpp>
PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

PNP_INTERNAL_NS_BEGIN
inline std::u8string
    int_to_u8string(
        const int value_arg
    )
{
    char              buffer[16];
    const char* const begin = buffer;
    const char* const end
        = std::to_chars(buffer, buffer + sizeof(buffer), value_arg).ptr;
    return std::u8string(begin, end);
}

constexpr std::u8string
    size_to_u8string(std::size_t value_arg);
/*!
 * @brief Fills an array with 0..N.
 * @tparam N
 * @return
 */
template <std::size_t N>
constexpr std::array<std::size_t, N>
    make_index_array() noexcept;
/*!
 * @brief Concept for a sortable range. Needs std::size, indeing, and movable
 * elements
 */
template <typename Range>
concept sortable_range_c = requires (Range& range_arg, std::size_t index_arg) {
    { std::size(range_arg) } -> std::convertible_to<std::size_t>;
    range_arg[index_arg];
    requires std::movable<std::remove_cvref_t<decltype(range_arg[index_arg])>>;
};
template <typename Range, typename Compare>
requires sortable_range_c<Range>
         && std::predicate<
             Compare&,
             const std::remove_cvref_t<decltype(std::declval<Range&>()[0])>&,
             const std::remove_cvref_t<decltype(std::declval<Range&>()[0])>&>
constexpr void
    stable_sort(Range& range_arg, Compare compare_arg);

constexpr bool
    valid_utf8(
        const std::u8string_view text_arg
    ) noexcept
{
    auto itt = text_arg.begin();
    while (itt != text_arg.end())
    {
        if (not unicode_bridge::next_char32_and_increment_iterator_no_error(
                    itt, text_arg.end()
            )
                    .has_value())
        {
            return false;
        }
    }
    return true;
}

// True if every code point in the text is whitespace. An empty string passes.
constexpr bool
    only_whitespace(const std::u8string_view text_arg) noexcept;

// Quotes text for an error message, with escapes, so whitespace is visible: "
// ", "\t".
constexpr std::u8string
    quoted(const std::u8string_view text_arg);

constexpr bool
    no_surrounding_whitespace(
        const std::u8string_view text_arg
    ) noexcept
{
    using namespace unicode_bridge;
    if (text_arg.empty())
    {
        return true;
    }
    const auto begin_itt        = text_arg.begin();
    const auto end_itt          = text_arg.end();
    const auto first_code_point = next_char32_no_error(begin_itt, end_itt);
    const auto last_code_point  = prev_char32_no_error(end_itt, begin_itt);
    if (not (first_code_point.has_value()) || not (last_code_point.has_value()))
    {
        return false;
    }
    return not (
        is_whitespace(first_code_point.value().first)
        || is_whitespace(last_code_point.value().first)
    );
}

template <typename T>
requires parse_constructible_c<T>
struct parse_target
{
    T value;

    constexpr parse_target()
    requires std::is_default_constructible_v<T>
        : value{}
    {}

    constexpr parse_target()
    requires (not std::is_default_constructible_v<T>)
        : value(placeholder_value<T>::value())
    {}
};

PNP_INTERNAL_NS_END
PNP_SUPPORT_NS_END
PNP_NS_END

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

PNP_INTERNAL_NS_BEGIN
constexpr std::u8string
    size_to_u8string(
        std::size_t value_arg
    )
{
    char8_t     buffer[std::numeric_limits<std::size_t>::digits10 + 1];
    std::size_t pos = sizeof(buffer);
    do
    {
        buffer[--pos]  = static_cast<char8_t>(u8'0' + value_arg % 10);
        value_arg     /= 10;
    }
    while (value_arg != 0);
    return std::u8string(buffer + pos, buffer + sizeof(buffer));
}

template <std::size_t N>
constexpr std::array<std::size_t, N>
    make_index_array() noexcept
{
    using namespace std;
    return []<size_t... Indices>(index_sequence<Indices...>)
    {
        return array<size_t, N>{Indices...};
    }(std::make_index_sequence<N>{});
}

constexpr bool
    is_digit(
        const char8_t code_unit_arg
    )
{
    return code_unit_arg >= u8'0' && code_unit_arg <= u8'9';
}
constexpr bool
is_number_code_unit(const char8_t code_unit_arg) noexcept
{
    return (code_unit_arg >= u8'0' && code_unit_arg <= u8'9') || (code_unit_arg >= u8'a' && code_unit_arg <= u8'z')
        || (code_unit_arg >= u8'A' && code_unit_arg <= u8'Z') || code_unit_arg == u8'.' || code_unit_arg == u8'+'
        || code_unit_arg == u8'-';
}
constexpr bool
    is_base_digit(
        const char8_t code_unit_arg,
        const int     base_arg
    ) noexcept
{
    int value = base_arg;   // anything not a digit is worth "too much"
    if (code_unit_arg >= u8'0' && code_unit_arg <= u8'9')
    {
        value = code_unit_arg - u8'0';
    }
    else if (code_unit_arg >= u8'a' && code_unit_arg <= u8'z')
    {
        value = code_unit_arg - u8'a' + 10;
    }
    else if (code_unit_arg >= u8'A' && code_unit_arg <= u8'Z')
    {
        value = code_unit_arg - u8'A' + 10;
    }
    return value < base_arg;
}

template <typename Range, typename Compare>
requires sortable_range_c<Range>
         && std::predicate<
             Compare&,
             const std::remove_cvref_t<decltype(std::declval<Range&>()[0])>&,
             const std::remove_cvref_t<decltype(std::declval<Range&>()[0])>&>
constexpr void
    stable_sort(
        Range&  range_arg,
        Compare compare_arg
    )
{
    using namespace std;
    const size_t size = std::size(range_arg);
    for (size_t idx = 1; idx < size; ++idx)
    {
        auto   current = std::move(range_arg[idx]);
        size_t jdx     = idx;
        // Only strictly-earlier elements move past current, which is what keeps
        // it stable.
        while (jdx > 0 && compare_arg(current, range_arg[jdx - 1]))
        {
            range_arg[jdx] = std::move(range_arg[jdx - 1]);
            --jdx;
        }
        range_arg[jdx] = std::move(current);
    }
}

constexpr bool
    only_whitespace(
        const std::u8string_view text_arg
    ) noexcept
{
    auto itt = text_arg.begin();
    while (itt != text_arg.end())
    {
        const auto decoded
            = unicode_bridge::next_char32_and_increment_iterator_no_error(
                itt, text_arg.end()
            );
        if (not decoded.has_value()
            || not unicode_bridge::is_whitespace(decoded.value()))
        {
            return false;
        }
    }
    return true;
}

constexpr std::u8string
    quoted(
        const std::u8string_view text_arg
    )
{
    return u8"\""
           + unicode_bridge::to_formatted_unicode_string<char8_t>(text_arg)
           + u8"\"";
}
static_assert(is_base_digit(u8'9', 10));
static_assert(not is_base_digit(u8'a', 10));
static_assert(is_base_digit(u8'f', 16));
static_assert(is_base_digit(u8'F', 16));
static_assert(not is_base_digit(u8'g', 16));
static_assert(not is_base_digit(u8'i', 16));
static_assert(not is_base_digit(u8'2', 2));
static_assert(is_base_digit(u8'z', 36));
PNP_INTERNAL_NS_END
PNP_SUPPORT_NS_END
PNP_NS_END