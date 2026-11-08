#pragma once
#include "pnp/core/options.hpp"
#include "pnp/core/parse/parser_result.hpp"
#include "pnp/core/parse/text_source.hpp"
#include "pnp/internal/macros.hpp"

#include <cassert>
#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <optional>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unicode_bridge.hpp>
#include <utility>
#include <vector>

PNP_NS_BEGIN
// Forward declarations.
struct parser_input;
struct input_mark;
struct input_text;
struct pinnable_storage;

/*!
 * @brief A saved position in a parser_input's text.
 *
 * Created by parser_input::mark(). While an input_mark exists, text from its
 * position onwards is never released, so parser_input::reset() can be called to
 * return to it - for example, to try another alternative in a parser after one
 * fails. Once an input_mark is destroyed, that text can be released by a later
 * release_consumed().
 *
 * An input_mark can be moved but not copied, and must not outlive the
 * parser_input it came from. Keep it as a local variable.
 */
struct input_mark
{
public:
    /*!
     * @brief Destructor. Informs the input_mark's owning parser_input that the
     * text from this input_mark's position onwards can be released.
     */
    constexpr ~input_mark() noexcept;
    // input_marks can't be copied, as each one holds a single position in its
    // owning parser_input.
    input_mark(const input_mark&) = delete;
    input_mark&
        operator=(const input_mark&)
        = delete;
    /*!
     * @brief Move constructor. Takes over other_arg's position; other_arg is
     * left with no position, and calling parser_input::reset() using it would
     * throw invalid_parser_foreign_mark.
     * @param other_arg The input_mark being moved from.
     */
    constexpr input_mark(input_mark&& other_arg) noexcept;
    /*!
     * @brief Move assignment. Frees this input_mark's position with its
     * owning parser_input, then takes over other_arg's position. other_arg
     * is left holding no position.
     * @param other_arg The input_mark being moved from.
     * @return *this.
     */
    constexpr input_mark&
        operator=(input_mark&& other_arg) noexcept;
private:
    friend struct parser_input;
    /*!
     * @brief Creates an input_mark at input_arg's current position, and informs
     * input_arg to not release text from that position onwards.
     * @param input_arg The parser_input used to construct this object.
     * @param source_location_arg Where the mark was created, for error
     * messages.
     */
    constexpr input_mark(
        parser_input&               input_arg,
        const std::source_location& source_location_arg
    );
    /*!
     * @brief Gets the parser_input this input_mark belongs to.
     * @return The owning parser_input, or nullptr if the mark has been moved
     * from.
     */
    [[nodiscard]]
    constexpr const parser_input*
        input() const noexcept;
    /*!
     * @brief Gets the saved position of the input_mark.
     * @return The position in the whole input this input_mark holds.
     */
    [[nodiscard]]
    constexpr std::size_t
        position() const noexcept;
    /*!
     * @brief Removes its holding position from the owning parser_input, if
     * there is one.
     *
     * Used by the destructor and the move assignment operator.
     */
    constexpr void
        let_go() noexcept;
    //! The parser_input this input_mark belongs to. nullptr once moved from.
    parser_input* _input  = nullptr;
    //! The position in the whole input that this mark holds.
    std::size_t _position = 0;
    //! Where the input_mark was created, reported if it's misused.
    std::source_location _source_location;
};

/*!
 * @brief A wrapper around a string view of some text.
 *
 * While the input_text exists, its text is kept in memory. The input_text
 * object must not outlive the parser_input it came from - as it cannot be
 * guaranteed that the data the string view is based on will continue to exist.
 * To keep the text longer, copy it into a std::u8string.
 *
 * Returned by parser_input's peek functions. get() stays valid while this
 * object lives, even if the parser_input reads more text or releases what
 * it has consumed. It keeps the text's memory, not its position: it does not
 * stop release_consumed, and you can't reset to it. Use an input_mark for that
 * functionality.
 */
struct input_text
{
public:
    // Copy constructor and copy assignment operator deleted.
    input_text(const input_text&) = delete;
    input_text&
        operator=(const input_text&)
        = delete;
    /*!
     * @brief Move constructor.
     *
     * Takes the other_arg's text. other_arg is left empty.
     *
     * @param other_arg The input_text being moved from.
     */
    constexpr input_text(input_text&& other_arg) noexcept;
    /*!
     * @brief Move assignment.
     *
     * Releases this object's text, then takes the other_arg's text. other_arg
     * is left empty.
     *
     * @param other_arg The input_text being moved from.
     * @return *this.
     */
    constexpr input_text&
        operator=(input_text&& other_arg) noexcept;
    /*!
     * @brief Destructor. Releases the text, which may then be freed.
     */
    constexpr ~input_text() noexcept;
    /*!
     * @brief Returns the underlying std::u8string_view object.
     *
     * The view is only valid while this object exists; it is empty if the
     * object has been moved from.
     *
     * @return The underlying std::u8string_view this object holds.
     */
    [[nodiscard]]
    constexpr std::u8string_view
        get() const noexcept;
private:
    friend struct parser_input;
    /*!
     * @brief Constructor for input_text.
     * @param text_arg String the input_text refers to.
     * @param text_source_arg The text_source this input_text was created from.
     * @param text_storage_arg A pointer to the pinnable_storage this object
     * holds.
     */
    constexpr input_text(
        const std::u8string_view text_arg,
        text_source*             text_source_arg,
        pinnable_storage*        text_storage_arg
    ) noexcept;
    /*!
     * @brief Unpins the storage. If it is no longer the source's current
     * storage and has no pins left, the source frees it.
     */
    constexpr void
        release() noexcept;
    //! The string_view the input_text contains.
    std::u8string_view _text;
    //! The text source that controls _text.
    text_source* _text_source       = nullptr;
    //! The storage that _text is contained in.
    pinnable_storage* _text_storage = nullptr;
};

/*!
 * @brief The core interface used by a parser to read input text.
 *
 * The text is internally defined as either a fixed string - so all input
 * is available - or some abstract source (a "text_source") that can be read a
 * piece at a time. parser_input only reads more information from a text_source
 * when it cannot decide how to proceed without it.
 *
 * UTF-8 validation is performed on the text as it arrives. All public functions
 * that return text are guaranteed to be valid UTF-8. Only when a request is
 * made of parser_input for text it knows is invalid will it raise an
 * invalid_utf8 exception.
 *
 * A parser that asks for something impossible (moving past the end or into a
 * character, a predicate that splits a character, a bad escape, or a foreign
 * mark) gets a parse_exception with an invalid_parser_ code, which parse()
 * reports as a failure. If parser_input finds its own state inconsistent, a bug
 * in pnp itself, it throws std::logic_error instead, so that parse() doesn't
 * mistake it for bad input. The text_source-based constructor to parser_input
 * will also throw a std::logic_error if the text_source is already in use.
 *
 * The function docs below refer to the code inside parse_exception, rather
 * than the parse_exception itself. e.g. "throws invalid_utf8" means a
 * parse_exception whose code is invalid_utf8.
 *
 * input_text objects, and the functions that return them, provide the core
 * functionality for writing parsers. Each ensures that parser_input's
 * underlying source keeps its text in memory until the input_text is destroyed.
 * input_mark objects keep a position, so the parser can reset to it.
 *
 * input_mark and input_text objects must not outlive the parser_input they
 * came from. To keep text longer, copy it into a std::u8string.
 *
 * Any function that allocates may throw std::bad_alloc, and exceptions
 * thrown by user-supplied code (predicates, parsers, text_source
 * implementations) pass through unchanged. These are not listed on each
 * function; only exceptions pnp throws itself are.
 */
struct parser_input
{
public:
    constexpr parser_input() = delete;
    /*!
     * @brief Creates a parser_input from a u8string_view.
     *
     * The text the view points to must outlive the parser_input.
     *
     * The whole string is validated as UTF-8 here, in one pass. Invalid UTF-8
     * does not stop construction; invalid_utf8 is thrown when a parser reaches
     * it.
     *
     * @param str_arg The text to parse.
     * @param context_chars_arg See context_chars(). Defaults to
     * current_pnp_options().parser_input_context_chars.
     */
    explicit constexpr parser_input(
        const std::u8string_view str_arg,
        const std::size_t        context_chars_arg
        = current_pnp_options().parser_input_context_chars
    ) noexcept;
    /*!
     * @brief Creates a parser_input from a string_view.
     *
     * The text the view points to must outlive the parser_input.
     *
     * The text is read as UTF-8, whatever the system's locale.
     *
     * The whole string is validated as UTF-8 here, in one pass. Invalid UTF-8
     * does not stop construction; invalid_utf8 is thrown when a parser reaches
     * it.
     *
     * @param str_arg The text to parse.
     * @param context_chars_arg See context_chars(). Defaults to
     * current_pnp_options().parser_input_context_chars.
     */
    explicit parser_input(
        const std::string_view str_arg,
        const std::size_t      context_chars_arg
        = current_pnp_options().parser_input_context_chars
    ) noexcept;
    /*!
     * @brief Creates a parser_input object from a text_source.
     *
     * The text is read from the source a piece at a time, as parsers need it,
     * and released once no longer needed, so large inputs can be parsed
     * without holding all of them in memory.
     *
     * The text_source must outlive the parser_input. Only one parser_input can
     * read from a text_source at a time; the text_source is in use until this
     * parser_input is destroyed. Construction reads no text, so it never
     * blocks.
     *
     * @param source_arg The text_source.
     * @param context_chars_arg See context_chars(). Defaults to
     * current_pnp_options().parser_input_context_chars.
     * @param fraction_of_buffer_threshold_arg See
     * fraction_of_buffer_threshold(). Values outside [0, 1), including NaN,
     * are treated as 0. Defaults to
     * current_pnp_options().parser_input_fraction_of_buffer_threshold.
     * @throws std::logic_error If another parser_input is already reading from
     * source_arg.
     */
    explicit parser_input(
        text_source&      source_arg,
        const std::size_t context_chars_arg
        = current_pnp_options().parser_input_context_chars,
        const double fraction_of_buffer_threshold_arg
        = current_pnp_options().parser_input_fraction_of_buffer_threshold
    );
    // input_marks point to their parser_input, and a text_source can only be
    // read by one parser_input at a time, so parser_input can be neither
    // copied nor moved.
    parser_input(const parser_input&) = delete;
    parser_input&
        operator=(const parser_input&)
        = delete;
    parser_input(parser_input&&) = delete;
    parser_input&
        operator=(parser_input&&)
        = delete;
    /*!
     * @brief Destructor.
     *
     * If constructed with a text_source, releases it, allowing it to be used by
     * another parser_input. All input_marks and input_text objects must have
     * been destroyed before this is called.
     */
    constexpr ~parser_input();
    /*!
     * @brief Tells the caller whether the object is at the end of the text.
     *
     * This function does not move the position that the parser_input points to
     * in its underlying text.
     *
     * When writing parsers, be careful if using this function - specifically
     * when ending a construct by it. For example, search for an ending "]" when
     * parsing a list, instead of using at_end().
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until at least one
     * more code unit arrives, or the input ends. It does not check that the
     * text is valid UTF-8, so it never throws invalid_utf8.
     *
     * @return True if the object is at the end of the text; false otherwise.
     */
    [[nodiscard]]
    constexpr bool
        at_end();
    /*!
     * @brief Returns the current position in the whole input, in code units.
     *
     * Counts from the start of the input, including any text already
     * released, so it can be stored and passed to fail(), location_at() or
     * get_context() later.
     *
     * @return The current position in the parser_input's underlying text.
     */
    [[nodiscard]]
    constexpr std::size_t
        position() const noexcept;
    /*!
     * @brief Checks that the parser_input's text at the current position
     * starts with the string token_arg.
     *
     * For example starts_with(u8"ab") will match "abc", but not "ac". See
     * starts_with(std::initializer_list<std::u8string_view>), which checks
     * several strings one after another, and advance_if_starts_with(), which
     * advances a parser_input's position if the string matches. An empty
     * token_arg always matches.
     *
     * This function does not move the position that the parser_input points to
     * in its underlying text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives to decide whether there is a match, or the input ends.
     *
     * This function stops at the first code unit that differs, so it reads no
     * further than it needs to.
     *
     * @param token_arg The string to check the current text against.
     * @return True if the current position in the text matches the given
     * argument. False otherwise, or if the input ends before token_arg can
     * be checked.
     * @throws invalid_utf8 If invalid UTF-8 is reached.
     */
    [[nodiscard]]
    constexpr bool
        starts_with(const std::u8string_view token_arg);
    /*!
     * @brief Checks that the parser_input's text at the current position
     * starts with the strings in tokens_arg, one after another.
     *
     * For example starts_with({u8"a",u8"b"}) will match "ab", but not "ac".
     * This is the same as starts_with() on the joined strings, without having
     * to join them. An empty tokens_arg always matches.
     *
     * This function does not move the position that the parser_input points to
     * in its underlying text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives to decide whether there is a match, or the input ends.
     *
     * This function stops at the first code unit that differs, so it reads no
     * further than it needs to.
     *
     * @param tokens_arg The strings to check the current text against.
     * @return True if the current position in the text matches the given
     * arguments. False otherwise, or if the input ends before all strings can
     * be checked.
     * @throws invalid_utf8 If invalid UTF-8 is reached.
     */
    [[nodiscard]]
    constexpr bool
        starts_with(const std::initializer_list<std::u8string_view> tokens_arg);
    /*!
     * @brief Returns an input_text, containing a string view from the current
     * position to (not including) the position of the first code unit that
     * fails predicate_arg.
     *
     * advance_while() is the same, but moves the position and doesn't
     * return the input_text object.
     *
     * The predicate function takes a single code unit (char8_t) as input.
     * peek_chars_while() is a similar function that uses a predicate that
     * checks char32_t values.
     *
     * The predicate must accept or reject all the code units of a character
     * together, so the returned text always ends on a character boundary.
     *
     * This function does not move the position that the parser_input points to
     * in its underlying text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until it finds a
     * code unit that fails the predicate, or the input ends.
     *
     * If the input ends before a code unit that fails the predicate is found,
     * an input_text representing all input from the current position to the end
     * of the text is returned.
     *
     * This function stops at the first code unit that does not satisfy the
     * predicate, so it reads no further than it needs to.
     *
     * @tparam Predicate The function type of predicate_arg.
     * @param predicate_arg A function that takes a char8_t and returns a bool.
     * @return An input_text holding the text from the current position up
     * to (not including) the first code unit that fails predicate_arg.
     * @throws invalid_utf8 If an attempt is made to read invalid UTF-8.
     * @throws invalid_parser_split_character If predicate_arg rejects a
     * continuation byte, so the returned text would end partway through a
     * character.
     */
    template <typename Predicate>
    requires std::predicate<Predicate&, char8_t>
    [[nodiscard]]
    constexpr input_text
        peek_while(Predicate predicate_arg);
    /*!
     * @brief Returns an input_text, containing a string view from the current
     * position to (not including) the position of the first code unit of the
     * first Unicode character that fails predicate_arg.
     *
     * advance_chars_while() is the same, but moves the position and doesn't
     * return the input_text object.
     *
     * The predicate function takes a char32_t character as input. This function
     * has a fast path for ASCII values, which skips Unicode decoding.
     * peek_while() is a similar function that uses a predicate that checks
     * single code units.
     *
     * This function does not move the position that the parser_input points to
     * in its underlying text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives to find a Unicode character that fails the predicate, or the
     * input ends.
     *
     * If the input ends before a character that fails the predicate is found,
     * an input_text representing all input from the current position to the end
     * of the text is returned.
     *
     * This function stops at the first character that does not satisfy the
     * predicate, so it reads no further than it needs to.
     *
     * @tparam Predicate The function type of predicate_arg.
     * @param predicate_arg A function that takes a char32_t and returns a bool.
     * @return An input_text holding the text from the current position up
     * to (not including) the first code unit of the first Unicode character
     * that fails predicate_arg.
     * @throws invalid_utf8 If an attempt is made to read invalid UTF-8.
     */
    template <typename Predicate>
    requires std::predicate<Predicate&, char32_t>
    [[nodiscard]]
    constexpr input_text
        peek_chars_while(Predicate predicate_arg);
    /*!
     * @brief Returns an input_text, containing a string view from the current
     * position to the position of the next token_arg (not past it) that is not
     * preceded by escape_arg.
     *
     * advance_until() is the same, but moves the position and doesn't return
     * the input_text object.
     *
     * An escape_arg covers (or "escapes") exactly one Unicode scalar value
     * after it - e.g. if escape_arg='#', token_arg='_', with the current
     * position at 0, and the input=hello#_x_, the underscore at position 6 is
     * ignored, and return value=hello#_x. An escape_arg can itself
     * be escaped: with hello##_x_, the first # escapes the second, so return
     * value=hello##.
     *
     * An empty token_arg is found immediately, and an empty input_text
     * returned.
     *
     * token_arg and escape_arg must not have the same first code unit.
     *
     * This function does not move the position that the parser_input points to
     * in its underlying text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives to find the token, or the input ends. If the token is never
     * found, the rest of the input is read, and none of it can be released.
     *
     * Both token_arg and escape_arg must be valid UTF-8.
     *
     * @param token_arg The text to check for.
     * @param escape_arg The escape text, or empty for no escapes. Each
     * occurrence escapes exactly one Unicode scalar value after it.
     * @return An input_text holding the text from the current position up
     * to (not including) the first token_arg not preceded by escape_arg, or
     * std::nullopt if the input ends before one is found.
     * @throws invalid_utf8 If an attempt is made to read invalid UTF-8.
     * @throws invalid_parser_escape_matches_token If token_arg and escape_arg
     * have the same first code unit.
     */
    [[nodiscard]]
    constexpr std::optional<input_text>
        peek_until(
            const std::u8string_view token_arg,
            const std::u8string_view escape_arg = {}
        );
    /*!
     * @brief Moves the position forward, from the current
     * position, to the first code unit that predicate_arg fails.
     *
     * The same as peek_while(), but moves the position instead of returning the
     * text, and creates no input_text. Useful for skipping text that isn't
     * needed, such as a line comment.
     *
     * The predicate function takes a single code unit (char8_t) as input.
     * advance_chars_while() is a similar function that uses a predicate that
     * checks char32_t values.
     *
     * The function checks that the new current position is at a character
     * boundary.
     *
     * This function may move the position that the parser_input points to
     * in its underlying text. If an exception is thrown, the position is
     * unchanged.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives to find a code unit that fails the predicate, or the input ends.
     *
     * If the input ends before a code unit that fails the predicate is found,
     * this function moves the current position to the end of the input text.
     *
     * This function stops at the first code unit that does not satisfy the
     * predicate, so it reads no further than it needs to.
     *
     * @tparam Predicate The function type of predicate_arg.
     * @param predicate_arg A function that takes a char8_t and returns a bool.
     * @return The number of code units advanced. 0 if none were accepted.
     * @throws invalid_utf8 If an attempt is made to read invalid UTF-8.
     * @throws invalid_parser_split_character If predicate_arg rejects a
     * continuation byte, so the new position would split a character.
     */
    template <typename Predicate>
    requires std::predicate<Predicate&, char8_t>
    constexpr std::size_t
        advance_while(Predicate predicate_arg);
    /*!
     * @brief Moves the current position forward past the Unicode characters to
     * the first code unit of the first Unicode character that fails
     * predicate_arg.
     *
     * The same as peek_chars_while(), but moves the position instead of
     * returning the text, and creates no input_text. Useful for skipping text
     * that isn't needed, such as a custom set of whitespace characters.
     *
     * The predicate function takes a char32_t character as input. This function
     * has a fast path for ASCII values, which skips Unicode decoding.
     * advance_while() is a similar function that uses a predicate that checks
     * single code units.
     *
     * This function may move the position that the parser_input points to
     * in its underlying text. If an exception is thrown, the position is
     * unchanged.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives to find a Unicode character that fails the predicate, or the
     * input ends.
     *
     * If the input ends before a character that fails the predicate is found,
     * the function moves the current position to the end of the input text.
     *
     * This function stops at the first character that does not satisfy the
     * predicate, so it reads no further than it needs to.
     *
     * @tparam Predicate The function type of predicate_arg.
     * @param predicate_arg A function that takes a char32_t and returns a bool.
     * @return The number of code units moved past; 0 if none were accepted.
     * @throws invalid_utf8 If an attempt is made to read invalid UTF-8.
     */
    template <typename Predicate>
    requires std::predicate<Predicate&, char32_t>
    constexpr std::size_t
        advance_chars_while(Predicate predicate_arg);
    /*!
     * @brief Advances the current position to the position of the next
     * token_arg (not past it) that is not preceded by escape_arg.
     *
     * The same as peek_until(), but moves the position instead of
     * returning the text, and creates no input_text. Useful for skipping text
     * that isn't needed.
     *
     * An escape_arg covers (or "escapes") exactly one Unicode scalar value
     * after it - e.g. if escape_arg='#', token_arg='_', with the current
     * position at 0, and the input=hello#_x_, the underscore at position 6 is
     * ignored, and the current position is moved to 8. An escape_arg can itself
     * be escaped: with hello##_x_, the first # escapes the second, so the
     * position moves to 7.
     *
     * An empty token_arg is found immediately.
     *
     * token_arg and escape_arg must not have the same first code unit.
     *
     * This function may move the position that the parser_input points to
     * in its underlying text. If the token isn't found, the current position is
     * unchanged, and std::nullopt is returned. If an exception is thrown, the
     * current position is unchanged.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives to find the token, or the input ends. If the token is never
     * found, the rest of the input is read, and none of it can be released.
     *
     * Both token_arg and escape_arg must be valid UTF-8.
     *
     * @param token_arg The text to check for.
     * @param escape_arg The escape text, or empty for no escapes. Each
     * occurrence escapes exactly one Unicode scalar value after it.
     * @return The number of code units moved past, or std::nullopt if the input
     * ends before a token_arg not preceded by an escape_arg is found.
     * @throws invalid_utf8 If an attempt is made to read invalid UTF-8.
     * @throws invalid_parser_escape_matches_token If token_arg and escape_arg
     * have the same first code unit.
     */
    [[nodiscard]]
    constexpr std::optional<std::size_t>
        advance_until(
            const std::u8string_view token_arg,
            const std::u8string_view escape_arg = {}
        );
    /*!
     * @brief Returns to the caller the next Unicode character, as a char32_t,
     * in the input text, together with the number of code units it uses in the
     * UTF-8 text.
     *
     * The number of code units can be passed to advance() to move past the
     * character.
     *
     * If there is no more text, a std::nullopt is returned.
     *
     * This function does not change the underlying position of the parser_input
     * in its input text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives to find the next Unicode character, or the input ends.
     *
     * @return Either a pair containing the next valid char32_t and the
     * number of code units used to create it, or a std::nullopt if no more
     * text is available.
     * @throws invalid_utf8 If invalid UTF-8 is reached.
     */
    [[nodiscard]]
    constexpr std::optional<std::pair<char32_t, std::size_t>>
        peek_char();
    /*!
     * @brief Returns to the caller the next count_arg Unicode characters in the
     * underlying text, wrapped in an input_text object.
     *
     * If the input ends before count_arg characters are found, a std::nullopt
     * is returned.
     *
     * This function does not change the underlying position of the parser_input
     * in its input text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives for it to either extract the required text, or determine there is
     * insufficient text.
     *
     * This function stops when it has the required number of Unicode
     * characters, so it reads no further than it needs to.
     *
     * @param count_arg The number of Unicode characters desired.
     * @return An optional either containing the next valid count_arg Unicode
     * characters in an input_text object, or an empty optional if sufficient
     * characters are not available.
     * @throws invalid_utf8 If invalid UTF-8 is reached.
     */
    [[nodiscard]]
    constexpr std::optional<input_text>
        peek_chars(const std::size_t count_arg);
    /*!
     * @brief Moves the position forward by n_code_units_arg code units.
     *
     * n_code_units_arg is measured in code units (char8_t), not Unicode
     * characters. It is usually the size of something already checked, such as
     * the text from a peek function, or a token matched by starts_with().
     *
     * This function can change the underlying position of the parser_input in
     * its input text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives for it to advance to the requested position.
     *
     * @param n_code_units_arg The number of code units to move ahead by.
     * @throws invalid_utf8 If invalid UTF-8 is reached.
     * @throws invalid_parser_advanced_past_end If an attempt is made to move
     * past the end of the text.
     * @throws invalid_parser_advanced_into_character If the new position
     * would be partway through a Unicode character.
     */
    constexpr void
        advance(const std::size_t n_code_units_arg);
    /*!
     * @brief Moves the position past token_arg if the text at the current
     * position starts with it.
     *
     * If the text doesn't start with token_arg, the position is unchanged.
     *
     * This function can change the underlying position of the parser_input in
     * its input text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives for it to perform its check.
     *
     * This function stops at the first code unit that allows it to determine
     * there is no match, so it reads no further than it needs to.
     *
     * @param token_arg The text to match. Must be valid UTF-8.
     * @return True if the text matched and the position moved. False
     * otherwise.
     * @throws invalid_utf8 If invalid UTF-8 is reached before a result is
     * decided.
     * @throws invalid_parser_advanced_into_character If token_arg is not valid
     * UTF-8, and so ends partway through a character.
     */
    constexpr bool
        advance_if_starts_with(const std::u8string_view token_arg);
    /*!
     * @brief Moves the current position forwards past any whitespace.
     *
     * See unicode_bridge::is_whitespace for details on what constitutes
     * whitespace. advance_chars_while() can be used to skip a different set of
     * characters, if required.
     *
     * This function can change the underlying position of the parser_input in
     * its input text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives for it to find non-whitespace, or the input ends.
     *
     * @throws invalid_utf8 If invalid UTF-8 is reached while skipping.
     */
    constexpr void
        skip_whitespace();
    /*!
     * @brief Creates a failed basic_parser_return_value, with message_arg
     * as its message, at position_arg.
     *
     * Intended to be returned straight from a parser, e.g.
     * return input.fail(u8"Expected a digit.", position);
     *
     * See basic_parser_return_value for details on what the returned object
     * contains.
     *
     * Does not move the position of parser_input.
     *
     * @tparam S The type of message_arg; std::u8string must be constructible
     * from it.
     * @param message_arg The bespoke message describing the failure.
     * @param position_arg The position in the whole input where the failure
     * occurred.
     * @return A basic_parser_return_value describing the error.
     */
    template <typename S>
    requires std::constructible_from<std::u8string, S&&>
    [[nodiscard]]
    constexpr basic_parser_return_value
        fail(S&& message_arg, const std::size_t position_arg) const;
    /*!
     * @brief Creates a failed basic_parser_return_value, with message_arg
     * as its message, at the current position in parser_input.
     *
     * Intended to be returned straight from a parser, e.g.
     * return input.fail(u8"Expected a digit.");
     *
     * See basic_parser_return_value for details on what the returned object
     * contains.
     *
     * Does not move the position of parser_input.
     *
     * @tparam S The type of message_arg; std::u8string must be constructible
     * from it.
     * @param message_arg The bespoke message describing the failure.
     * @return A basic_parser_return_value describing the error.
     */
    template <typename S>
    requires std::constructible_from<std::u8string, S&&>
    [[nodiscard]]
    constexpr basic_parser_return_value
        fail(S&& message_arg) const;
    /*!
     * @brief Creates a text_location based on the given position.
     *
     * A text_location object represents a line and column position.
     * See text_location.
     *
     * For a parser_input constructed with a string, the line and column are
     * always available. For one constructed with a text_source, what is
     * available depends on the source; see text_source::location_at().
     *
     * @param position_arg The position in the whole input (as returned by
     * position()). For a parser_input constructed with a string, a position
     * past the end is treated as the end.
     * @return The text_location object describing the given position in the
     * text.
     */
    [[nodiscard]]
    constexpr text_location
        location_at(const std::size_t position_arg) const;
    /*!
     * @brief Creates a text_location based on the current position.
     *
     * A text_location object represents a line and column position.
     * See text_location.
     *
     * For a parser_input constructed with a string, the line and column are
     * always available. For one constructed with a text_source, what is
     * available depends on the source; see text_source::location_at().
     *
     * @return The text_location object describing the current position in the
     * text.
     */
    [[nodiscard]]
    constexpr text_location
        location() const;
    /*!
     * @brief Creates a parse_error_context around the given position.
     *
     * Shows up to context_chars() characters either side of the given
     * position. Only text still in memory can be shown, so text that has
     * been released is left out. See parse_error_context and context_chars().
     *
     * This function never blocks: it only reads more text from sources that
     * can do so without waiting.
     *
     * @param position_arg The position in the whole input (as returned by
     * position()) to focus on. If it has already been released, the earliest
     * text still in memory is the focus instead.
     * @return The created parse_error_context object.
     */
    [[nodiscard]]
    constexpr parse_error_context
        get_context(const std::size_t position_arg);
    /*!
     * @brief Creates a parse_error_context around the current position.
     *
     * Shows up to context_chars() characters either side of the current
     * position. Only text still in memory can be shown, so text that has
     * been released is left out. See parse_error_context and context_chars().
     *
     * This function never blocks: it only reads more text from sources that
     * can do so without waiting.
     *
     * @return The created parse_error_context object.
     */
    [[nodiscard]]
    constexpr parse_error_context
        get_context();
    /*!
     * @brief Creates an input_mark object, which saves the current position in
     * the object.
     *
     * When used in tandem with reset, it can allow those writing
     * parsers to move forward/back through the text.
     *
     * While the input_mark exists, the parser_input won't release text from
     * its position onwards.
     *
     * When the input_mark is destroyed, it signals to the object that text from
     * that position onwards can be released by a later release_consumed(),
     * unless another input_mark still holds an earlier position.
     *
     * The created input_mark must not outlive the parser_input object it was
     * created from.
     *
     * The created object should be kept as a local variable.
     *
     * @param source_arg The source location where the mark was created. Used
     * for diagnosing errors.
     * @return The created input_mark.
     */
    [[nodiscard]]
    constexpr input_mark
        mark(
            const std::source_location& source_arg
            = std::source_location::current()
        );
    /*!
     * @brief Tells the parser_input that the text before the current position
     * will not be required again, and can be released.
     *
     * It is designed for parsers that can parse their input strings in parts -
     * e.g. sequences or strings. It is also called by run_parser()
     * after it successfully parses a value.
     *
     * Text from the earliest active input_mark's position onwards is not
     * released, signifying that text is still required. Once an input_mark is
     * destroyed, a later call to release_consumed may release that text.
     * input_text objects don't stop text being released; instead each relies on
     * interacting with the text_source to ensure its text remains in memory as
     * long as it is required.
     *
     * Text is only released once enough can be released at once; see
     * fraction_of_buffer_threshold(). context_chars() characters before the
     * release point are kept, for error messages.
     */
    constexpr void
        release_consumed();
    /*!
     * @brief Runs the parser parser_arg on the text at the current position,
     * using value_arg to store the parsed value.
     *
     * This is the only way to run parsers. run_parser in parser objects
     * requires a parser_run_key object, which can only be created by
     * parser_input.
     *
     * This function calls release_consumed after it successfully finishes
     * parsing.
     *
     * @tparam Parser The type of parser_arg.
     * @tparam T The type being parsed.
     * @param parser_arg The parser to run.
     * @param value_arg Where the parsed value is stored. On failure, it may
     * have been partly modified. Parsers are encouraged, but not required, to
     * leave it unchanged.
     * @return basic_parser_return_value, representing the state of the parse.
     */
    template <typename Parser, typename T>
    requires parser_c<Parser, T>
    constexpr basic_parser_return_value
        run_parser(const Parser& parser_arg, T& value_arg);
    /*!
     * @brief Goes back or forward to a saved position in the parser_input's
     * text.
     *
     * The input_mark's position is always still available to the parser_input -
     * while an input_mark exists, the text from its position onwards is never
     * released. For string-based parser_inputs the whole string is always
     * available.
     *
     * This function will change the underlying position of the parser_input in
     * its input text.
     *
     * @param mark_arg The input_mark holding the position to move to.
     * @param source_location_arg The source of where reset was called from.
     * @throws invalid_parser_foreign_mark If mark_arg came from a different
     * parser_input, or has been moved from.
     */
    constexpr void
        reset(
            const input_mark&           mark_arg,
            const std::source_location& source_location_arg
            = std::source_location::current()
        );
    /*!
     * @brief Gets the object's context chars.
     *
     * The number of Unicode characters shown either side of the focus character
     * in a context (see get_context()), used in error messages. Fewer may be
     * shown near the start or end of the input, or where text has been
     * released, as the number of characters may not be available.
     *
     * For the purposes of the context, each code unit of invalid UTF-8 counts
     * as one character.
     *
     * @return The object's context chars.
     */
    [[nodiscard]]
    constexpr std::size_t
        context_chars() const noexcept;
    /*!
     * @brief Gets the fraction of buffer threshold.
     *
     * release_consumed() only releases text once the amount it could release
     * is at least this fraction of the buffered text, and at least the source's
     * minimum_release(). This stops it making many small releases, which would
     * be slow. Always in [0, 1). Unused when parser_input is constructed using
     * a string, as nothing is released.
     *
     * @return The fraction of buffer threshold.
     */
    [[nodiscard]]
    constexpr double
        fraction_of_buffer_threshold() const noexcept;
private:
    friend struct input_mark;
    /*!
     * @brief A helper function to create and throw parse_exceptions.
     *
     * @param code_arg The error to be thrown.
     * @throws parse_exception Always thrown, containing code_arg.
     */
    [[noreturn]]
    static void
        throw_error(parse_result_error&& code_arg);
    /*!
     * @brief Reloads the underlying text (_data) in the parser_input object,
     * as well as its associated variables.
     *
     * This function assumes the parser_input uses a text_source.
     *
     * Reloads _data, _size and _valid.
     *
     * input_text objects are unaffected by this, as the text_source guarantees
     * that their underlying std::u8string_views will remain valid while the
     * objects exist. Explicit views of _data - not held by an input_text - are
     * not guaranteed to stay valid.
     *
     * Does not update _base or _index.
     */
    constexpr void
        reload() noexcept;
    /*!
     * @brief Reads more text from the text_source.
     *
     * Calls reload() afterwards, so any view of _data taken before this call is
     * invalid afterwards. For a source that may wait for input, such as
     * std::cin, this can block.
     *
     * @return True if more text was read; false if there's no text_source, the
     * input has ended, or invalid UTF-8 has been reached, as nothing more can
     * then become valid.
     */
    constexpr bool
        refill();
    /*!
     * @brief Checks that the text in _data, up to (but not including) end_arg,
     * is available and valid UTF-8.
     *
     * This function does not move the position that the parser_input points to
     * in its underlying text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives for it to check up to end_arg.
     *
     * @param end_arg One past the last index in _data that must be valid.
     * @return True if the input was found to be valid. False if the input ends
     * first.
     * @throws invalid_utf8 If invalid UTF-8 is found before end_arg.
     */
    constexpr bool
        ensure_valid(const std::size_t end_arg);
    /*!
     * @brief Checks whether the string token_arg matches the text at
     * offset_arg, relative to the current position, in the parser_input's text.
     *
     * This function does not move the position that the parser_input points to
     * in its underlying text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives to check whether there is a match.
     *
     * This function stops at the first code unit that allows it to determine
     * there is no match, so it reads no further than it needs to.
     *
     * @param offset_arg The offset from the current position at which to
     * check token_arg.
     * @param token_arg The text to check against the input at that
     * offset.
     * @return True if token_arg matches the text at offset_arg; false if it
     * doesn't, or if the input ends first.
     * @throws invalid_utf8 If an attempt is made to read invalid UTF-8.
     */
    constexpr bool
        matches_at(
            const std::size_t        offset_arg,
            const std::u8string_view token_arg
        );
    /*!
     * @brief Finds the position in _data of the next code unit, from
     * the current position, that fails the given predicate.
     *
     * The predicate function takes a single code unit (char8_t) as input.
     * scan_chars_while() is a similar function that uses a predicate that
     * checks char32_t values.
     *
     * The function checks that the returned position is at a character
     * boundary.
     *
     * This function does not move the position that the parser_input points to
     * in its underlying text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives to find a code unit that fails the predicate.
     *
     * If the input ends before a code unit that fails the predicate is found,
     * this function returns the position just after the end of the input.
     *
     * This function stops at the first code unit that does not satisfy the
     * predicate, so it reads no further than it needs to.
     *
     * @tparam Predicate The function type of predicate_arg.
     * @param predicate_arg A function that takes a char8_t and returns a bool.
     * It is taken by reference to avoid copying the caller's predicate.
     * @return The position of the first code unit that fails predicate_arg, or
     * the position just after the end of the input if none does.
     * @throws invalid_utf8 If an attempt is made to read invalid UTF-8.
     * @throws invalid_parser_split_character If predicate_arg rejects a
     * continuation byte, so the returned position would split a character.
     */
    template <typename Predicate>
    requires std::predicate<Predicate&, char8_t>
    constexpr std::size_t
        scan_while(Predicate& predicate_arg);
    /*!
     * @brief Finds the position in _data of the next Unicode scalar value, from
     * the current position, that fails the given predicate.
     *
     * The predicate function takes a char32_t character as input. This function
     * has a fast path for ASCII values, which skips Unicode decoding.
     * scan_while is a similar function that uses a predicate that checks single
     * code units.
     *
     * This function does not move the position that the parser_input points to
     * in its underlying text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives to find a Unicode character that fails the predicate.
     *
     * If the input ends before a character that fails the predicate is found,
     * this function returns the position just after the end of the input.
     *
     * This function stops at the first character that does not satisfy the
     * predicate, so it reads no further than it needs to.
     *
     * @tparam Predicate The function type of predicate_arg.
     * @param predicate_arg A function that takes a char32_t and returns a bool.
     * It is taken by reference to avoid copying the caller's predicate.
     * @return The position of the first code unit of the first character that
     * fails predicate_arg, or the position just after the end of the input if
     * none does.
     * @throws invalid_utf8 If an attempt is made to read invalid UTF-8.
     */
    template <typename Predicate>
    requires std::predicate<Predicate&, char32_t>
    constexpr std::size_t
        scan_chars_while(Predicate& predicate_arg);
    /*!
     * @brief Finds the position in _data of the next token_arg, from the
     * current position, that is not preceded by escape_arg.
     *
     * An escape_arg covers (or "escapes") exactly one Unicode scalar value
     * after it - e.g. if escape_arg='#', token_arg='_', with the current
     * position at 0, and the input=hello#_x_, the underscore at position 6 is
     * ignored, and 8 is returned. An escape_arg can itself be escaped: with
     * hello##_x_, the first # escapes the second, so the underscore at position
     * 7 is found.
     *
     * An empty token_arg is found immediately.
     *
     * token_arg and escape_arg must not have the same first code unit.
     *
     * This function does not move the position that the parser_input points to
     * in its underlying text.
     *
     * This function can request more text from the text_source, if the
     * parser_input object was constructed with one. For a source that may wait
     * for input, such as std::cin, this function can block until enough text
     * arrives to find the token.
     *
     * Both token_arg and escape_arg must be valid UTF-8.
     *
     * @param token_arg The text to check for.
     * @param escape_arg The escape text, or empty for no escapes. Each
     * occurrence escapes exactly one Unicode scalar value after it.
     * @return The position in _data where the next unescaped token_arg
     * starts, or std::nullopt if the input ends before one is found.
     * @throws invalid_utf8 If an attempt is made to read invalid UTF-8.
     * @throws invalid_parser_escape_matches_token If token_arg and escape_arg
     * have the same first code unit.
     */
    constexpr std::optional<std::size_t>
        scan_until(
            const std::u8string_view token_arg,
            const std::u8string_view escape_arg
        );
    /*!
     * @brief Informs the parser_input object that the text from position_arg, a
     * position in the whole input, must be kept. Called when an input_mark is
     * created; paired with a remove_hold of the same position when the mark is
     * destroyed.
     *
     * @param position_arg The position in the whole input that should be kept.
     */
    constexpr void
        add_hold(const std::size_t position_arg);
    /*!
     * @brief Removes a single hold position at position_arg.
     *
     * Holds are usually removed in the reverse order they are added, so this is
     * usually immediate.
     *
     * @param position_arg The position to search for in the _holds structure.
     */
    constexpr void
        remove_hold(const std::size_t position_arg) noexcept;
    /*!
     * @brief Helper function that creates an input_text from start and length
     * positions.
     *
     * If the object is created using a text_source, the returned input_text
     * pins the text source's current storage (if it has any - can be nullptr).
     *
     * @param index_arg The start position of the underlying std::u8string_view
     * the created input_text will contain.
     * @param length_arg The length of the underlying std::u8string_view the
     * created input_text will contain.
     * @return The created input_text object.
     */
    [[nodiscard]]
    constexpr input_text
        make_text(const std::size_t index_arg, const std::size_t length_arg);
    //! The underlying text the object is currently looking at.
    const char8_t* _data = nullptr;
    //! The number of code units currently in _data.
    std::size_t _size    = 0;
    //! The number of code units at the start of _data known to be valid UTF-8.
    std::size_t _valid   = 0;
    //! The position the object is currently at in _data.
    std::size_t _index   = 0;
    //! The position in the whole input of _source that _data[0] is at. 0 when
    //! parser_input is initialised with a string.
    std::size_t _base    = 0;
    //! The underlying text_source, containing the text data. Can be nullptr
    //! when parser_input is initialised with a string.
    text_source* _source = nullptr;
    //! The positions held by active input_marks. Text from the earliest is
    //! never released.
    std::vector<std::size_t> _holds;
    //! See context_chars().
    std::size_t _context_chars           = 0;
    //! See fraction_of_buffer_threshold().
    double _fraction_of_buffer_threshold = 0.0;
};

PNP_INTERNAL_NS_BEGIN
/*!
 * @brief Gets the length of a UTF-8 sequence from its first code unit.
 *
 * If char_arg is not a leading byte, sequence_length returns 1.
 *
 * @param char_arg The first code unit in a UTF-8 sequence.
 * @return The sequence length if char_arg is a valid leading byte, 1 otherwise.
 */
constexpr std::size_t
    sequence_length(const char8_t char_arg);
PNP_INTERNAL_NS_END
PNP_NS_END

PNP_NS_BEGIN
constexpr input_mark::input_mark(
    parser_input&               input_arg,
    const std::source_location& source_location_arg
)
    : _input(&input_arg)
    , _position(input_arg.position())
    , _source_location(source_location_arg)
{
    _input->add_hold(_position);
}

constexpr input_mark::~input_mark() noexcept
{
    let_go();
}

constexpr input_mark::input_mark(
    input_mark&& other_arg
) noexcept
    : _input(std::exchange(other_arg._input, nullptr))
    , _position(other_arg._position)
    , _source_location(other_arg._source_location)
{}

constexpr input_mark&
    input_mark::operator=(
        input_mark&& other_arg
    ) noexcept
{
    if (this != &other_arg)
    {
        let_go();
        _input           = std::exchange(other_arg._input, nullptr);
        _position        = other_arg._position;
        _source_location = other_arg._source_location;
    }
    return *this;
}

constexpr const parser_input*
    input_mark::input() const noexcept
{
    return _input;
}

constexpr std::size_t
    input_mark::position() const noexcept
{
    return _position;
}

constexpr void
    input_mark::let_go() noexcept
{
    if (_input != nullptr)
    {
        _input->remove_hold(_position);
        _input = nullptr;
    }
}

constexpr input_text::input_text(
    input_text&& other_arg
) noexcept
    : _text(std::exchange(other_arg._text, std::u8string_view()))
    , _text_source(std::exchange(other_arg._text_source, nullptr))
    , _text_storage(std::exchange(other_arg._text_storage, nullptr))
{}

constexpr input_text&
    input_text::operator=(
        input_text&& other_arg
    ) noexcept
{
    if (this != &other_arg)
    {
        release();
        _text         = std::exchange(other_arg._text, std::u8string_view());
        _text_source  = std::exchange(other_arg._text_source, nullptr);
        _text_storage = std::exchange(other_arg._text_storage, nullptr);
    }
    return *this;
}

constexpr input_text::~input_text() noexcept
{
    release();
}

constexpr std::u8string_view
    input_text::get() const noexcept
{
    return _text;
}

constexpr input_text::input_text(
    const std::u8string_view text_arg,
    text_source*             text_source_arg,
    pinnable_storage*        text_storage_arg
) noexcept
    : _text(text_arg)
    , _text_source(text_source_arg)
    , _text_storage(text_storage_arg)
{}

constexpr void
    input_text::release() noexcept
{
    if (_text_storage != nullptr)
    {
        --_text_storage->pins;
        // A block that is no longer current and with no pins can be removed.
        if (_text_storage->pins == 0
            && _text_storage != _text_source->_current_storage)
        {
            _text_source->free_unpinned_storage(_text_storage);
        }
    }
    _text_source  = nullptr;
    _text_storage = nullptr;
}

constexpr parser_input::parser_input(
    const std::u8string_view str_arg,
    const std::size_t        context_chars_arg
) noexcept
    : _data(str_arg.data())
    , _size(str_arg.size())
    , _valid(UNICODE_BRIDGE_NAMESPACE::validate_unicode(str_arg))
    , _context_chars(context_chars_arg)
    , _fraction_of_buffer_threshold(
          current_pnp_options().parser_input_fraction_of_buffer_threshold
      )
{}

inline parser_input::parser_input(
    const std::string_view str_arg,
    const std::size_t      context_chars_arg
) noexcept
    : parser_input(
          std::u8string_view(
              UNICODE_BRIDGE_NAMESPACE::cast_char_ptr<char8_t>(str_arg.data()),
              str_arg.size()
          ),
          context_chars_arg
      )
{}

inline parser_input::parser_input(
    text_source&      source_arg,
    const std::size_t context_chars_arg,
    const double      fraction_of_buffer_threshold_arg
)
    : _base(source_arg.offset())
    , _source(&source_arg)
    , _context_chars(context_chars_arg)
    , _fraction_of_buffer_threshold(PNP_INTERNAL_NAMESPACE::clamp_fraction(
          fraction_of_buffer_threshold_arg
      ))
{
    if (source_arg._in_use)
    {
        throw std::logic_error(
            "parser_input: another parser_input is already reading from the "
            "source"
        );
    }
    source_arg._in_use = true;
    reload();
}

constexpr parser_input::~parser_input()
{
    assert(_holds.empty());
    if (_source != nullptr)
    {
        _source->_in_use = false;
    }
}

constexpr bool
    parser_input::at_end()
{
    // We use not refill() instead of not ensure_valid(_index + 1) for 2
    // reasons: 1) It would wait for entire character, not just first byte. 2)
    // It would throw invalid_utf8 if the remaining text is invalid.
    return _index >= _size && not refill();
}

constexpr std::size_t
    parser_input::position() const noexcept
{
    return _base + _index;
}

constexpr bool
    parser_input::starts_with(
        const std::u8string_view token_arg
    )
{
    return matches_at(0, token_arg);
}

constexpr bool
    parser_input::starts_with(
        const std::initializer_list<std::u8string_view> tokens_arg
    )
{
    using namespace std;
    size_t offset = 0;
    for (const u8string_view token : tokens_arg)
    {
        if (not matches_at(offset, token))
        {
            return false;
        }
        offset += token.size();
    }
    return true;
}

template <typename Predicate>
requires std::predicate<Predicate&, char8_t>
constexpr input_text
    parser_input::peek_while(
        Predicate predicate_arg
    )
{
    const std::size_t end = scan_while(predicate_arg);
    return make_text(_index, end - _index);
}

template <typename Predicate>
requires std::predicate<Predicate&, char32_t>
constexpr input_text
    parser_input::peek_chars_while(
        Predicate predicate_arg
    )
{
    const std::size_t end = scan_chars_while(predicate_arg);
    return make_text(_index, end - _index);
}

constexpr std::optional<input_text>
    parser_input::peek_until(
        const std::u8string_view token_arg,
        const std::u8string_view escape_arg
    )
{
    const auto end = scan_until(token_arg, escape_arg);
    if (not end.has_value())
    {
        return std::nullopt;
    }
    else
    {
        return make_text(_index, end.value() - _index);
    }
}

template <typename Predicate>
requires std::predicate<Predicate&, char8_t>
constexpr std::size_t
    parser_input::advance_while(
        Predicate predicate_arg
    )
{
    const std::size_t end      = scan_while(predicate_arg);
    const std::size_t advanced = end - _index;
    _index                     = end;
    return advanced;
}

template <typename Predicate>
requires std::predicate<Predicate&, char32_t>
constexpr std::size_t
    parser_input::advance_chars_while(
        Predicate predicate_arg
    )
{
    const std::size_t end      = scan_chars_while(predicate_arg);
    const std::size_t advanced = end - _index;
    _index                     = end;
    return advanced;
}

constexpr std::optional<std::size_t>
    parser_input::advance_until(
        const std::u8string_view token_arg,
        const std::u8string_view escape_arg
    )
{
    const auto end = scan_until(token_arg, escape_arg);
    if (not end.has_value())
    {
        return std::nullopt;
    }
    else
    {
        const std::size_t advanced = end.value() - _index;
        _index                     = end.value();
        return advanced;
    }
}

constexpr std::optional<std::pair<char32_t, std::size_t>>
    parser_input::peek_char()
{
    using namespace std;
    using namespace UNICODE_BRIDGE_NAMESPACE;
    // Ensures (if possible) there's at least one valid character to look at.
    if (_index >= _valid && not ensure_valid(_index + 1))
    {
        // There's no next character to find
        return std::nullopt;
    }
    const u8string_view to_check(_data + _index, _valid - _index);
    auto next_char = next_char32_no_error(to_check.begin(), to_check.end());
    if (not next_char.has_value()) [[unlikely]]
    {
        // This should not happen, as text has been validated. So there's a bug
        // somewhere.
        throw std::logic_error(
            "pnp internal error: text checked as valid UTF-8 and not "
            "at end of string. However, next_char32 returning error. This is a "
            "bug in pnp; please report it."
        );
    }
    else
    {
        return next_char;
    }
}

constexpr std::optional<input_text>
    parser_input::peek_chars(
        const std::size_t count_arg
    )
{
    using namespace std;
    // Where the characters counted so far end; initialised at the current
    // position. Since position is at a character boundary, and we only probe
    // valid UTF-8, all we have to do is increment using the sequence length of
    // the characters.
    size_t end = _index;
    // Counts actual chars, not code units.
    for (size_t counted = 0; counted < count_arg; ++counted)
    {
        // If passed valid and ensure_valid returns false (so there is no more
        // text), return nullopt as the request failed.
        if (end >= _valid && not ensure_valid(end + 1))
        {
            return std::nullopt;
        }
        end += PNP_INTERNAL_NAMESPACE::sequence_length(_data[end]);
    }
    return make_text(_index, end - _index);
}

constexpr void
    parser_input::advance(
        const std::size_t n_code_units_arg
    )
{
    using namespace std;
    // Move through the input, ensure that what we're trying to get can exist.
    while (n_code_units_arg > _size - _index && refill())
    {
    }
    // Checking for overflow. _index guaranteed to be <= _size, so subtraction
    // shouldn't underflow.
    if (n_code_units_arg > _size - _index) [[unlikely]]
    {
        throw_error(parse_result_error::invalid_parser_advanced_past_end(
            position(), n_code_units_arg, _size - _index
        ));
    }
    // By this point, know target_position is <= _size.
    const size_t target_position = _index + n_code_units_arg;
    // Moving past valid UTF8, must be checked.
    if (target_position > _valid) [[unlikely]]
    {
        // Check up to target_position is valid UTF-8. Can't return false, as
        // the text is already here.
        ensure_valid(target_position);
    }
    // Checks that target_position is valid, and the character it's pointing at
    // is a continuation byte. If target_position == _valid, it's guaranteed to
    // be on a character boundary, so this is skipped.
    if (target_position < _valid
        && UNICODE_BRIDGE_NAMESPACE::UNICODE_BRIDGE_NAMESPACE_INTERNAL::
            is_utf8_continuation_byte(_data[target_position])) [[unlikely]]
    {
        // Trying to land inside a character.
        throw_error(parse_result_error::invalid_parser_advanced_into_character(
            _base + target_position, n_code_units_arg, _data[target_position]
        ));
    }
    _index = target_position;
}

constexpr bool
    parser_input::advance_if_starts_with(
        const std::u8string_view token_arg
    )
{
    if (not starts_with(token_arg))
    {
        return false;
    }
    advance(token_arg.size());
    return true;
}

constexpr void
    parser_input::skip_whitespace()
{
    advance_chars_while(
        [](const char32_t char_arg)
        {
            return UNICODE_BRIDGE_NAMESPACE::is_whitespace(char_arg);
        }
    );
}

template <typename S>
requires std::constructible_from<std::u8string, S&&>
constexpr basic_parser_return_value
    parser_input::fail(
        S&&               message_arg,
        const std::size_t position_arg
    ) const
{
    return basic_parser_return_value(
        position_arg, std::forward<S>(message_arg)
    );
}

template <typename S>
requires std::constructible_from<std::u8string, S&&>
constexpr basic_parser_return_value
    parser_input::fail(
        S&& message_arg
    ) const
{
    return fail(std::forward<S>(message_arg), position());
}

constexpr text_location
    parser_input::location_at(
        const std::size_t position_arg
    ) const
{
    // If there's a source, let that deal with the location_at functionality.
    using namespace std;
    return (_source != nullptr)
               ? _source->location_at(position_arg)
               : text_location{}.after(u8string_view(
                     _data,
                     // Use a ternary instead of std::min, so no reliance on
                     // <algorithm>
                     position_arg < _size ? position_arg : _size
                 ));
}

constexpr text_location
    parser_input::location() const
{
    return location_at(position());
}

constexpr parse_error_context
    parser_input::get_context(
        const std::size_t position_arg
    )
{
    using namespace std;
    using namespace UNICODE_BRIDGE_NAMESPACE;
    // Is the UTF8 character that begins at position_arg available? Or has it
    // been discarded.
    const bool is_positions_char_discarded = position_arg < _base;
    size_t     index = is_positions_char_discarded ? 0 : position_arg - _base;
    // If the object is using a text_source, and that source can read
    // without waiting (so input isn't a stream or over network).
    if (_source != nullptr && _source->reads_without_waiting())
    {
        // Grab the maximum number of characters we will need.
        // Maximum will be 4 (max sequence size UTF8) * number of characters
        // wanted for the context.
        const size_t wanted = index + 4 * (context_chars() + 1);
        while (_size < wanted && refill())
        {
        }
    }
    // Only get the text now, as it may have been reset.
    const u8string_view text(_data, _size);
    // This makes sure index isn't beyond the text size.
    index             = index < _size ? index : _size;
    // If index == size(), then the index is at the end, so there shouldn't be a
    // focus_size.
    size_t focus_size = 0;
    if (index < text.size())
    {
        const size_t whole_length
            = PNP_INTERNAL_NAMESPACE::sequence_length(text[index]);
        const size_t remaining = text.size() - index;
        focus_size = whole_length < remaining ? whole_length : remaining;
    }
    // This creates a set of integers that describe what positions of the source
    // to extract to get the desired text - that is, focused around index, which
    // uses focus_size code units, with context_chars() Unicode characters
    // either side of the focus.
    const focused_string_info info = make_focused_string_info(
        text, index, focus_size, context_chars(), context_chars()
    );
    return parse_error_context{
        .before_pos_text
        = u8string(text.substr(info.left_begin_str, info.left_str_size)),
        .current_pos_text
        = u8string(text.substr(info.central_begin_str, info.central_str_size)),
        .after_pos_text
        = u8string(text.substr(info.right_begin_str, info.right_str_size)),
        .before_text_cut_off
        // Each of these says that "there was more text before the context"
        = info.left_had_more_input || _base > 0 || is_positions_char_discarded,
        .after_text_cut_off
        = info.right_had_more_input
          // For abstract sources, just ask if the source was exhausted.
          || (_source != nullptr && not _source->exhausted()),
        .location
        = location_at(is_positions_char_discarded ? _base : position_arg),
    };
}

constexpr parse_error_context
    parser_input::get_context()
{
    return get_context(position());
}

constexpr input_mark
    parser_input::mark(
        const std::source_location& source_arg
    )
{
    return input_mark(*this, source_arg);
}

constexpr void
    parser_input::release_consumed()
{
    using namespace std;
    // The earliest held position, or numeric_limits::max if there is none.
    auto earliest_hold = [this]()
    {
        size_t earliest = numeric_limits<size_t>::max();
        for (const size_t hold : _holds)
        {
            earliest = hold < earliest ? hold : earliest;
        }
        return earliest;
    };
    // To calculate n_chars_arg Unicode characters from from_arg.
    auto back_up_chars
        = [this](const size_t from_arg, const size_t n_chars_arg) -> size_t
    {
        size_t index = from_arg - _base;
        for (size_t backed = 0; backed < n_chars_arg && index > 0; ++backed)
        {
            --index;
            // Grab all the continuation bytes. We know the previous text is
            // valid, so all we have to do is find a non-continuation byte.
            while (
                index > 0
                && UNICODE_BRIDGE_NAMESPACE::UNICODE_BRIDGE_NAMESPACE_INTERNAL::
                    is_utf8_continuation_byte(_data[index])
            )
            {
                --index;
            }
        }
        return _base + index;
    };
    // Fixed strings have nothing to release.
    if (_source == nullptr)
    {
        return;
    }
    auto&        source_var       = *_source;
    const size_t current          = position();
    // Fraction of the size of the buffered text.
    const auto threshold_fraction = static_cast<size_t>(
        fraction_of_buffer_threshold() * static_cast<double>(_size)
    );
    // Normalise this value based off threshold_fraction and source_var's
    // minimum_release().
    const size_t threshold = source_var.minimum_release() > threshold_fraction
                                 ? source_var.minimum_release()
                                 : threshold_fraction;
    // The target is never past the current position, so if that isn't far
    // enough, nothing can be released; skip scanning the holds.
    if (current - _base < threshold)
    {
        return;
    }
    // Text before the current position is finished with, except from the
    // earliest hold onwards.
    const size_t hold = earliest_hold();
    // Where the release target is; the minimum of the earliest hold and the
    // current position. Holds are never before _base.
    size_t target     = hold < current ? hold : current;
    // Based on where we want to release, does it meet the threshold?
    if (target - _base < threshold)
    {
        return;
    }
    // Threshold is met.
    // Got to keep context_chars for the context if printing output.
    target = back_up_chars(target, context_chars());
    // Check again. back_up_chars could be done earlier, but its more
    // computationally expensive than working out the above, so its given its
    // own check after calculation.
    if (target - _base < threshold)
    {
        return;
    }
    // Release the text.
    source_var.release_text(target - _base);
    // Reload the data, _base and _index.
    _base = source_var.offset();
    reload();
    _index = current - _base;
}

template <typename Parser, typename T>
requires parser_c<Parser, T>
constexpr basic_parser_return_value
    parser_input::run_parser(
        const Parser& parser_arg,
        T&            value_arg
    )
{
    basic_parser_return_value result
        = parser_arg.run_parser(*this, value_arg, parser_run_key());
    if (result.succeeded())
    {
        release_consumed();
    }
    return result;
}

constexpr void
    parser_input::reset(
        const input_mark&           mark_arg,
        const std::source_location& source_location_arg
    )
{
    using enum mark_problem_enum;
    const parser_input* owner = mark_arg.input();
    if (owner != this)
    {
        throw_error(parse_result_error::invalid_parser_foreign_mark(
            position(),
            owner == nullptr ? moved_from : other_input,
            mark_arg._source_location,
            source_location_arg
        ));
    }
    // Just a change of position. The entire point of input_mark is to hold a
    // position, in case its needed in future (designed for alternatives).
    _index = mark_arg.position() - _base;
}

constexpr std::size_t
    parser_input::context_chars() const noexcept
{
    return _context_chars;
}

constexpr double
    parser_input::fraction_of_buffer_threshold() const noexcept
{
    return _fraction_of_buffer_threshold;
}

inline void
    parser_input::throw_error(
        parse_result_error&& code_arg
    )
{
    throw parse_exception(std::move(code_arg));
}

constexpr void
    parser_input::reload() noexcept
{
    using namespace std;
    // Note, this assumes that a _source is not nullptr.
    assert(_source != nullptr);
    const u8string_view text = _source->buffered();
    _data                    = text.data();
    _size                    = text.size();
    _valid                   = _source->valid_size();
}

constexpr bool
    parser_input::refill()
{
    // If there's no abstract source, or the source found invalid text, return
    // false. Nothing more can become valid.
    if (_source == nullptr || _source->stopped_at_invalid())
    {
        return false;
    }
    else
    {
        // read_more() returns whether the text grew.
        const bool text_grew = _source->read_more();
        // This isn't in a condition; we can't guarantee that even if text_grew
        // == false, the underlying text didn't change - it may change its
        // buffer for some reason. So reload the text.
        reload();
        return text_grew;
    }
}

constexpr bool
    parser_input::ensure_valid(
        const std::size_t end_arg
    )
{
    using namespace std;
    while (_valid < end_arg)
    {
        // If refill succeeds, it gets more input then the condition is checked.
        if (not refill())
        {
            // No more input is coming.
            if (_valid < _size)
            {
                const u8string_view bad_str(_data + _valid, _size - _valid);
                auto result = UNICODE_BRIDGE_NAMESPACE::next_char32(
                    bad_str.begin(), bad_str.end()
                );
                if (result.has_value()) [[unlikely]]
                {
                    // The source says the text is invalid, but using
                    // next_char32 gets us something valid.
                    throw std::logic_error(
                        "pnp internal error: source says text is invalid "
                        "UTF-8. However, next_char32 returning success. This "
                        "is a bug in pnp; please report it."
                    );
                }
                else
                {
                    throw_error(parse_result_error::invalid_utf8(
                        _base + _valid, result.error()
                    ));
                }
            }
            else
            {
                // What we have is valid, we just couldn't get what was asked
                // for.
                return false;
            }
        }
    }
    return true;
}

constexpr bool
    parser_input::matches_at(
        const std::size_t        offset_arg,
        const std::u8string_view token_arg
    )
{
    using namespace std;
    const size_t start = _index + offset_arg;
    const size_t stop  = start + token_arg.size();
    // The most likely case; just check they are the same.
    if (stop <= _valid) [[likely]]
    {
        return u8string_view(_data + start, token_arg.size()) == token_arg;
    }
    // The code below is to ensure that the minimum number of bytes are used to
    // check the function. For sources such as std::cin, this allows the
    // function to terminate as early as possible.
    size_t matched = 0;
    while (true)
    {
        const size_t index_to_check_to = _valid < stop ? _valid : stop;
        // Checks one byte at a time, so a mismatch is found as soon as it
        // arrives.
        for (; start + matched < index_to_check_to; ++matched)
        {
            if (_data[start + matched] != token_arg[matched])
            {
                return false;
            }
        }

        if (matched == token_arg.size())
        {
            // The whole token has been matched.
            return true;
        }
        // Get the next valid character. Return false if it cannot.
        if (not ensure_valid(_valid + 1))
        {
            return false;
        }
    }
}

template <typename Predicate>
requires std::predicate<Predicate&, char8_t>
constexpr std::size_t
    parser_input::scan_while(
        Predicate& predicate_arg
    )
{
    using namespace std;
    size_t end = _index;
    while (true)
    {
        while (end < _valid)
        {
            if (not predicate_arg(_data[end]))
            {
                // Stopping on a continuation byte would split a character, so
                // the predicate is faulty.
                if (UNICODE_BRIDGE_NAMESPACE::
                        UNICODE_BRIDGE_NAMESPACE_INTERNAL::
                            is_utf8_continuation_byte(_data[end])) [[unlikely]]
                {
                    throw_error(
                        parse_result_error::invalid_parser_split_character(
                            _base + end, _data[end]
                        )
                    );
                }
                else
                {
                    return end;
                }
            }
            ++end;
        }
        // Everything validated so far was accepted; get more, if there is any.
        if (not ensure_valid(end + 1))
        {
            return end;
        }
    }
}

template <typename Predicate>
requires std::predicate<Predicate&, char32_t>
constexpr std::size_t
    parser_input::scan_chars_while(
        Predicate& predicate_arg
    )
{
    using namespace std;
    using namespace UNICODE_BRIDGE_NAMESPACE;
    size_t end = _index;
    while (true)
    {
        while (end < _valid)
        {
            const char8_t byte = _data[end];
            if (is_valid_ascii(byte))
            {
                if (not predicate_arg(static_cast<char32_t>(byte)))
                {
                    return end;
                }
                ++end;
                continue;
            }
            else
            {
                const u8string_view rest(_data + end, _valid - end);
                const auto          next_char
                    = next_char32_no_error(rest.begin(), rest.end());
                if (not next_char.has_value()) [[unlikely]]
                {
                    throw std::logic_error(
                        "pnp internal error: text checked as valid UTF-8, but "
                        "next_char32 returned an error. This is a bug in pnp; "
                        "please report it."
                    );
                }
                else
                {
                    if (not predicate_arg(next_char.value().first))
                    {
                        return end;
                    }
                    end += next_char.value().second;
                }
            }
        }
        // ensure_valid grabs the next whole Unicode character.
        if (not ensure_valid(end + 1))
        {
            return end;
        }
    }
}

constexpr std::optional<std::size_t>
    parser_input::scan_until(
        const std::u8string_view token_arg,
        const std::u8string_view escape_arg
    )
{
    using namespace std;
    constexpr size_t npos = u8string_view::npos;
    // An empty token is found straight away, and the text before it is empty.
    if (token_arg.empty())
    {
        return make_optional(_index);
    }
    // Function relies on the escape_arg and token_arg not having the same first
    // code unit.
    if (not escape_arg.empty() && escape_arg.front() == token_arg.front())
    {
        throw_error(parse_result_error::invalid_parser_escape_matches_token(
            position(), token_arg, escape_arg
        ));
    }
    // How far, from _index, the search has proceeded. Everything before checked
    // has been worked out - it holds no token, and it has not got part-way
    // through an escape. It's kept across reads, so after reading more, the
    // search can continue from where it stopped.
    size_t checked = 0;
    while (true)
    {
        // The text to be checked from the current position.
        const u8string_view text(_data + _index, _valid - _index);
        // token_at set once here, so the value can be cached over the
        // while-loop iterations.
        size_t token_at = text.find(token_arg.front(), checked);
        while (checked < text.size())
        {
            // Only re-calculate this if token_at has moved before checked. npos
            // is never less than checked, so no need to check against it.
            if (token_at < checked)
            {
                token_at = text.find(token_arg.front(), checked);
            }
            // The next place where an escape could start, if there are escapes.
            // Escapes have to be checked in order from checked, as repeated
            // escapes can cancel each other out; "\\"" is an escaped backslash
            // followed by a real quote. But "\"" is an escaped quote. This also
            // needs to be checked even when a token_arg hasn't been found - the
            // token may appear in the next batch of UTF-8 data.
            //
            // We only search up to the token, as one of the
            // requirements of the function is that the token_arg and the
            // escape_arg must not start with the same code units - so there's
            // no need to check token_at.
            const size_t escape_limit
                = token_at == npos ? text.size() : token_at;
            // Finds the first escape character, if escape_arg isn't empty.
            // Otherwise is npos.
            const size_t escape_at
                = escape_arg.empty() ? npos
                                     : text.substr(0, escape_limit)
                                           .find(escape_arg.front(), checked);
            // Whichever comes first is dealt with first. They can't be the
            // same, as they begin with different code units.
            const size_t next = token_at < escape_at ? token_at : escape_at;
            if (next == npos)
            {
                // Neither are in the text, so checked moved to the end and we
                // break out.
                checked = text.size();
                break;
            }
            // The text from that point on, to compare against.
            const u8string_view tail = text.substr(next);
            // From this point on either the escape or token has been found.
            // Whichever comes first is dealt with here.
            if (next == escape_at)
            {
                // If the rest of the escape_arg is next.
                if (tail.starts_with(escape_arg))
                {
                    // We can't see anything past the escape sequence, so leave
                    // checked at escape_at, so more input can (potentially) be
                    // gathered, so we can check for escape then token.
                    if (tail.size() == escape_arg.size())
                    {
                        checked = escape_at;
                        break;
                    }
                    else
                    {
                        // Escape has been found. We don't need to check for the
                        // token_arg next, we just need to set checked to one
                        // past the first Unicode character after the
                        // escape_arg. e.g. "hello\"goodbye". set checked to one
                        // past the end of " (so 'g') - that way, the next
                        // search can't match on the token_arg.
                        checked = next + escape_arg.size()
                                  + PNP_INTERNAL_NAMESPACE::sequence_length(
                                      tail[escape_arg.size()]
                                  );
                        continue;
                    }
                }
                if (escape_arg.starts_with(tail))
                {
                    // The start of an escape, but we need more information
                    // before we can match with it.
                    checked = next;
                    break;
                }
                // Not a match at all, only first code unit matched.
                checked = next + 1;
                continue;
            }
            // (next == token_at). But it's the only possible thing, so we use
            // else.
            else
            {
                if (tail.starts_with(token_arg))
                {
                    // Can exit function, we've found what we're looking for.
                    return make_optional(_index + next);
                }
                if (token_arg.starts_with(tail))
                {
                    // Start of the token, but unsure whether it's actually the
                    // token_arg. Needs more input.
                    checked = next;
                    break;
                }
                // Not the token, only the first character matched.
                checked = next + 1;
            }
            // All cases have been accounted for.
        }
        // All checked text has been searched, or something has been cut off.
        // Ask for more characters, or end the input.
        if (not ensure_valid(_index + text.size() + 1))
        {
            return nullopt;
        }
    }
}

constexpr void
    parser_input::add_hold(
        const std::size_t position_arg
    )
{
    _holds.push_back(position_arg);
}

constexpr void
    parser_input::remove_hold(
        const std::size_t position_arg
    ) noexcept
{
    // This function uses assertions, as remove_hold is called from destructors.
    // Though if it were not, the assertions would be exceptions.
    // If empty something has gone wrong here.
    if (_holds.empty())
    {
        assert(false && "parser_input::remove_hold: no holds to remove");
        return;
    }
    // The most likely thing is that the most recently added hold is removed.
    if (_holds.back() == position_arg)
    {
        _holds.pop_back();
        return;
    }
    // Moving backwards, so current_itt starts at end()-1 then is decremented in
    // each loop. Offset at -1 as the last element has been checked, so start
    // before it.
    auto       current_itt = _holds.end() - 1;
    const auto start_itt   = _holds.begin();
    while (current_itt != start_itt)
    {
        --current_itt;
        if (*current_itt == position_arg)
        {
            _holds.erase(current_itt);
            return;
        }
    }
    // If at this point, an attempt is being made to remove an element from
    // _holds that doesn't exist. This shouldn't be possible.
    assert(false && "parser_input::remove_hold: no hold at that position");
}

constexpr input_text
    parser_input::make_text(
        const std::size_t index_arg,
        const std::size_t length_arg
    )
{
    using namespace std;
    auto str = u8string_view(_data + index_arg, length_arg);
    if (_source == nullptr)
    {
        return input_text(str, nullptr, nullptr);
    }
    // Pin the storage the text is in, so it stays in memory while the
    // input_text lives.
    pinnable_storage* const storage = _source->_current_storage;
    if (storage != nullptr)
    {
        ++storage->pins;
    }
    return input_text(str, _source, storage);
}

PNP_INTERNAL_NS_BEGIN
constexpr std::size_t
    sequence_length(
        const char8_t char_arg
    )
{
    auto uc_result = UNICODE_BRIDGE_NAMESPACE::
        UNICODE_BRIDGE_NAMESPACE_INTERNAL::utf8_sequence_length(char_arg);
    return uc_result == 0 ? 1 : uc_result;
}

PNP_INTERNAL_NS_END
PNP_NS_END

// append_focused_string_view stays here, commented out, until error messages
// are revisited. When it's revived, _complete_string becomes data() and
// _cur_itt becomes the current index.

/*constexpr void
    parser_input::append_focused_string_view(
        std::u8string&    str_to_append_to_arg,
        const std::size_t minimum_chars_to_look_ahead_to_arg,
        const std::size_t minimum_chars_to_look_behind_at_arg
    ) const noexcept
{
    using namespace std;
    using namespace unicode_bridge;
    auto normalised_min_focus = _minimum_focus == 0 ? 1 : _minimum_focus;
    auto minimum_chars_to_look_ahead_to
        = std::max(minimum_chars_to_look_ahead_to_arg, normalised_min_focus);
    auto minimum_chars_to_look_behind_at
        = std::max(minimum_chars_to_look_behind_at_arg, normalised_min_focus);
    str_to_append_to_arg.append(u8"At position ");
    auto position        = std::distance(_complete_string.begin(), _cur_itt);
    auto position_as_str = std::to_string(position);
    str_to_append_to_arg.append(cast_string_view<char8_t>(position_as_str));
    str_to_append_to_arg.append(u8". Parser context ");
    if (_complete_string.empty())
    {
        str_to_append_to_arg.append(u8"(empty string, showing before/current): "
                                    u8"<string begin> / <string end>");
    }
    else if (at_end())
    {
        str_to_append_to_arg.append(
            u8"(past end of string, showing before/current): \""
        );
        const auto& [left_cutoff, right_cutoff, str_view] = make_focused_string(
            _complete_string,
            position - 1,
            1,
            minimum_chars_to_look_behind_at - 1,
            0
        );
        if (left_cutoff)
        {
            str_to_append_to_arg.append(u8"...");
        }
        str_to_append_to_arg.append(str_view);
        str_to_append_to_arg.append(u8"\" / <string end>");
    }
    else
    {
        str_to_append_to_arg.append(u8"(showing before/current/after): ");
        if (position == 0)
        {
            str_to_append_to_arg.append(u8"<string begin>");
        }
        else
        {
            str_to_append_to_arg.append(u8"\"");
            const auto& [left_cutoff_before, right_cutoff_before,
str_view_before] = make_focused_string( _complete_string, position - 1, 1,
                    minimum_chars_to_look_behind_at - 1,
                    0
                );
            if (left_cutoff_before)
                str_to_append_to_arg.append(u8"...");
            str_to_append_to_arg.append(u8"\"");
            str_to_append_to_arg.append(str_view_before);
            str_to_append_to_arg.append(u8"\"");
        }

        str_to_append_to_arg.append(u8" / \"");
        const auto& [left_cutoff_current, right_cutoff_current,
str_view_current] = make_focused_string(_complete_string, position, 1, 0, 0);
        str_to_append_to_arg.append(str_view_current);
        str_to_append_to_arg.append(u8"\" / ");
        const auto& [left_cutoff_after, right_cutoff_after, str_view_after]
            = make_focused_string(
                _complete_string,
                position + 1,
                1,
                0,
                minimum_chars_to_look_ahead_to - 1
            );
        if (str_view_after.empty())
        {
            str_to_append_to_arg.append(u8"<string end>");
        }
        else
        {
            str_to_append_to_arg.append(u8"\"");
            str_to_append_to_arg.append(str_view_after);
            if (right_cutoff_after)
            {
                str_to_append_to_arg.append(u8"...");
            }
            str_to_append_to_arg.append(u8"\"");
        }
    }
}*/
