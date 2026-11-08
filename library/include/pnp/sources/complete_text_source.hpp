#pragma once
// For text_source.
#include "pnp/core/parse/text_source.hpp"
// For pnp namespaces.
#include "pnp/internal/macros.hpp"

// For std::size_t.
#include <cstddef>
// For the numeric_limits.
#include <limits>
// For the text.
#include <string_view>

PNP_NS_BEGIN

/*!
 * @brief A base class for text_sources whose text is all in memory from the
 * start.
 *
 * The whole input is set once, when the source is constructed. There is never
 * more text to read, and the text never moves, so there is no storage to
 * manage. input_text objects are never able to go out of scope.
 *
 * Releasing text from these sources frees no memory, so parser_input never
 * releases from them: minimum_release() is the largest std::size_t.
 * release_up_to() still works, as it ignores minimum_release().
 *
 * To write one, derive from it, and either pass the text to the constructor,
 * or - if the derived class owns the text, so it doesn't exist yet while this
 * base is constructed - use the default constructor, and call
 * update_text(text, nullptr) from the derived class's constructor.
 *
 * The text is checked for valid UTF-8 in one pass, when it is set.
 */
struct complete_text_source : public text_source
{
protected:
    /*!
     * @brief Creates a source with no text yet.
     *
     * For a derived class that owns its text: call update_text(text, nullptr)
     * from its constructor, once the text exists.
     */
    explicit complete_text_source() noexcept;
    /*!
     * @brief Creates a complete_text_source holding text_arg as the whole
     * input.
     *
     * @param text_arg The whole input. It must stay valid, and unchanged,
     * while the complete_text_source exists.
     */
    explicit complete_text_source(const std::u8string_view text_arg);
    /*!
     * @brief Does nothing, as there is never more to read.
     * @return Always false.
     */
    bool
        fetch_more() final;
    /*!
     * @brief Does nothing: the text never moves, so there's no old storage.
     */
    void
        free_storage() final;
};

PNP_NS_END

PNP_NS_BEGIN

inline complete_text_source::complete_text_source() noexcept
    : text_source(true, std::numeric_limits<std::size_t>::max())
{}

inline complete_text_source::complete_text_source(
    const std::u8string_view text_arg
)
    : text_source(true, std::numeric_limits<std::size_t>::max())
{
    update_text(text_arg, nullptr);
}

inline bool
    complete_text_source::fetch_more()
{
    return false;
}

inline void
    complete_text_source::free_storage()
{}

PNP_NS_END