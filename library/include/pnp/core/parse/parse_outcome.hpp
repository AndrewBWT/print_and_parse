#pragma once
#include "pnp/core/parse/parser_result.hpp"
#include "pnp/core/parse/placeholder.hpp"

#include <cstddef>
#include <type_traits>
#include <utility>

PNP_NS_BEGIN

/*!
 * @brief Aggregate struct that contains data about the outcome of an attempted
 * parse.
 *
 * It has two success states - when the parser consumed all input and when it
 * didn't. It also has two error states - one when the parser returned an error,
 * and one when an error was reported by throwing a parse_exception.
 */
struct parse_outcome
{
    /*!
     * @brief The state that describes the outcome of the parser.
     */
    enum class parse_outcome_state_enum
    {
        //! The parser succeeded and all the input was consumed.
        success_and_all_input_consumed,
        //! The parser succeeded, however input remains.
        success_but_input_remains,
        //! The parser failed due to some safely returned error - e.g. when
        //! text was expected in the parser, but none found.
        failure_parser_error,
        //! The parser failed with some internal exception; e.g. malformed UTF-8
        //! input.
        failure_parser_exception,
    };

    //! Position in the input where the parser ended; on failure, where it
    //! failed. On success, a location determined by the parser or its
    //! controlling function(s). All the core functions in pnp greedily consume
    //! whitespace after a parse.
    std::size_t position = 0;
    //! The messages given from the parser. These are written from the bottom
    //! up; for a hierarchy of parsers that have failed, the lowest level error
    //! is at index 0. Should only be used when parse_outcome_state =
    //! failure_parser_error.
    basic_parser_return_value::messages_t parser_error_messages;
    //! Used to store information about where the parser was when it failed.
    parse_error_context parsers_error_context;
    //! The state of the parse_outcome.
    parse_outcome_state_enum parse_outcome_state
        = parse_outcome_state_enum::failure_parser_error;
    //! The error type for when the parser failed. Should only be used when
    //! parse_outcome_state = failure_parser_exception.
    parse_result_error::details_t error;
    /*!
     * @brief Tells the caller whether the value was parsed, and all input
     * consumed.
     * @return True if all input consumed and value parsed successfully, false
     * otherwise.
     */
    [[nodiscard]]
    constexpr bool
        succeeded() const noexcept;
    /*!
     * @brief Tells the caller whether the value was parsed, but with input
     * remaining.
     * @return True if the value parsed successfully but input remained,
     * false otherwise.
     */
    [[nodiscard]]
    constexpr bool
        succeeded_with_input_remaining() const noexcept;
    /*!
     * @brief Tells the caller whether the value was parsed, with the status of
     * the input ignored.
     * @return True if the value was parsed. False otherwise.
     */
    [[nodiscard]]
    constexpr bool
        parsed_value() const noexcept;
    /*!
     * @brief Tells the caller whether the parser failed, either by throwing an
     * exception or by a failed parse.
     * @return True if the parser experienced some error, false otherwise.
     */
    [[nodiscard]]
    constexpr bool
        failed() const noexcept;
};

/*!
 * @brief Compound object combining parse_outcome with a T object. Used in
 * functions such as try_parse, in parsing environments where there is no
 * initial value before the parse.
 *
 * The three constructors cover the following cases:
 * - Where T isn't a C array. make_placeholder<T>() initialises value: T's
 * placeholder if it has one, otherwise a value-initialised T.
 * - Where T is a C array, whose elements have no placeholder. The internal
 * elements are value initialised.
 * - Where T is a C array, whose elements have a placeholder. Each element is
 * initialised using the placeholder.
 *
 * @tparam T The type of the object parsed. Must meet the parse_constructible_c
 * concept.
 */
template <typename T>
requires parse_constructible_c<T>
struct parse_outcome_with_value
{
public:
    /*!
     * @brief Constructor for all T types that are not arrays.
     */
    constexpr parse_outcome_with_value()
    requires (not std::is_array_v<T>);
    /*!
     * @brief Constructor for C array whose elements have no placeholder - the
     * array's elements are value initialised.
     */
    constexpr parse_outcome_with_value()
    requires std::is_array_v<T>
             && (not has_placeholder_value_c<std::remove_all_extents_t<T>>);
    /*!
     * @brief Constructor for C array whose elements have a placeholder value
     * associated with them. Each element is initialised using its placeholder
     * value.
     *
     * Note that, for large array sizes, this can be computationally expensive
     * at compile time.
     */
    constexpr parse_outcome_with_value()
    requires std::is_array_v<T>
             && has_placeholder_value_c<std::remove_all_extents_t<T>>;
    //! The outcome of the parsing. See parse_outcome.
    parse_outcome outcome;
    //! The value containing the parsed result. See the separate constructors.
    T value;
private:
    /*!
     * @brief Constructor that the constructor above delegates to. Initialises a
     * C array's elements, using make_placeholder of the element type.
     * @tparam ...Indices One per element of the flattened array; only
     * their number is used.
     * @param index_sequence_arg Only used to deduce Indices.
     */
    template <std::size_t... Indices>
    constexpr explicit parse_outcome_with_value(
        std::index_sequence<Indices...> index_sequence_arg
    );
};

PNP_NS_END

PNP_NS_BEGIN
constexpr bool
    parse_outcome::succeeded() const noexcept
{
    using enum parse_outcome_state_enum;
    return parse_outcome_state == success_and_all_input_consumed;
}

constexpr bool
    parse_outcome::succeeded_with_input_remaining() const noexcept
{
    using enum parse_outcome_state_enum;
    return parse_outcome_state == success_but_input_remains;
}

constexpr bool
    parse_outcome::parsed_value() const noexcept
{
    return succeeded() || succeeded_with_input_remaining();
}

constexpr bool
    parse_outcome::failed() const noexcept
{
    using enum parse_outcome_state_enum;
    return parse_outcome_state == failure_parser_error
           || parse_outcome_state == failure_parser_exception;
}

template <typename T>
requires parse_constructible_c<T>
constexpr parse_outcome_with_value<T>::parse_outcome_with_value()
requires (not std::is_array_v<T>)
    : value(make_placeholder<T>())
{}

template <typename T>
    requires parse_constructible_c<T>
constexpr parse_outcome_with_value<T>::parse_outcome_with_value()
    requires std::is_array_v<T>
&& (not has_placeholder_value_c<std::remove_all_extents_t<T>>)
    : value()
{}

template <typename T>
requires parse_constructible_c<T>
constexpr parse_outcome_with_value<T>::parse_outcome_with_value()
requires std::is_array_v<T>
         && has_placeholder_value_c<std::remove_all_extents_t<T>>
    : parse_outcome_with_value(
          std::make_index_sequence<
              sizeof(T) / sizeof(std::remove_all_extents_t<T>)>{}
      )
{}

template <typename T>
             requires parse_constructible_c<T>
         template <std::size_t... Indices>
         constexpr parse_outcome_with_value<T>::parse_outcome_with_value(
             std::index_sequence<Indices...>
         )
             : value{ (static_cast<void>(Indices),
                      make_placeholder<std::remove_all_extents_t<T>>())... }
{}

PNP_NS_END