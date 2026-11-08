#pragma once
#include "pnp/core/concepts.hpp"
#include "pnp/core/parse/parser_input.hpp"

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

PNP_NS_BEGIN

/*!
 * @brief An object that represents an overridable parser. It holds a copy of
 * any parser for T under one type.
 *
 * It is designed to allow the user to change a parser during runtime.
 *
 * Copying a container copies the parser it holds using the parser's own copy
 * constructor.
 *
 * No move operations are declared deliberately: an rvalue is copied, so a
 * container always holds a parser. Defaulted moves would leave an empty one.
 *
 * @tparam T The type being parsed.
 */
template <typename T>
struct runtime_parser_container
{
public:
    /*!
     * @brief Default constructor. It holds the default_parser for T.
     *
     * There must be a default_parser for T for this constructor to be
     * available.
     */
    runtime_parser_container()
    requires default_parseable_c<T>;
    /*!
     * @brief Constructor creating an object that holds parser_arg internally.
     *
     * The parser cannot be a runtime_parser_container itself, must be copy
     * constructible, and must adhere to the parser_c concept.
     *
     * @tparam Parser The type of the parser.
     * @param parser_arg The Parser to initialise the object with.
     */
    template <typename Parser>
    requires (not std::same_as<Parser, runtime_parser_container<T>>)
             && parser_c<Parser, T> && std::copy_constructible<Parser>
    runtime_parser_container(Parser parser_arg);
    /*!
     * @brief Copy constructor.
     * @param other_arg The object to be copied into this object.
     */
    runtime_parser_container(const runtime_parser_container& other_arg);
    /*!
     * @brief Copy assignment operator.
     * @param other_arg The object to be copied to this object.
     * @return *this.
     */
    runtime_parser_container&
        operator=(const runtime_parser_container& other_arg);
    /*!
     * @brief Function to run the internal parser.
     * @param input_arg The parser_input required for the parser.
     * @param value_arg The reference to the object which will contain the
     * parsed value.
     * @param parser_key_arg The key from parser_input. Not used.
     * @return The result of the parse.
     */
    basic_parser_return_value
        run_parser(
            parser_input&  input_arg,
            T&             value_arg,
            parser_run_key parser_key_arg
        ) const;
private:
    /*!
     * @brief Abstract object which holds the parser itself. It is extended with
     * a concrete object below.
     */
    struct abstract_internal_parser
    {
        virtual ~abstract_internal_parser() = default;
        /*!
         * @brief Function to run the parser.
         * @param input_arg The parser_input.
         * @param value_arg A reference to the value to override.
         * @return The result of the parsing.
         */
        virtual basic_parser_return_value
            run_parser(parser_input& input_arg, T& value_arg) const
            = 0;
        /*!
         * @brief Function that copies the internal parser.
         * @return A copy of the parser.
         */
        virtual std::unique_ptr<abstract_internal_parser>
            clone() const = 0;
    };

    /*!
     * @brief Concrete object containing the parser itself.
     * @tparam Parser The type of the parser.
     */
    template <typename Parser>
    struct concrete_internal_parser final : abstract_internal_parser
    {
        /*!
         * @brief Main constructor.
         * @param parser_arg The object used to initialise the parser.
         */
        explicit concrete_internal_parser(Parser parser_arg);
        /*!
         * @brief Runs the parser.
         * @param input_arg The parser_input.
         * @param value_arg A reference to the value to override.
         * @return The parser's result.
         */
        basic_parser_return_value
            run_parser(parser_input& input_arg, T& value_arg) const override;
        /*!
         * @brief Creates a copy of the parser.
         * @return A copied value containing _parser.
         */
        std::unique_ptr<abstract_internal_parser>
            clone() const override;
        //! The Parser object itself.
        Parser _parser;
    };

    //! Contains the parser itself.
    std::unique_ptr<abstract_internal_parser> _parser;
};

PNP_NS_END

PNP_NS_BEGIN

template <typename T>
runtime_parser_container<T>::runtime_parser_container()
requires default_parseable_c<T>
    : runtime_parser_container(default_parser<T>{})
{}

template <typename T>
template <typename Parser>
requires (not std::same_as<Parser, runtime_parser_container<T>>)
         && parser_c<Parser, T> && std::copy_constructible<Parser>
runtime_parser_container<T>::runtime_parser_container(
    Parser parser_arg
)
    : _parser(std::make_unique<concrete_internal_parser<Parser>>(
          std::move(parser_arg)
      ))
{}

template <typename T>
runtime_parser_container<T>::runtime_parser_container(
    const runtime_parser_container& other_arg
)
    : _parser(other_arg._parser->clone())
{}

template <typename T>
runtime_parser_container<T>&
    runtime_parser_container<T>::operator=(
        const runtime_parser_container& other_arg
    )
{
    if (this != &other_arg)
    {
        _parser = other_arg._parser->clone();
    }
    return *this;
}

template <typename T>
basic_parser_return_value
    runtime_parser_container<T>::run_parser(
        parser_input& input_arg,
        T&            value_arg,
        parser_run_key
    ) const
{
    return _parser->run_parser(input_arg, value_arg);
}

template <typename T>
template <typename Parser>
runtime_parser_container<T>::concrete_internal_parser<Parser>::
    concrete_internal_parser(
        Parser parser_arg
    )
    : _parser(std::move(parser_arg))
{}

template <typename T>
template <typename Parser>
basic_parser_return_value
    runtime_parser_container<T>::concrete_internal_parser<Parser>::run_parser(
        parser_input& input_arg,
        T&            value_arg
    ) const
{
    return input_arg.run_parser(_parser, value_arg);
}

template <typename T>
template <typename Parser>
std::unique_ptr<typename runtime_parser_container<T>::abstract_internal_parser>
    runtime_parser_container<T>::concrete_internal_parser<Parser>::clone() const
{
    return std::make_unique<concrete_internal_parser>(_parser);
}

PNP_NS_END