#pragma once

#include "pnp/core/parse/placeholder.hpp"
#include "pnp/internal/macros.hpp"

#include <cassert>
#include <concepts>
#include <cstdint>
#include <exception>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <unicode_bridge.hpp>
#include <utility>
#include <vector>


PNP_NS_BEGIN

/*!
 * @brief Aggregate struct that holds text_location information; specifically a
 * line and a column.
 */
struct text_location
{
    std::size_t line   = 1;
    std::size_t column = 1;
    /*!
     * @brief Returns the text_location just after text_arg, using the current
     * object as the position at the start of text_arg.
     *
     * Each '\n' in text_arg moves the new text_location to the next line, with
     * the column reset to 1. The column then counts the Unicode characters
     * after the last newline, or after the start of text_arg if it has none.
     *
     * @param text_arg The text used to create the new text_location.
     * @return The text_location just after text_arg.
     */
    [[nodiscard]]
    constexpr text_location
        after(const std::u8string_view text_arg) const noexcept;
};

/*!
 * @brief A struct that represents a context in a parser. Specifically, centered
 * around a single character. It has the previous n characters, the focal-point
 * character, and the next n characters, as well as bools saying whether there
 * was more text before/after the previous/next characters.
 *
 * For this object, "character" represents a Unicode character.
 */
struct parse_error_context
{
    //! The text before the focus of this object.
    std::u8string before_pos_text;
    //! The focal point in the object. Is a std::u8string, as a single
    //! Unicode scalar value may require multiple code units.
    std::u8string current_pos_text;
    //! The text after the focus of this object.
    std::u8string after_pos_text;
    //! The text before the focus point was cut off - it did not encounter the
    //! beginning of the text.
    bool before_text_cut_off = true;
    //! The text after the focus point was cut off - it did not encounter the
    //! end of the text.
    bool after_text_cut_off  = true;
    //! The location in the entire input of the focal point of the character in
    //! this object.
    text_location location;
};

/*!
 * @brief The type used to store a parser's result. It contains information
 * about error messages, and where the error (if any) was reported
 */
struct basic_parser_return_value
{
public:
    //! Type synonym used to represent a set of messages.
    using messages_t = std::vector<std::u8string>;
    /*!
     * @brief Default constructor. Used when returning successfully from a
     * parser.
     *
     * In those instances, written as "return {}".
     *
     * Failing error messages are written using the parser_input functions fail.
     */
    constexpr basic_parser_return_value();
    /*!
     * @brief Reports the position that the object holds. If set by a failing
     * parser, this will be some value representing a position in the text
     * input. If set by a succeeding parser, it will be 0.
     * @return The internal value of the position of the
     * basic_parser_return_value.
     */
    [[nodiscard]]
    constexpr std::size_t
        position() const noexcept;
    /*!
     * @brief Obtains the messages in the basic_parser_return_value object.
     *
     * Uses C++23's deducing this, so Self&& makes the object a
     * forward-referencing parameter, so Self records how the function was
     * called. Can return an lvalue, const lvalue or rvalue by using
     * object.messages(), std::as_const(object).messages() and
     * std::move(object).messages(), respectively.
     *
     * @tparam Self Used as the type of the object calling the function.
     * @param self The object of type Self.
     * @return Some form of messages_t. See above.
     */
    template <typename Self>
    [[nodiscard]]
    constexpr auto&&
        messages(this Self&& self) noexcept;
    /*!
     * @brief Reports whether the basic_parser_return_value object represents a
     * successful parse or not. It checks the internal messages; a non-zero size
     * means it was a failure, a size of zero means it was a success.
     * @return True if the basic_parser_return_value object represents a parsers
     * success. False otherwise.
     */
    [[nodiscard]]
    constexpr bool
        succeeded() const noexcept;
    /*!
     * @brief Reports whether the basic_parser_return_value object represents a
     * failed parse or not. It checks the internal messages; a size of zero
     * means it was a success. A non-zero size means it was a failure.
     * @return True if the basic_parser_return_value object represents a parsers
     * failure. False otherwise.
     */
    [[nodiscard]]
    constexpr bool
        failed() const noexcept;
    /*!
     * @brief Adds a message to the basic_parser_return_value object.
     *
     * Template type is designed to allow overloads of string literals, views
     * and string.
     *
     * Returns the object used as the argument - self_arg.
     *
     * e.g add_msg(u8"one").add_msg(u8"two").add_msg(u8"three").
     *
     * @tparam Self The type of this object - basic_parser_return_value.
     * @tparam S The type of the argument. Must be constructable into a
     * std::u8string.
     * @param self_arg The deducing this, self argument.
     * @param message_arg The argument itself.
     * @return The self argument.
     */
    template <typename Self, typename S>
    requires std::constructible_from<std::u8string, S&&>
    constexpr Self
        add_msg(this Self&& self_arg, S&& message_arg);
private:
    friend struct parser_input;
    //! Position of where the parser got up to in the input, or where its
    //! reported error originated from.
    std::size_t _position = 0;
    //! The errors that were raised during the attempted parse. Is a sequence as
    //! parsers can be nested, and when an error is created, an error for each
    //! parser is put in this structure.
    messages_t _messages;
    /*!
     * @brief Constructor for creating a basic_parser_return_value with a
     * position and an initial error message.
     *
     * Private, so only available to friend classes.
     *
     * Message argument must be convertable to a std::u8string.
     *
     * @tparam S The parameter of the message.
     * @param position_arg The position of the error.
     * @param message_arg The error message.
     */
    template <typename S>
    requires std::constructible_from<std::u8string, S&&>
    constexpr basic_parser_return_value(
        const std::size_t position_arg,
        S&&               message_arg
    );
};
/*!
 * @brief Enum used to describe the error behind a mark raising an error.
 */
enum class mark_problem_enum : uint8_t
{
    //! The mark has been moved using a move operator.
    moved_from,
    //! The mark's parser_input does not match the current parser_input.
    other_input
};

/*!
 * @brief Object that defines an error from a parser. It is designed to describe
 * errors that the normal control-flow in a parser wouldn't usually take into
 * account - e.g. invalid UTF-8, or a function called on parser_input putting it
 * into an invalid state.
 */
struct parse_result_error
{
public:
    /*!
     * @brief Enum used to describe errors.
     */
    enum class error_enum : uint8_t
    {
        //! Used when invalid UTF-8 is encountered.
        invalid_utf8,
        //! The function advance was called, however the argument pushed the
        //! parser_input's position beyond the end of the input.
        invalid_parser_advanced_past_end,
        //! The function peek_until was called, however the escape argument
        //! starts with the same first code unit as its token - the function
        //! cannot guarantee a correct result.
        invalid_parser_escape_matches_token,
        //! The function reset was given a mark from a different parser_input,
        //! or a moved-from mark.
        invalid_parser_foreign_mark,
        //! Error for when advance is called, however the number of code units
        //! puts the parser_input into a state where it is not on a character
        //! boundary.
        invalid_parser_advanced_into_character,
        //! Error for when peek_while is called, and the predicate accepted the
        //! first byte of a character, but rejected the continuation byte.
        invalid_parser_split_character
    };

    /*!
     * @brief Inner struct that contains the error information.
     */
    struct details_t
    {
        //! The enum describing the error.
        error_enum error_code = error_enum::invalid_utf8;
        //! The requested number of bytes. Used in
        //! invalid_parser_advanced_past_end and
        //! invalid_parser_advanced_into_character.
        std::size_t requested = 0;
        //! The number of available bytes in the input. Used in
        //! invalid_parser_advanced_past_end.
        std::size_t available = 0;
        //! The token string. Used in
        //! invalid_parser_escape_matches_token.
        std::u8string token;
        //! The escape string. Used in
        //! invalid_parser_escape_matches_token.
        std::u8string escape;
        //! Enum describing the mark error. Used in
        //! invalid_parser_foreign_mark.
        mark_problem_enum mark_problem = mark_problem_enum::other_input;
        //! The source location of where the mark was created. Used in
        //! invalid_parser_foreign_mark.
        std::source_location mark_location;
        //! The source location of where reset was called. Used in
        //! invalid_parser_foreign_mark.
        std::source_location reset_location;
        //! The invalid continuation byte that caused the error. Used in
        //! invalid_parser_advanced_into_character and
        //! invalid_parser_split_character
        char8_t char8 = u8'\0';
        //! The utf8_error raised. Optional used as next_char32_error does not
        //! have a default value. Used in invalid_utf8.
        std::optional<UNICODE_BRIDGE_NAMESPACE::next_char32_error> utf8_error;
        /*!
         * @brief Gets a text representation of the error, when given the
         * associated position_arg.
         * @param position_arg The position where the error was found.
         * @return A std::u8string representing the error.
         */
        constexpr std::u8string
            to_message(const std::size_t position_arg) const;
    };

    /*!
     * @brief Returns the position of the error.
     *
     * For invalid_utf8, this is the location where the invalid UTF-8 was found.
     *
     * For invalid_parser_advanced_into_character this is the landing point,
     * where advance attempted to move the parser to.
     *
     * For invalid_parser_advanced_past_end this is where the parser was before
     * advance was called.
     *
     * For invalid_parser_escape_matches_token this is where the parser was
     * before peek_until was called.
     *
     * For invalid_parser_foreign_mark, this is where the parser was before the
     * mark error was found.
     *
     * For invalid_parser_split_character, this is where the rejected
     * continuation byte is.
     *
     * It is kept separate from the other fields as parse_outcome has its own
     * position member variable, which is initialised with this value.
     *
     * @return The error's position.
     */
    [[nodiscard]]
    constexpr std::size_t
        position() const noexcept;
    /*!
     * @brief Returns to the caller the error's error_enum, describing the error
     * itself.
     * @return The error's error_enum.
     */
    [[nodiscard]]
    constexpr error_enum
        code() const noexcept;
    /*!
     * @brief Returns to the caller the internal details_t object, containing
     * the fields describing the error.
     * @return The error's details_t object.
     */
    [[nodiscard]]
    constexpr const details_t&
        details() const noexcept;
    /*!
     * @brief Gets a text representation of the error.
     * @return A std::u8string representing the error.
     */
    constexpr std::u8string
        to_message() const;
private:
    friend struct parser_input;
    //! The details of the error.
    details_t _details;
    //! The position of the error.
    std::size_t _position          = 0;
    /*!
     * @brief Default constructor. Kept private so that parse_result_error
     * objects can only be created by the private functions.
     */
    constexpr parse_result_error() = default;
    /*!
     * @brief Static constructor for the invalid_utf8 error.
     *
     * Used so that only the required fields are set.
     *
     * @param position_arg The position of the error.
     * @param error_arg The error describing the invalid UTF-8
     * @return The parse_result_error object containing the created error.
     */
    [[nodiscard]]
    static constexpr parse_result_error
        invalid_utf8(
            std::size_t                                        position_arg,
            const UNICODE_BRIDGE_NAMESPACE::next_char32_error& error_arg
        ) noexcept;
    /*!
     * @brief Static constructor for the invalid_parser_advanced_past_end error.
     *
     * Used so that only the required fields are set.
     *
     * @param position_arg The position of the parser before the error was
     * created.
     * @param requested_arg The number of requested code_units.
     * @param available_arg The number of available code units in the parsers
     * input.
     * @return The parse_result_error object containing the created error.
     */
    [[nodiscard]]
    static constexpr parse_result_error
        invalid_parser_advanced_past_end(
            std::size_t position_arg,
            std::size_t requested_arg,
            std::size_t available_arg
        ) noexcept;
    /*!
     * @brief Static constructor for the invalid_parser_escape_matches_token
     * error.
     *
     * Used so that only the required fields are set.
     *
     * @param position_arg The position of the parser before the error was
     * created.
     * @param token_arg The token string that causes this error.
     * @param escape_arg The escape string that causes this error.
     * @return The parse_result_error object containing the created error.
     */
    [[nodiscard]]
    static constexpr parse_result_error
        invalid_parser_escape_matches_token(
            std::size_t        position_arg,
            std::u8string_view token_arg,
            std::u8string_view escape_arg
        );
    /*!
     * @brief Static constructor for the invalid_parser_foreign_mark error.
     *
     * Used so that only the required fields are set.
     *
     * @param position_arg The position of the parser before the error was
     * created.
     * @param problem_arg The enum describing the mark error found.
     * @param mark_location_arg The location where the input_mark was created.
     * @param reset_location_arg The location where reset was called, and where
     * the error was raised.
     * @return The parse_result_error object containing the created error.
     */
    [[nodiscard]]
    static constexpr parse_result_error
        invalid_parser_foreign_mark(
            std::size_t                 position_arg,
            mark_problem_enum           problem_arg,
            const std::source_location& mark_location_arg,
            const std::source_location& reset_location_arg
        ) noexcept;
    /*!
     * @brief Static constructor for the invalid_parser_advanced_into_character
     * error.
     *
     * Used so that only the required fields are set.
     *
     * @param position_arg The position in the text where advance was attempting
     * to move the parser.
     * @param requested_arg The requested number of bytes advance asked for.
     * @param invalid_character_arg The continuation byte found.
     * @return The parse_result_error object containing the created error.
     */
    [[nodiscard]]
    static constexpr parse_result_error
        invalid_parser_advanced_into_character(
            const std::size_t position_arg,
            const std::size_t requested_arg,
            const char8_t     invalid_character_arg
        ) noexcept;
    /*!
     * @brief Static constructor for the invalid_parser_split_character error.
     *
     * Used so that only the required fields are set.
     *
     * @param position_arg The position of where the continuation byte was
     * found.
     * @param invalid_character_arg The continuation byte found.
     * @return The parse_result_error object containing the created error.
     */
    [[nodiscard]]
    static constexpr parse_result_error
        invalid_parser_split_character(
            const std::size_t position_arg,
            const char8_t     invalid_character_arg
        ) noexcept;
};

/*!
 * @brief Exception object for storing parse_result_error's as exceptions.
 */
struct parse_exception : public std::exception
{
public:
    /*!
     * @brief Constructor for parse_exception
     * @param error_arg The object used to create the exception.
     */
    constexpr explicit parse_exception(parse_result_error&& error_arg);
    /*!
     * @brief Exception's what function.
     *
     * Internally, this assigns text through the parse_result_error's
     * to_message() function. This can fail, in which case some placeholder text
     * is returned.
     *
     * @return A string describing the error. It is encoded in UTF-8.
     */
    const char*
        what() const noexcept override;
    /*!
     * @brief Getter for the internal parse_result_error member variable.
     * @return The object's internal parse_result_error member variable.
     */
    constexpr const parse_result_error&
        error() const noexcept;
private:
    //! Describing the error itself.
    parse_result_error _error;
    //! A cached string which describes the error in plain text.
    mutable std::string _what_cache;
};

PNP_NS_END

PNP_NS_BEGIN
constexpr text_location
    text_location::after(
        const std::u8string_view text_arg
    ) const noexcept
{
    text_location result  = *this;
    // Where the last line of text_arg starts: past the last newline, if any.
    std::size_t last_line = 0;
    std::size_t newline   = text_arg.find(u8'\n');
    while (newline != std::u8string_view::npos)
    {
        ++result.line;
        last_line = newline + 1;
        newline   = text_arg.find(u8'\n', last_line);
    }
    if (last_line != 0)
    {
        result.column = 1;
    }
    result.column
        += UNICODE_BRIDGE_NAMESPACE::count_chars(text_arg.substr(last_line));
    return result;
}

constexpr basic_parser_return_value::basic_parser_return_value()
{}

constexpr std::size_t
    basic_parser_return_value::position() const noexcept
{
    return _position;
}

template <typename Self>
constexpr auto&&
    basic_parser_return_value::messages(
        this Self&& self
    ) noexcept
{
    return std::forward<Self>(self)._messages;
}

constexpr bool
    basic_parser_return_value::succeeded() const noexcept
{
    return _messages.empty();
}

constexpr bool
    basic_parser_return_value::failed() const noexcept
{
    return not _messages.empty();
}

// Designed to cover the const char8_t*, u8string_view and u8string&& cases.
template <typename Self, typename S>
requires std::constructible_from<std::u8string, S&&>
constexpr Self
    basic_parser_return_value::add_msg(
        this Self&& self_arg,
        S&&         message_arg
    )
{
    assert(self_arg.failed() && "add_msg called on a successful result");
    self_arg._messages.emplace_back(std::forward<S>(message_arg));
    return std::forward<Self>(self_arg);
}

template <typename S>
requires std::constructible_from<std::u8string, S&&>
constexpr basic_parser_return_value::basic_parser_return_value(
    const std::size_t position_arg,
    S&&               message_arg
)
    : _position(position_arg)
{
    _messages.emplace_back(std::forward<S>(message_arg));
}

constexpr std::u8string
    parse_result_error::details_t::to_message(
        const std::size_t position_arg
    ) const
{
    using namespace std;
    auto append_number = [](std::u8string& out_arg, std::size_t value_arg)
    {
        auto str = std::to_string(value_arg);
        out_arg.append(UNICODE_BRIDGE_NAMESPACE::cast_ascii<char8_t>(str));
    };
    auto append_hex_byte = [](std::u8string& out_arg, const char8_t byte_arg)
    {
        constexpr std::u8string_view digits  = u8"0123456789ABCDEF";
        out_arg                             += u8"0x";
        out_arg.push_back(digits[static_cast<std::size_t>(byte_arg >> 4)]);
        out_arg.push_back(digits[static_cast<std::size_t>(byte_arg & 0x0F)]);
    };
    auto append_text =
        []<typename CharT>(
            std::u8string& out_arg, const std::basic_string_view<CharT> text_arg
        )
    {
        out_arg.append(UNICODE_BRIDGE_NAMESPACE::cast_ascii<char8_t>(text_arg));
    };
    auto append_location
        = [&](std::u8string& out_arg, const std::source_location& location_arg)
    {
        append_text(out_arg, std::string_view(location_arg.file_name()));
        out_arg += u8":";
        append_number(out_arg, location_arg.line());
        out_arg += u8":";
        append_number(out_arg, location_arg.column());
    };
    // Every message starts with where the error happened, so what() is
    // useful even without a parse_error_context.
    u8string result = u8"At code unit ";
    append_number(result, position_arg);
    result += u8": ";
    switch (error_code)
    {
    case error_enum::invalid_utf8:
        result += u8"the input is not valid UTF-8.";
        if (utf8_error.has_value())
        {
            result += u8" The UTF-8 decoder reported: ";
            append_text(result, basic_string_view(utf8_error->message()));
        }
        break;
    case error_enum::invalid_parser_advanced_past_end:
        result += u8"parser error: advance(";
        append_number(result, requested);
        result += u8") was called, but only ";
        append_number(result, available);
        result += u8" code units remain in the input.";
        break;
    case error_enum::invalid_parser_escape_matches_token:
        result += u8"parser error: peek_until was given the escape \"";
        result += escape;
        result += u8"\" and the token \"";
        result += token;
        result += u8"\", which begin with the same code unit. The "
                  u8"escape and "
                  u8"token must start differently.";
        break;
    case error_enum::invalid_parser_foreign_mark:
        result += mark_problem == mark_problem_enum::moved_from
                      ? u8"parser error: reset was given an input_mark "
                        u8"that "
                        u8"has been moved from."
                      : u8"parser error: reset was given an input_mark "
                        u8"created "
                        u8"by a different parser_input.";
        result += u8" The mark was created at ";
        append_location(result, mark_location);
        result += u8"; reset was called at ";
        append_location(result, reset_location);
        result += u8".";
        break;
    case error_enum::invalid_parser_advanced_into_character:
        result += u8"parser error: advance(";
        append_number(result, requested);
        result += u8") lands on continuation byte ";
        append_hex_byte(result, char8);
        result += u8", in the middle of a character. Advance by whole "
                  u8"characters.";
        break;
    case error_enum::invalid_parser_split_character:
        result += u8"parser error: the peek_while predicate accepted the "
                  u8"start "
                  u8"of a character, but rejected its continuation byte ";
        append_hex_byte(result, char8);
        result += u8". A peek_while predicate must accept or reject "
                  u8"all of a "
                  u8"character's bytes; use peek_chars_while to test "
                  u8"whole "
                  u8"characters.";
        break;
    }
    return result;
}

constexpr std::size_t
    parse_result_error::position() const noexcept
{
    return _position;
}

constexpr parse_result_error::error_enum
    parse_result_error::code() const noexcept
{
    return _details.error_code;
}

constexpr const parse_result_error::details_t&
    parse_result_error::details() const noexcept
{
    return _details;
}

constexpr std::u8string
    parse_result_error::to_message() const
{
    return _details.to_message(_position);
}

constexpr parse_result_error
    parse_result_error::invalid_utf8(
        const std::size_t                                  position_arg,
        const UNICODE_BRIDGE_NAMESPACE::next_char32_error& error_arg
    ) noexcept
{
    parse_result_error result;
    result._details.error_code = error_enum::invalid_utf8;
    result._position           = position_arg;
    result._details.utf8_error = error_arg;
    return result;
}

constexpr parse_result_error
    parse_result_error::invalid_parser_advanced_past_end(
        const std::size_t position_arg,
        const std::size_t requested_arg,
        const std::size_t available_arg
    ) noexcept
{
    parse_result_error result;
    result._details.error_code = error_enum::invalid_parser_advanced_past_end;
    result._position           = position_arg;
    result._details.requested  = requested_arg;
    result._details.available  = available_arg;
    return result;
}

constexpr parse_result_error
    parse_result_error::invalid_parser_escape_matches_token(
        const std::size_t        position_arg,
        const std::u8string_view token_arg,
        const std::u8string_view escape_arg
    )
{
    parse_result_error result;
    result._details.error_code
        = error_enum::invalid_parser_escape_matches_token;
    result._position       = position_arg;
    result._details.token  = token_arg;
    result._details.escape = escape_arg;
    return result;
}

constexpr parse_result_error
    parse_result_error::invalid_parser_foreign_mark(
        const std::size_t           position_arg,
        const mark_problem_enum     problem_arg,
        const std::source_location& mark_location_arg,
        const std::source_location& reset_location_arg
    ) noexcept
{
    parse_result_error result;
    result._details.error_code     = error_enum::invalid_parser_foreign_mark;
    result._position               = position_arg;
    result._details.mark_problem   = problem_arg;
    result._details.mark_location  = mark_location_arg;
    result._details.reset_location = reset_location_arg;
    return result;
}

constexpr parse_result_error
    parse_result_error::invalid_parser_advanced_into_character(
        const std::size_t position_arg,
        const std::size_t requested_arg,
        const char8_t     invalid_character_arg
    ) noexcept
{
    parse_result_error result;
    result._details.error_code
        = error_enum::invalid_parser_advanced_into_character;
    result._position          = position_arg;
    result._details.char8     = invalid_character_arg;
    result._details.requested = requested_arg;
    return result;
}

constexpr parse_result_error
    parse_result_error::invalid_parser_split_character(
        const std::size_t position_arg,
        const char8_t     invalid_character_arg
    ) noexcept
{
    parse_result_error result;
    result._details.error_code = error_enum::invalid_parser_split_character;
    result._position           = position_arg;
    result._details.char8      = invalid_character_arg;
    return result;
}

constexpr parse_exception::parse_exception(
    parse_result_error&& error_arg
)
    : _error(std::move(error_arg))
{}

inline const char*
    parse_exception::what() const noexcept
{
    using namespace UNICODE_BRIDGE_NAMESPACE;
    try
    {
        if (_what_cache.empty())
        {
            _what_cache = cast_string_view<char>(_error.to_message());
        }
        return _what_cache.c_str();
    }
    catch (...)
    {
        return "pnp::parse_exception when allocating memory for char* message.";
    }
}

constexpr const parse_result_error&
    parse_exception::error() const noexcept
{
    return _error;
}

PNP_NS_END