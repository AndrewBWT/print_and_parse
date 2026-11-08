#pragma once

#include "pnp/core/print/text_sink.hpp"
#include "pnp/internal/macros.hpp"

#include <ostream>
#include <string_view>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN

/*!
 * @brief text_sink that writes a std::ostream. Can be used for std::cout
 * (assuming the console has been set correctly), a file, or a
 * std::ostringstream.
 *
 * The UTF-8 is written as it is given, with no conversion. The stream given to
 * the constructor must outlive the text_sink.
 */
struct ostream_sink : public text_sink
{
public:
    /*!
     * @brief Constructs the ostream_sink, given the stream to write to.
     *
     * stream_arg must outlive the object.
     *
     * @param stream_arg The stream to write to.
     */
    explicit ostream_sink(std::ostream& stream_arg) noexcept;
private:
    //! The stream being written to.
    std::ostream* _stream;
    /*!
     * @brief Writes a single char8_t to _stream.
     * @param char_arg The char8_t to write to the stream.
     *
     * Errors are reported through the stream's state, as with any std::ostream.
     * Check the stream after printing, or enable exceptions on it with
     * exceptions().
     *
     */
    void
        put_char(const char8_t char_arg) override;
    /*!
     * @brief Writes a std::u8string_view to _stream.
     *
     * Errors are reported through the stream’s state, as with any std::ostream.
     * Check the stream after printing, or enable exceptions on it with
     * exceptions().
     *
     * @param text_arg The string to write to the stream.
     */
    void
        write_text(const std::u8string_view text_arg) override;
};

PNP_NS_END


PNP_NS_BEGIN

inline ostream_sink::ostream_sink(
    std::ostream& stream_arg
) noexcept
    : _stream(&stream_arg)
{}

inline void
    ostream_sink::put_char(
        const char8_t char_arg
    )
{
    _stream->put(static_cast<char>(char_arg));
}

inline void
    ostream_sink::write_text(
        const std::u8string_view text_arg
    )
{
    _stream->write(
        UNICODE_BRIDGE_NAMESPACE::cast_char_ptr<char>(text_arg.data()),
        static_cast<std::streamsize>(text_arg.size())
    );
}

PNP_NS_END