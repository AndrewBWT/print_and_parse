#pragma once
// Included for default options used by constructor.
#include "pnp/core/options.hpp"
// For text_location.
#include "pnp/core/parse/parser_result.hpp"
// For pnp namespaces.
#include "pnp/internal/macros.hpp"

// For debug asserts.
#include <cassert>
// For std::size_t.
#include <cstddef>
// For exceptions.
#include <stdexcept>
// u8string_view's used in many places.
#include <string_view>
// Used for Unicode functions.
#include <unicode_bridge.hpp>

PNP_NS_BEGIN

/*!
 * @brief A base type used to represent storage that must be kept in
 * memory while anything views it.
 *
 * A text_source that moves or frees its text derives its storage from this, and
 * passes it to text_source::update_text(). Each input_text "pins" (increments
 * the object's pins counter) the storage its text is in, and "unpins"
 * (decrements pins) it when the input_text object is destroyed, or given other
 * text.
 *
 * While pins is non-zero, the text_source must not free, reuse or move the
 * storage. Once storage that is no longer current has its pins variable equal
 * zero, it is freed using text_source::free_unpinned_storage().
 *
 * pins is updated by parser_input and input_text. A text_source should only
 * read it.
 */
struct pinnable_storage
{
    //! The number of input_text objects viewing this storage.
    std::size_t pins = 0;
};

/*!
 * @brief A source of text for a parser_input, read one piece at a time.
 *
 * A parser_input can be constructed from a string, when all of the text is
 * already in memory, or from a text_source, a generic interface for sources of
 * text that can be read in pieces - for example, from std::cin, a file, or a
 * network connection. Only the text a parser may still need is kept in memory,
 * so inputs much larger than the available memory can be parsed.
 *
 * A text_source:
 * - Holds the buffered text: text that has been read, but not yet released.
 *   offset() is the position in the whole input that it starts at. Releasing
 *   text drops it from the front of the buffered text once nothing needs it
 *   again; the source may then free or reuse the memory it used.
 * - Reads more text when asked to, through read_more().
 * - Checks that new text is valid UTF-8 as it arrives. Invalid UTF-8 is not an
 *   error. The valid text ends when invalid UTF-8 is found (see valid_size()
 *   and stopped_at_invalid()). parser_input throws invalid_utf8 if a parser
 *   reaches it. A character split across reads counts as valid once all of it
 *   has arrived.
 *
 * Objects derived from text_source are designed to be controlled by
 * parser_input, itself the interface for writing parsers. Only one parser_input
 * can read from a text_source at a time (see in_use()). Between parses,
 * release_up_to() can be used to discard text that has been consumed.
 *
 * pnp includes two abstract subclasses of text_source, which should be
 * sufficient for most needs. buffered_text_source reads pieces of text into
 * blocks of memory it manages. complete_text_source can be used for text that
 * is already in memory.
 *
 * If neither is suitable, derive your own class from
 * text_source directly. The following functions must have definitions
 * provided:
 * - fetch_more(). It reads more text into storage directly after the buffered
 *   text, then calls update_text() with the old and new text as one view.
 *   It returns true once text has been added, or false, adding nothing, once
 *   the input has ended. See fetch_more() for the details.
 * - free_storage(). It is called after text is released, so storage can be
 *   reused. An empty definition is correct for a source whose text never
 *   moves.
 *
 * If the text ever moves or is freed while the source exists, also:
 * - Derive the storage it lives in from pinnable_storage, and pass it to
 *   update_text(). input_text objects can then pin it while they view it.
 * - Override free_unpinned_storage(), which is called when storage that is no
 *   longer current loses its last pin. The default does nothing, which is
 *   correct for a source that only ever uses one storage object, or none.
 *
 * A source must follow these rules, so that every input_text stays valid:
 * - The buffered text only changes by having text added to its end, or by
 *   being released from its front. It may move to a new address, as long as
 *   update_text() is called straight after the move, before anything that
 *   may throw.
 * - Storage whose pinnable_storage::pins is non-zero must not be freed,
 *   reused or moved. New text can go into different storage locations instead;
 *   the old storage is freed in free_unpinned_storage() once its last pin is
 *   gone.
 * - Storage that is no longer current, and has no pins, can be freed straight
 *   away. parser_input re-reads buffered() after every read and release, so
 *   nothing else can be viewing it.
 *
 * Exceptions thrown by a derived class's fetch_more() or free_storage() pass
 * through unchanged, and are not listed on each function.
 */
struct text_source
{
public:
    // A source is the only owner of its input. Copying would allow two readers
    // to be reading from one input source.
    text_source(const text_source&) = delete;
    text_source&
        operator=(const text_source&)
        = delete;
    /*!
     * @brief Destructor.
     *
     * Every parser_input reading from the source, and so every input_text and
     * input_mark made from it, must have been destroyed before this is called.
     */
    virtual ~text_source();
    /*!
     * @brief Tells the caller whether a parser_input is reading from this
     * source.
     *
     * While one is, constructing another parser_input from this source throws
     * std::logic_error, as does calling release_up_to(). The source stops
     * being in use when that parser_input is destroyed.
     *
     * @return True if a parser_input is reading from this source. False
     * otherwise.
     */
    [[nodiscard]]
    bool
        in_use() const noexcept;
    /*!
     * @brief Returns the buffered text: text that has been read, but not yet
     * released.
     *
     * It starts at offset() in the whole input. It may end with part of a
     * character still arriving, or with invalid UTF-8; valid_size() gives how
     * much of it is valid.
     *
     * The view stays valid until the next call to read_more(), or until text is
     * released by release_up_to() or by a parser_input.
     *
     * @return The buffered text.
     */
    [[nodiscard]]
    std::u8string_view
        buffered() const noexcept;
    /*!
     * @brief Returns how many code units at the start of buffered() are valid
     * UTF-8.
     *
     * The valid text always ends on a character boundary. Anything after it is
     * either part of a character still arriving, or invalid UTF-8 (see
     * stopped_at_invalid()).
     *
     * @return The number of code units at the start of buffered() that are
     * valid UTF-8.
     */
    [[nodiscard]]
    std::size_t
        valid_size() const noexcept;
    /*!
     * @brief Tells the caller whether invalid UTF-8 has been found at
     * valid_size().
     *
     * Once this is true, the valid text never grows again. That includes the
     * input ending partway through a character, as that character can never be
     * completed.
     *
     * @return True if invalid UTF-8 has been found. False otherwise.
     */
    [[nodiscard]]
    bool
        stopped_at_invalid() const noexcept;
    /*!
     * @brief Tells the caller whether the input has ended.
     *
     * Once this is true, read_more() will always return false.
     *
     * @return True if the input has ended. False otherwise.
     */
    [[nodiscard]]
    bool
        exhausted() const noexcept;
    /*!
     * @brief Returns the position in the whole input that buffered() starts
     * at.
     *
     * This is the number of code units released so far.
     *
     * @return The position in the whole input of the first code unit of
     * buffered().
     */
    [[nodiscard]]
    std::size_t
        offset() const noexcept;
    /*!
     * @brief Returns the text_location of position_arg in the whole input.
     *
     * See text_location. The column is counted in Unicode characters, not code
     * units. Lines and columns are counted from the very start of the input,
     * including text that has already been released.
     *
     * Only buffered text can be looked at, so a position before offset() is
     * treated as offset(), and one past the end of buffered() as its end.
     *
     * @param position_arg A position in the whole input.
     * @return The text_location of position_arg.
     */
    [[nodiscard]]
    text_location
        location_at(const std::size_t position_arg) const;
    /*!
     * @brief Reads more of the input into buffered().
     *
     * Text added may not be valid yet - for example, the first part of a
     * character still arriving - so check valid_size() afterwards. Reading
     * continues after invalid UTF-8 is found, but valid_size() never grows
     * again.
     *
     * For a source that may wait for input, such as std::cin, this function
     * can block until at least one code unit arrives, or the input ends.
     *
     * Any view returned by buffered() before this call may be invalid after
     * it.
     *
     * @return True if text was added. False once the input has ended; after
     * that, always false.
     */
    bool
        read_more();
    /*!
     * @brief Releases the buffered text before position_arg.
     *
     * For use between parses, when no parser_input is reading from the source
     * - for example, a loop that parses one value at a time from the same
     * source. While a parser_input is reading, it releases text itself (see
     * parser_input::release_consumed()).
     *
     * Only valid text is released. A position before offset() releases
     * nothing; one past the valid text releases all of the valid text.
     * minimum_release() does not apply.
     *
     * Any view returned by buffered() before this call may be invalid after
     * it.
     *
     * @param position_arg The position in the whole input of the first code
     * unit to keep.
     * @throws std::logic_error If in_use() is true.
     */
    void
        release_up_to(const std::size_t position_arg);
    /*!
     * @brief Returns the minimum number of code units parser_input releases at
     * once.
     *
     * parser_input::release_consumed() waits until it can release at least
     * this many code units, and at least
     * parser_input::fraction_of_buffer_threshold() of the buffered text,
     * before releasing anything. Each release has a fixed cost, so releasing a
     * little at a time is slow. A larger value means fewer releases, but more
     * text held in memory.
     *
     * release_up_to() does not apply it.
     *
     * Set when the text_source is constructed. Defaults to
     * current_pnp_options().text_source_minimum_release.
     *
     * @return The minimum number of code units parser_input releases at once.
     */
    [[nodiscard]]
    std::size_t
        minimum_release() const noexcept;
    /*!
     * @brief Tells the caller whether more text can be read without waiting for
     * input.
     *
     * True for sources such as strings and files; false for sources such as
     * std::cin, pipes or network connections, where a read may wait for input
     * that hasn't arrived yet.
     *
     * parser_input::get_context() only reads more text, to show context after
     * a failure, when this is true, so that reporting an error never blocks.
     *
     * Set when the text_source is constructed.
     *
     * @return True if the source can read more text without waiting. False
     * otherwise.
     */
    [[nodiscard]]
    bool
        reads_without_waiting() const noexcept;
protected:
    /*!
     * @brief Constructor.
     *
     * The source starts with no buffered text, at offset 0. A derived class
     * that already holds its text can pass it to update_text() in its own
     * constructor.
     *
     * @param reads_without_waiting_arg See reads_without_waiting().
     * @param minimum_release_arg See minimum_release(). Defaults to
     * current_pnp_options().text_source_minimum_release.
     */
    explicit text_source(
        const bool        reads_without_waiting_arg,
        const std::size_t minimum_release_arg
        = current_pnp_options().text_source_minimum_release
    ) noexcept;
    /*!
     * @brief Sets the buffered text, and the storage it lives in.
     *
     * Call this after every read that adds text, and whenever the text moves.
     * If the text has moved, call it straight after the move, before anything
     * that may throw.
     *
     * text_arg must start with the same code units as buffered(), possibly at a
     * new address, followed by any new text. Only the new text is checked for
     * valid UTF-8.
     *
     * input_text objects made from now on pin storage_arg. Existing ones
     * keep the storage they already pin.
     *
     * @param text_arg The buffered text.
     * @param storage_arg The storage text_arg lives in, or nullptr if the text
     * is never moved or freed while the source exists (for example, a string
     * the source doesn't own), so nothing needs pinning.
     */
    constexpr void
        update_text(
            const std::u8string_view text_arg,
            pinnable_storage* const  storage_arg
        ) noexcept;
private:
    friend struct parser_input;
    friend struct input_text;
    /*!
     * @brief Reads more text, directly after the buffered text.
     *
     * Called by read_more(). The new text must follow the buffered text in
     * memory, so that the two form one view; move the buffered text first if
     * there isn't room after it. Then call update_text() with that view before
     * returning true.
     *
     * The text may be moved to new storage to make room, following the rules
     * in the text_source documentation. Storage with pins must be kept.
     *
     * If a read produces no text but the input hasn't ended, keep reading, so
     * that this only returns once text has been added, or there is none left.
     *
     * For a source that may wait for input, this function is where it waits.
     *
     * @return True once text has been added. False, without adding any text,
     * once the input has ended; it is then never called again.
     */
    virtual bool
        fetch_more()
        = 0;
    /*!
     * @brief Called after text has been released from the front of the
     * buffered text, so the source can reuse the space it held.
     *
     * Called by release_up_to(), and by parser_input as it releases text.
     * buffered() has already been updated, so it may now be empty.
     *
     * Storage with pins must not be freed, reused or moved. A source whose text
     * never moves has nothing to do here - an empty definition is correct.
     */
    virtual void
        free_storage()
        = 0;
    /*!
     * @brief Releases the first count_arg code units of the buffered text,
     * then calls free_storage().
     *
     * Called by release_up_to(), and by parser_input::release_consumed().
     * Only valid text is released, so count_arg is reduced to valid_size() if
     * it is larger.
     *
     * @param count_arg The number of code units to release.
     */
    void
        release_text(const std::size_t count_arg);
    /*!
     * @brief Called when storage that is no longer current loses its last pin,
     * so the source can free it.
     *
     * Only reached by sources that pass more than one storage object to
     * update_text() over their lifetime. The default does nothing, which is
     * correct for every other source.
     *
     * Called from input_text's destructor, so must not throw.
     *
     * @param storage_arg The storage that has no pins left.
     */
    virtual void
        free_unpinned_storage(pinnable_storage* const storage_arg) noexcept;
    //! See buffered().
    std::u8string_view _text;
    //! See offset().
    std::size_t _offset    = 0;
    //! The position in the whole input one past the last valid code unit. See
    //! valid_size().
    std::size_t _valid_end = 0;
    //! See stopped_at_invalid().
    bool _invalid          = false;
    //! See exhausted().
    bool _exhausted        = false;
    //! See reads_without_waiting().
    bool _reads_without_waiting;
    //! See minimum_release().
    std::size_t _minimum_release;
    //! See in_use().
    bool _in_use = false;
    //! The text_location of offset(), so location_at() can count on from it
    //! after text is released.
    text_location _offset_location;
    //! The storage that input_text objects made now pin, or nullptr if none is
    //! needed. Set by update_text().
    pinnable_storage* _current_storage = nullptr;
};

PNP_NS_END

PNP_NS_BEGIN

inline text_source::~text_source()
{
    assert(not _in_use);
}

inline bool
    text_source::in_use() const noexcept
{
    return _in_use;
}

inline std::u8string_view
    text_source::buffered() const noexcept
{
    return _text;
}

inline std::size_t
    text_source::valid_size() const noexcept
{
    return _valid_end - _offset;
}

inline bool
    text_source::stopped_at_invalid() const noexcept
{
    return _invalid;
}

inline bool
    text_source::exhausted() const noexcept
{
    return _exhausted;
}

inline std::size_t
    text_source::offset() const noexcept
{
    return _offset;
}

inline text_location
    text_source::location_at(
        const std::size_t position_arg
    ) const
{
    const std::size_t end_of_text = _offset + _text.size();
    const std::size_t position    = position_arg < _offset       ? _offset
                                    : position_arg > end_of_text ? end_of_text
                                                                 : position_arg;
    return _offset_location.after(_text.substr(0, position - _offset));
}

inline bool
    text_source::read_more()
{
    if (_exhausted)
    {
        return false;
    }
    if (fetch_more())
    {
        return true;
    }
    else
    {
        _exhausted = true;
        // A character cut off by the end of the input can never be completed.
        if (_valid_end < _offset + _text.size())
        {
            _invalid = true;
        }
        return false;
    }
}

inline void
    text_source::release_up_to(
        const std::size_t position_arg
    )
{
    if (_in_use)
    {
        throw std::logic_error(
            "text_source::release_up_to: a parser_input is reading from "
            "the source, and may still need the text"
        );
    }
    const std::size_t count
        = position_arg > _offset ? position_arg - _offset : 0;
    // No parser_input, so no views of the text can be in use.
    release_text(count);
}

inline std::size_t
    text_source::minimum_release() const noexcept
{
    return _minimum_release;
}

inline bool
    text_source::reads_without_waiting() const noexcept
{
    return _reads_without_waiting;
}

inline text_source::text_source(
    const bool        reads_without_waiting_arg,
    const std::size_t minimum_release_arg
) noexcept
    : _reads_without_waiting(reads_without_waiting_arg)
    , _minimum_release(minimum_release_arg)
{}

constexpr void
    text_source::update_text(
        const std::u8string_view text_arg,
        pinnable_storage* const  storage_arg
    ) noexcept
{
    using namespace std;
    using namespace UNICODE_BRIDGE_NAMESPACE;
    assert(text_arg.size() >= _text.size());
    _text                     = text_arg;
    _current_storage          = storage_arg;
    auto could_still_complete = [](const u8string_view rest_arg) noexcept
    {
        const size_t needed_code_units
            = UNICODE_BRIDGE_NAMESPACE::UNICODE_BRIDGE_NAMESPACE_INTERNAL::
                utf8_sequence_length(rest_arg[0]);
        if (needed_code_units <= 1 || rest_arg.size() >= needed_code_units)
        {
            return false;
        }
        for (size_t idx = 1; idx < rest_arg.size(); ++idx)
        {
            if (not UNICODE_BRIDGE_NAMESPACE::
                    UNICODE_BRIDGE_NAMESPACE_INTERNAL::
                        is_utf8_continuation_byte(rest_arg[idx]))
            {
                return false;
            }
        }
        return true;
    };
    // If invalid UTF-8 text is found previously, function does no work.
    if (_invalid)
    {
        return;
    }
    // Only count the new text - that past _valid_end.
    const u8string_view unscanned_text  = _text.substr(_valid_end - _offset);
    auto                valid_length    = validate_unicode(unscanned_text);
    // If no code points were invalid, valid_length gives
    // unscanned_text.size()
    _valid_end                         += valid_length;
    // Rest is anything left over.
    const u8string_view rest            = unscanned_text.substr(valid_length);
    // Check if rest COULD be the start of a valid character. If it definitely
    // can't, then what we have is definitely invalid.
    _invalid = not rest.empty() && not could_still_complete(rest);
}

inline void
    text_source::release_text(
        const std::size_t count_arg
    )
{
    // Normalise count.
    const std::size_t local_count
        = count_arg < valid_size() ? count_arg : valid_size();
    // Move the location up.
    _offset_location = _offset_location.after(_text.substr(0, local_count));
    // Moves the string view forward by the count given.
    _text.remove_prefix(local_count);
    // Updates the offset.
    _offset += local_count;
    // Then frees storage.
    free_storage();
}

inline void
    text_source::free_unpinned_storage(
        pinnable_storage* const
    ) noexcept
{}

PNP_NS_END