#pragma once

#include "pnp/internal/macros.hpp"

#include <string_view>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN

/*!
 * @brief Object where printers write their output. They can write UTF-8 text,
 * std::strings, or a char8_t at a time. Each derived text_sink decides what
 * to do with the text.
 *
 * To create a text_sink, derive from it, and implement write_text(const
 * std::u8string_view). put_char can also be overridden, if required.
 */
struct text_sink
{
public:
    // Copy constructor and copy assignment operator deleted. Having multiple
    // sinks can cause issues.
    text_sink(const text_sink&) = delete;
    text_sink&
        operator=(const text_sink&)
        = delete;
    /*!
     * @brief Destructor. Default behaviour for base class, but can be
     * overridden.
     */
    virtual ~text_sink() = default;
    /*!
     * @brief Writes a single UTF-8 code unit.
     * @param char_arg The code unit being written.
     */
    void
        put(const char8_t char_arg);
    /*!
     * @brief Writes UTF-8 text to the sink.
     * @param text_arg The text to be written.
     */
    void
        write(const std::u8string_view text_arg);
    /*!
     * @brief Writes std::string_views, casting the argument to a UTF-8 string.
     * Assumes the text is UTF-8 encoded.
     *
     * The text isn't copied or converted.
     *
     * @param text_arg The text to be written.
     */
    void
        write(const std::string_view text_arg);
protected:
    /*!
     * @brief Default constructor.
     *
     * Protected so that sub-classes can call it, but cannot be instantiated
     * publicly.
     */
    text_sink() = default;
private:
    /*!
     * @brief Writes a single UTF-8 code unit to the destination.
     *
     * By default, writes a one-character string using write_text.
     *
     * @param char_arg The code unit being written.
     */
    virtual void
        put_char(const char8_t char_arg);
    /*!
     * @brief Writes UTF-8 text to the destination.
     * @param text_arg The text to be written.
     */
    virtual void
        write_text(const std::u8string_view text_arg)
        = 0;
};

PNP_NS_END

PNP_NS_BEGIN
inline void
    text_sink::put(
        const char8_t char_arg
    )
{
    put_char(char_arg);
}

inline void
    text_sink::write(
        const std::u8string_view text_arg
    )
{
    write_text(text_arg);
}

inline void
    text_sink::write(
        const std::string_view text_arg
    )
{
    write_text(std::u8string_view(
        UNICODE_BRIDGE_NAMESPACE::cast_char_ptr<char8_t>(text_arg.data()),
        text_arg.size()
    ));
}

inline void
    text_sink::put_char(
        const char8_t char_arg
    )
{
    write_text(std::u8string_view(&char_arg, 1));
}

PNP_NS_END