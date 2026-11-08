#pragma once
// pnp namespaces.
#include "pnp/internal/macros.hpp"
// Base class.
#include "pnp/sources/buffered_text_source.hpp"

// std::size_t.
#include <cstddef>
// std::ios_base::failure, std::streamsize.
#include <ios>
// std::istream.
#include <istream>
// The space read_piece() writes to.
#include <span>
// std::invalid_argument.
#include <stdexcept>

PNP_NS_BEGIN

/*!
 * @brief A text_source that reads from a std::istream.
 *
 * The bytes are read as they are, and taken to be UTF-8, whatever the stream's
 * locale.
 *
 * It is recomended that files are opened in binary mode (std::ios::binary), so
 * that line endings aren't changed on the way in, that the endings of files are
 * dealt with correctly, and that the positional data of errors from pnp matches
 * the text the user sess.
 *
 * The stream must outlive the source. Reading leaves the stream at the end of
 * the input, or just after the last code unit read by the text_source.
 *
 * How it reads depends on reads_without_waiting():
 * - If true - for files, and string streams - each read asks for as much text
 *   as there is room for, which is fastest.
 * - If false - for std::cin, pipes, and sockets - each read waits for one code
 *   unit, then takes only what has already arrived. A parser waiting for one
 *   more character then gets it as soon as it's typed, rather than once a
 *   whole block has been.
 *
 * When reading from std::cin, calling
 * std::ios_base::sync_with_stdio(false) first lets it take more than one code
 * unit at a time.
 *
 * If the stream reports an error (badbit), read_more() throws
 * std::ios_base::failure, rather than treating it as the end of the input.
 */
struct istream_source : public buffered_text_source
{
public:
    /*!
     * @brief Creates an istream_source reading from stream_arg.
     *
     * Reads nothing until text is first needed.
     *
     * @param stream_arg The stream to read from. It must outlive the source.
     * @param reads_without_waiting_arg See
     * text_source::reads_without_waiting(). True for files and string streams;
     * false for std::cin, pipes and sockets.
     * @param minimum_release_arg See text_source::minimum_release(). Defaults
     * to current_pnp_options().text_source_minimum_release.
     * @param minimum_block_size_arg See
     * buffered_text_source::minimum_block_size(). Defaults to
     * current_pnp_options().buffered_source_minimum_block_size.
     * @param maximum_reused_block_size_arg See
     * buffered_text_source::maximum_reused_block_size(). Defaults to
     * current_pnp_options().buffered_source_maximum_reused_block_size.
     * @param minimum_read_fraction_arg See
     * buffered_text_source::minimum_read_size(). Defaults to
     * current_pnp_options().buffered_source_minimum_read_fraction.
     * @param growth_factor_arg See buffered_text_source::growth_factor().
     * Defaults to current_pnp_options().buffered_source_growth_factor.
     * @param compaction_fraction_arg See
     * buffered_text_source::compaction_fraction(). Defaults to
     * current_pnp_options().buffered_source_compaction_fraction.
     * @throws std::invalid_argument If stream_arg has already failed - for
     * example, a file that couldn't be opened.
     */
    explicit istream_source(
        std::istream&     stream_arg,
        const bool        reads_without_waiting_arg,
        const std::size_t minimum_release_arg
        = current_pnp_options().text_source_minimum_release,
        const std::size_t minimum_block_size_arg
        = current_pnp_options().buffered_source_minimum_block_size,
        const std::size_t maximum_reused_block_size_arg
        = current_pnp_options().buffered_source_maximum_reused_block_size,
        const double minimum_read_fraction_arg
        = current_pnp_options().buffered_source_minimum_read_fraction,
        const double growth_factor_arg
        = current_pnp_options().buffered_source_growth_factor,
        const double compaction_fraction_arg
        = current_pnp_options().buffered_source_compaction_fraction
    );
private:
    /*!
     * @brief Reads the next piece of the stream into into_arg.
     *
     * See the class documentation for how much is read at once.
     *
     * @param into_arg The space to write the text to. Never empty.
     * @return A read_result with how many code units were written, and whether
     * the stream has more input to come.
     * @throws std::ios_base::failure If the stream reports an error.
     */
    read_result
        read_piece(const std::span<char8_t> into_arg) override;
    //! The stream read from.
    std::istream* _stream;
};

PNP_NS_END

PNP_NS_BEGIN

inline istream_source::istream_source(
    std::istream&     stream_arg,
    const bool        reads_without_waiting_arg,
    const std::size_t minimum_release_arg,
    const std::size_t minimum_block_size_arg,
    const std::size_t maximum_reused_block_size_arg,
    const double      minimum_read_fraction_arg,
    const double      growth_factor_arg,
    const double      compaction_fraction_arg
)
    : buffered_text_source(
          reads_without_waiting_arg,
          minimum_release_arg,
          minimum_block_size_arg,
          maximum_reused_block_size_arg,
          minimum_read_fraction_arg,
          growth_factor_arg,
          compaction_fraction_arg
      )
    , _stream(&stream_arg)
{
    if (_stream->fail())
    {
        throw std::invalid_argument(
            "istream_source: the stream has already failed - for example, a "
            "file that couldn't be opened."
        );
    }
}

inline buffered_text_source::read_result
    istream_source::read_piece(
        const std::span<char8_t> into_arg
    )
{
    using namespace std;
    char* const char_buffer = reinterpret_cast<char*>(into_arg.data());
    size_t      chars_read  = 0;
    if (reads_without_waiting())
    {
        // Nothing to wait for, so fill as much as there's room for.
        _stream->read(char_buffer, static_cast<streamsize>(into_arg.size()));
        chars_read = static_cast<size_t>(_stream->gcount());
    }
    else
    {
        // Wait for one code unit, then take only what has already arrived.
        if (_stream->get(char_buffer[0]))
        {
            chars_read = 1;
            if (into_arg.size() > 1)
            {
                chars_read += static_cast<size_t>(_stream->readsome(
                    char_buffer + 1,
                    static_cast<streamsize>(into_arg.size() - 1)
                ));
            }
        }
    }
    if (_stream->bad())
    {
        throw ios_base::failure(
            "istream_source: reading from the stream failed."
        );
    }
    return read_result{
        .code_units_written = chars_read,
        .more_content       = _stream->good(),
    };
}

PNP_NS_END