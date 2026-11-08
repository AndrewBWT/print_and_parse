#pragma once
// pnp namespaces.
#include "pnp/internal/macros.hpp"
// Base class.
#include "pnp/sources/complete_text_source.hpp"
// Required for deleted constructor.
#include <string>
// Required for object.
#include <string_view>

PNP_NS_BEGIN

/*!
 * @brief A text_source that views text the caller owns, without copying it.
 *
 * The text must stay valid, and unchanged, while the source exists. Use
 * string_source to parse text the source should own instead.
 *
 * Anything that converts to a std::u8string_view is accepted: a std::u8string,
 * a std::u8string_view, a u8"..." literal, or a const char8_t*. A pointer is
 * read up to its first null character, as a C string.
 *
 * A temporary std::u8string, such as one returned from a function, is
 * rejected at compile time, as it would be destroyed while the source still
 * views it.
 *
 * Like every text_source, it can't be copied or moved.
 */
    struct string_view_source : public complete_text_source
{
public:
    /*!
     * @brief Creates a string_view_source viewing text_arg.
     *
     * @param text_arg The whole input. It must stay valid, and unchanged,
     * while the source exists.
     */
    explicit string_view_source(const std::u8string_view text_arg) noexcept;
    /*!
     * @brief Deleted: a temporary string would be destroyed while the source
     * still views it. Use string_source to parse text the source should own.
     */
    template <typename Allocator>
    string_view_source(std::basic_string<
        char8_t,
        std::char_traits<char8_t>,
        Allocator>&&)
        = delete;
};

PNP_NS_END

PNP_NS_BEGIN

inline string_view_source::string_view_source(
    const std::u8string_view text_arg
) noexcept
    : complete_text_source(text_arg)
{
}

PNP_NS_END