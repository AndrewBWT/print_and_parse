#pragma once
// pnp namespaces.
#include "pnp/internal/macros.hpp"
// Base class.
#include "pnp/sources/complete_text_source.hpp"
// std::constructible_from.
#include <concepts>
// Required for this object.
#include <string>
// std::forward
#include <utility>

PNP_NS_BEGIN

/*!
 * @brief A text_source that owns its text, a std::u8string handed to it.
 *
 * Use it when the text is in a string that won't outlive the parse, or that
 * may change during it. Use string_view_source to parse text that will stay
 * valid and unchanged for the whole parse, without copying it.
 *
 * Like every text_source, it can't be copied or moved.
 */
struct string_source : public complete_text_source
{
public:
    /*!
     * @brief Creates a string_source owning the text in text_arg.
     *
     * An rvalue std::u8string - for example, std::move(text) - is moved in,
     * without copying the text. Anything else - an lvalue std::u8string, a
     * literal, or a view - is copied.
     *
     * @tparam S The type of text_arg; a std::u8string must be constructible
     * from it.
     * @param text_arg The whole input.
     */
    template <typename S>
    requires std::constructible_from<std::u8string, S&&>
    explicit string_source(S&& text_arg);
private:
    //! The text owned by the object.
    std::u8string _owned_text;
};

PNP_NS_END

PNP_NS_BEGIN

template <typename S>
requires std::constructible_from<std::u8string, S&&>
inline string_source::string_source(
    S&& text_arg
)
    : complete_text_source(), _owned_text(std::forward<S>(text_arg))
{
    // Called here, as _owned_text is only constructed after the base.
    update_text(_owned_text, nullptr);
}

PNP_NS_END