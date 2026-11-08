#pragma once
#include "pnp/core/options.hpp"
#include "pnp/core/parse/text_source.hpp"
#include "pnp/internal/macros.hpp"

#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>
// For char_traits
#include <string>

PNP_NS_BEGIN

/*!
 * @brief A text_source that reads its input a piece at a time, into memory it
 * manages.
 *
 * buffered_text_source takes care of the text_source rules about memory. It
 * allocates blocks for the text, moves the buffered text when a block runs out
 * of room, keeps blocks that input_text objects still pin, and frees or reuses
 * them once they don't. A derived class only has to provide the functionality
 * to read text.
 *
 * To write a buffered_text_source, derive from it, pass its constructor whether
 * the source can read without waiting, and implement read_piece().
 * read_piece() writes the next piece of text into the space it is given, and
 * returns two things: how many code units it wrote, and whether there is more
 * input to come. It doesn't have to deal with UTF-8: a piece can end anywhere,
 * even partway through a character.
 *
 * How blocks are sized and reused can be tuned through the constructor; see
 * minimum_block_size(), maximum_reused_block_size(), minimum_read_size(),
 * growth_factor() and compaction_fraction(). The defaults are from
 * current_pnp_options().
 *
 * Any function that allocates may throw std::bad_alloc, including the
 * constructor. These exceptions are not listed on each function. If a block
 * would be too large to represent, read_more() throws std::length_error.
 * Exceptions thrown by read_piece() pass through read_more() unchanged.
 */
struct buffered_text_source : public text_source
{
public:
    /*!
     * @brief Destructor.
     *
     * Checks, in debug builds, that no input_text still pins any of its
     * blocks.
     */
    ~buffered_text_source() override;
    /*!
     * @brief Returns the smallest block of memory allocated for text, in code
     * units.
     *
     * Each new block allocated is sized from the text it must hold and
     * growth_factor(), but is never smaller than this value.
     *
     * 0 is allowed; each new block is at least one code unit larger than the
     * text it must hold.
     *
     * Set when the buffered_text_source is constructed. Defaults to
     * current_pnp_options().buffered_source_minimum_block_size.
     *
     * @return The minimum block size, in code units.
     */
    constexpr std::size_t
        minimum_block_size() const noexcept;
    /*!
     * @brief Returns the size of the largest block kept for reuse once all its
     * text has been released.
     *
     * A block grows when a parser needs a lot of text kept at once - for
     * example, holding an input_mark while a very long value is read. Once all
     * that text is released from a block, if that block is larger than this
     * value it will be freed, and the next read allocates one of
     * minimum_block_size() instead, so the large memory block isn't kept for
     * the rest of the input. Any block of this size or smaller is reused.
     *
     * If this value is smaller than minimum_block_size(), no block is ever
     * reused. Each is freed once its text has been released, and the next read
     * allocates a new one.
     *
     * Set when the buffered_text_source is constructed. Defaults to
     * current_pnp_options().buffered_source_maximum_reused_block_size.
     *
     * @return The maximum reused block size, in code units.
     */
    constexpr std::size_t
        maximum_reused_block_size() const noexcept;
    /*!
     * @brief Returns the minimum free space each new read is given, in code
     * units.
     *
     * Before each call to read_piece(), if the current block has less free
     * space than this after the buffered text, room is made available - either
     * by moving the buffered text to new memory, or by moving the buffered text
     * to the front of the block (see compaction_fraction()).
     *
     * A smaller value means the text is moved less frequently, but read_piece()
     * may be given less space, so it is called more often.
     *
     * It is the minimum read fraction given to the constructor multiplied by
     * minimum_block_size(), but never more than minimum_block_size(), and never
     * less than 1, so read_piece() is never given an empty span.
     *
     * Set when the buffered_text_source is constructed. The fraction defaults
     * to current_pnp_options().buffered_source_minimum_read_fraction.
     *
     * @return The minimum read size, in code units.
     */
    constexpr std::size_t
        minimum_read_size() const noexcept;
    /*!
     * @brief Returns how much larger than the space needed each new block is
     * made.
     *
     * When the text is moved to a new block, the space required is calculated
     * as the buffered text size plus minimum_read_size(). The new block is the
     * space required multiplied by the growth factor, always at least one code
     * unit larger than needed, and never smaller than minimum_block_size().
     *
     * A larger growth factor means fewer moves while a long value is read, but
     * more unused memory. At 1 or below, each block is only just large enough,
     * which is correct but slow, as the text may have to be moved on almost
     * every read.
     *
     * The value given to the constructor is normalised to a finite number of
     * at least 0. Defaults to
     * current_pnp_options().buffered_source_growth_factor.
     *
     * @return The growth factor.
     */
    constexpr double
        growth_factor() const noexcept;
    /*!
     * @brief Returns the compaction fraction.
     *
     * When the current block runs out of room, if no input_text holds a pin to
     * it, and the buffered text fills at most this fraction of it, the buffered
     * text is moved to the front of the block and the block is reused, instead
     * of moving the text to a new block. This avoids allocating a new block
     * when the current one has enough released space to reuse.
     *
     * A higher value allocates less often, but copies more text each time. At
     * 0, a block is only reused in place once none of its text is left.
     *
     * The value given to the constructor is normalised to [0, 1). Defaults to
     * current_pnp_options().buffered_source_compaction_fraction.
     *
     * @return The compaction fraction.
     */
    constexpr double
        compaction_fraction() const noexcept;
protected:
    /*!
     * @brief The result type of a call to read_piece().
     */
    struct read_result
    {
        //! How many code units were written to the start of the span.
        std::size_t code_units_written = 0;
        //! False if the input has ended, true if there is more content. Text
        //! may be returned with false.
        bool more_content              = true;
    };

    /*!
     * @brief Constructor.
     *
     * @param reads_without_waiting_arg See
     * text_source::reads_without_waiting().
     * @param minimum_release_arg See text_source::minimum_release(). Defaults
     * to current_pnp_options().text_source_minimum_release.
     * @param minimum_block_size_arg See minimum_block_size(). Defaults to
     * current_pnp_options().buffered_source_minimum_block_size.
     * @param maximum_reused_block_size_arg See maximum_reused_block_size().
     * Defaults to
     * current_pnp_options().buffered_source_maximum_reused_block_size.
     * @param minimum_read_fraction_arg The least free space each read is given,
     * as a fraction of minimum_block_size_arg; see minimum_read_size().
     * Anything that isn't a finite number of at least 0 is treated as 0.
     * Defaults to current_pnp_options().buffered_source_minimum_read_fraction.
     * @param growth_factor_arg See growth_factor(). Anything that isn't a
     * finite number of at least 0 is treated as 0. Defaults to
     * current_pnp_options().buffered_source_growth_factor.
     * @param compaction_fraction_arg See compaction_fraction(). Values outside
     * [0, 1), including NaN, are treated as 0. Defaults to
     * current_pnp_options().buffered_source_compaction_fraction.
     */
    explicit buffered_text_source(
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
     * @brief Reads the next piece of text into into_arg.
     *
     * Writes at most into_arg.size() code units to the start of into_arg, and
     * returns how many were written. The text may end anywhere, even partway
     * through a UTF-8 character.
     *
     * A read may write nothing without the input having ended: return 0 code
     * units with more_content true, and read_piece() will be called again. The
     * last of the text can be returned together with more_content=false. Once
     * more_content=false has been returned, read_piece() is never called
     * again.
     *
     * For a source that may wait for input, such as std::cin, this function is
     * where it waits. Return as soon as any text is available, rather than
     * waiting to fill into_arg, so that parsing interactive input doesn't
     * stall.
     *
     * If read_piece() throws, the exception passes out of read_more(). The text
     * already read is unchanged, and anything written to into_arg is ignored.
     *
     * @param into_arg The space to write the text to. Never empty.
     * @return A read_result, showing how many code units were written, and
     * whether there is more text.
     */
    virtual read_result
        read_piece(const std::span<char8_t> into_arg)
        = 0;
    /*!
     * @brief Makes sure _block has at least required_size_arg code units of
     * free space.
     *
     * The buffered text (text not yet released - see buffered()) sits at the
     * end of _block's used part, after any released text.
     *
     * If no input_text has a pin in _block, the buffered text is at most
     * compaction_fraction() of _block, and moving it frees enough room to hold
     * required_size_arg code units, the buffered text is moved to the start of
     * _block, and the block is reused.
     *
     * Otherwise, a new block is made, holding the buffered text with room for
     * required_size_arg more, with its size multiplied by the growth factor. It
     * is at least one code unit larger than needed and at least the minimum
     * block size. If an input_text still pins the old block, it is retired
     * until its last pin goes; otherwise its buffer is freed.
     *
     * If anything throws an exception, nothing is changed.
     *
     * @param required_size_arg The minimum amount of room to make free in
     * _block.
     * @throws std::length_error If the new block's size can't be represented.
     */
    void
        make_room(const std::size_t required_size_arg);
    /*!
     * @brief Fetches more text through read_piece(), making room first if
     * required.
     *
     * Calls read_piece() until it writes some text, or reports that the input
     * has ended. See text_source::fetch_more().
     *
     * @return True if text was added. False once the input has ended.
     */
    bool
        fetch_more() final;
    /*!
     * @brief Reuses or frees the current block's buffer once all of its text
     * has been released.
     *
     * Called after every release; does nothing while any buffered text is
     * left, or while an input_text pins the block. Otherwise the buffer is
     * reused from its start for the next read, or freed if it is larger than
     * maximum_reused_block_size(), so the next read allocates a new one.
     */
    void
        free_storage() final;
    /*!
     * @brief Frees a retired block once its last pin has been removed.
     * @param storage_arg The block to be freed.
     */
    void
        free_unpinned_storage(pinnable_storage* const storage_arg
        ) noexcept final;

    /*!
     * @brief A block of memory holding text, which input_text objects can pin.
     *
     * It can be seen as a very simple string type.
     */
    struct storage_block : pinnable_storage
    {
        //! The text itself.
        std::unique_ptr<char8_t[]> _data;
        //! The size of _data.
        std::size_t _capacity = 0;
    };

    //! The block new text is written to. Never null; its buffer may be.
    std::unique_ptr<storage_block> _block;
    //! How much of _block is used, including released text at the front.
    std::size_t _used = 0;
    //! Old blocks still pinned by input_text objects. Each is freed when its
    //! last pin goes.
    std::vector<std::unique_ptr<storage_block>> _retired;
    //! Whether read_piece() has reported the end of the input.
    bool _input_ended = false;
    //! See minimum_block_size().
    std::size_t _minimum_block_size;
    //! See maximum_reused_block_size().
    std::size_t _maximum_reused_block_size;
    //! See minimum_read_size().
    std::size_t _minimum_read_size;
    //! See growth_factor().
    double _growth_factor;
    //! See compaction_fraction().
    double _compaction_fraction;
};

PNP_INTERNAL_NS_BEGIN
/*!
 * @brief Helper function to calculate the minimum_read_size value from the
 * given minimum_block_size and fraction minimum_read_fraction.
 *
 * Roughly equal to minimum_read_fraction_arg*minimum_block_size_arg. Result
 * is capped at the minimum block size, and is at least 1.
 *
 * @param minimum_block_size_arg Minimum block size.
 * @param minimum_read_fraction_arg Minimum read fraction.
 * @return The minimum read size calculated from the arguments.
 */
std::size_t
    minimum_read_size(
        const std::size_t minimum_block_size_arg,
        const double      minimum_read_fraction_arg
    ) noexcept;
PNP_INTERNAL_NS_END
PNP_NS_END

PNP_NS_BEGIN
inline buffered_text_source::~buffered_text_source()
{
    assert(_retired.empty() && _block->pins == 0);
}

constexpr std::size_t
    buffered_text_source::minimum_block_size() const noexcept
{
    return _minimum_block_size;
}

constexpr std::size_t
    buffered_text_source::maximum_reused_block_size() const noexcept
{
    return _maximum_reused_block_size;
}

constexpr std::size_t
    buffered_text_source::minimum_read_size() const noexcept
{
    return _minimum_read_size;
}

constexpr double
    buffered_text_source::growth_factor() const noexcept
{
    return _growth_factor;
}

constexpr double
    buffered_text_source::compaction_fraction() const noexcept
{
    return _compaction_fraction;
}

inline buffered_text_source::buffered_text_source(
    const bool        reads_without_waiting_arg,
    const std::size_t minimum_release_arg,
    const std::size_t minimum_block_size_arg,
    const std::size_t maximum_reused_block_size_arg,
    const double      minimum_read_fraction_arg,
    const double      growth_factor_arg,
    const double      compaction_fraction_arg
)
    : text_source(reads_without_waiting_arg, minimum_release_arg)
    , _block(std::make_unique<storage_block>())
    , _minimum_block_size(minimum_block_size_arg)
    , _maximum_reused_block_size(maximum_reused_block_size_arg)
    , _minimum_read_size(PNP_INTERNAL_NAMESPACE::minimum_read_size(
          minimum_block_size_arg,
          minimum_read_fraction_arg
      ))
    , _growth_factor(
          PNP_INTERNAL_NAMESPACE::clamp_non_negative_finite(growth_factor_arg)
      )
    , _compaction_fraction(
          PNP_INTERNAL_NAMESPACE::clamp_fraction(compaction_fraction_arg)
      )
{}

inline void
    buffered_text_source::make_room(
        const std::size_t required_size_arg
    )
{
    using namespace std;
    using traits              = char_traits<char8_t>;
    constexpr size_t max_size = numeric_limits<size_t>::max();
    if (_block->_capacity - _used >= required_size_arg)
    {
        // We have the required room.
        return;
    }
    const size_t live_text_size = buffered().size();
    // If nothing views the block, and moving the live text to the front frees
    // enough room, compact in place instead of allocating. Only done when the
    // live text is at most compaction_fraction() of the block, so each code
    // unit is moved a bounded number of times.
    if (_block->pins == 0 && required_size_arg <= _block->_capacity
        && live_text_size <= _block->_capacity - required_size_arg
        && static_cast<double>(live_text_size)
               <= _compaction_fraction * static_cast<double>(_block->_capacity))
    {
        if (live_text_size != 0)
        {
            traits::move(
                _block->_data.get(), buffered().data(), live_text_size
            );
        }
        _used = live_text_size;
        update_text(
            u8string_view(_block->_data.get(), live_text_size), _block.get()
        );
        return;
    }
    // basically we can't represent the block.
    if (required_size_arg >= max_size - live_text_size)
    {
        throw length_error(
            "buffered_text_source: the block needed is too large to represent."
        );
    }
    const size_t needed_size = live_text_size + required_size_arg;
    // Calculate the new capacity of the block.
    const double grown_as_double
        = _growth_factor * static_cast<double>(needed_size);
    // Checks it can actually fit into a size_t.
    if (not (grown_as_double < static_cast<double>(max_size)))
    {
        throw length_error(
            "buffered_text_source: the block size from growth_factor is too "
            "large to represent."
        );
    }
    const auto grown = static_cast<size_t>(grown_as_double);
    // At least one more than needed, however close to 1 the factor is.
    const size_t at_least_needed
        = grown > needed_size ? grown : needed_size + 1;
    const size_t new_capacity = at_least_needed > minimum_block_size()
                                    ? at_least_needed
                                    : minimum_block_size();
    // Everything that can throw happens before anything is changed.
    auto new_data = make_unique_for_overwrite<char8_t[]>(new_capacity);
    const u8string_view kept_text = buffered();
    if (not kept_text.empty())
    {
        traits::copy(new_data.get(), kept_text.data(), kept_text.size());
    }
    if (_block->pins != 0)
    {
        // Views still point into the current block, so it is retired, and a
        // new block object takes its place.
        auto replacement = make_unique<storage_block>();
        _retired.reserve(_retired.size() + 1);
        _retired.push_back(std::move(_block));
        _block = std::move(replacement);
    }
    // Any unpinned old buffer is freed here, as it's replaced.
    _block->_data     = std::move(new_data);
    _block->_capacity = new_capacity;
    _used             = live_text_size;
    // The same text, now in the new block.
    update_text(
        u8string_view(_block->_data.get(), live_text_size), _block.get()
    );
}

inline bool
    buffered_text_source::fetch_more()
{
    using namespace std;
    if (_input_ended)
    {
        return false;
    }
    while (true)
    {
        make_room(minimum_read_size());
        const size_t live_text_size = buffered().size();
        // Put the free space into a span.
        const span<char8_t> free_space_span(
            _block->_data.get() + _used, _block->_capacity - _used
        );
        const read_result read_result_var = read_piece(free_space_span);
        assert(read_result_var.code_units_written <= free_space_span.size());
        if (not read_result_var.more_content)
        {
            _input_ended = true;
        }
        if (read_result_var.code_units_written > 0)
        {
            // Update the internal structures.
            _used += read_result_var.code_units_written;
            update_text(
                u8string_view(
                    _block->_data.get() + _used - live_text_size
                        - read_result_var.code_units_written,
                    live_text_size + read_result_var.code_units_written
                ),
                _block.get()
            );
            return true;
        }
        if (_input_ended)
        {
            return false;
        }
    }
}

inline void
    buffered_text_source::free_storage()
{
    using namespace std;
    // Text or views still need the block, so leave it.
    if (not buffered().empty() || _block->pins != 0)
    {
        return;
    }
    if (_block->_capacity > maximum_reused_block_size())
    {
        // Frees the buffer.
        _block->_data.reset();
        _block->_capacity = 0;
    }
    _used = 0;
}

inline void
    buffered_text_source::free_unpinned_storage(
        pinnable_storage* const storage_arg
    ) noexcept
{
    auto* const block = static_cast<storage_block*>(storage_arg);
    assert(block != _block.get() && block->pins == 0);
    for (auto itt = _retired.begin(); itt != _retired.end(); ++itt)
    {
        if (itt->get() == block)
        {
            _retired.erase(itt);
            return;
        }
    }
    assert(
        false && "buffered_text_source::free_unpinned_storage: unknown block"
    );
}

PNP_INTERNAL_NS_BEGIN
inline std::size_t
    minimum_read_size(
        const std::size_t minimum_block_size_arg,
        const double      minimum_read_fraction_arg
    ) noexcept
{
    using namespace std;
    const double normalised_fraction
        = clamp_non_negative_finite(minimum_read_fraction_arg);
    const double block_size  = static_cast<double>(minimum_block_size_arg);
    const double scaled_size = normalised_fraction * block_size;
    // Ensure that value fits in size_t.
    const size_t read_size   = scaled_size < block_size
                                   ? static_cast<size_t>(scaled_size)
                                   : minimum_block_size_arg;
    return read_size > 0 ? read_size : 1;
}

PNP_INTERNAL_NS_END
PNP_NS_END