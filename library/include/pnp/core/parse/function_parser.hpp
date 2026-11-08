#pragma once

#include "pnp/core/parse/parser_input.hpp"
#include "pnp/internal/macros.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

PNP_NS_BEGIN

/*!
 * @brief A callable that can be a parser's body: called as
 * func(parser_input&, T&), through a const reference, returning a
 * basic_parser_return_value.
 */
template <typename T, typename F>
concept function_parser_callable_c = requires (
    const F&      func_arg,
    parser_input& parser_input_arg,
    T&            object_ref_arg
) {
    {
        func_arg(parser_input_arg, object_ref_arg)
    } -> std::same_as<basic_parser_return_value>;
};
PNP_INTERNAL_NS_BEGIN

// These are in the internal namespace, as it is intended for the user to write.
// auto fp = function_parser<int>([](args){...}); instead of having to name the
// callable type.

/*!
 * @brief Allows user to create parsers from any callable function.
 * @tparam T The type parameter of the underlying parser.
 * @tparam F The function type.
 */
template <typename T, typename F>
requires function_parser_callable_c<T, F>
struct func_parser
{
public:
    /*!
     * @brief Constructor for lvalue and rvalue arguments.
     *
     * Constraints ensure that it doesn't overlap with copy constructors.
     *
     * @param callable_arg The callable object used internally as the object's
     * parser. It matches both lvalues (copied) and rvalues (moved).
     */
    template <typename U>
    requires (not std::same_as<std::remove_cvref_t<U>, func_parser<T, F>>)
             && std::constructible_from<F, U&&>
    constexpr explicit func_parser(U&& callable_arg)
        noexcept(std::is_nothrow_constructible_v<F, U&&>);
    /*!
     * @brief Runs the internal function object as a parser.
     * @param parser_input_arg The parser_input describing the status of the
     * parser.
     * @param object_ref_arg The object to override the resulting parse with.
     * @param run_key_arg Dummy parameter, used to ensure parsers aren't
     * misused. Not passed onto on the functions callable in func_parser, so
     * the key's job ends here.
     * @return The return value from the internal parser.
     */
    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            T&             object_ref_arg,
            parser_run_key run_key_arg
        ) const;
private:
    F _callable;
};

PNP_INTERNAL_NS_END
/*!
 * @brief Helper function to create a function_parser object. By using this
 * function, the user doesn't have to know the type of the callable object.
 * @tparam T The type parameter of the internal parser.
 * @tparam F The type parameter of the callable type.
 * @param callable_arg The callable object.
 * @return A func_parser containing callable_arg as a captured callable object.
 */
template <typename T, typename F>
requires function_parser_callable_c<T, std::decay_t<F>>
constexpr auto
    function_parser(F&& callable_arg);
PNP_NS_END

PNP_NS_BEGIN
PNP_INTERNAL_NS_BEGIN

template <typename T, typename F>
requires function_parser_callable_c<T, F>
template <typename U>
    requires (not std::same_as<std::remove_cvref_t<U>, func_parser<T,F>>)
&& std::constructible_from<F, U&&>
constexpr func_parser<T, F>::func_parser(U&& callable_arg) noexcept(std::is_nothrow_constructible_v<F, U&&>)
    : _callable(std::forward<U>(callable_arg))
{}

template <typename T, typename F>
requires function_parser_callable_c<T, F>
constexpr basic_parser_return_value
    func_parser<T, F>::run_parser(
        parser_input& parser_input_arg,
        T&            object_ref_arg,
        parser_run_key
    ) const
{
    return _callable(parser_input_arg, object_ref_arg);
}

PNP_INTERNAL_NS_END

template <typename T, typename F>
requires function_parser_callable_c<T, std::decay_t<F>>
constexpr auto
    function_parser(
        F&& callable_arg
    )
{
    // Decay_t removes any const/volatile, references etc on F, as well as
    // turning a function into a function pointer.
    return PNP_INTERNAL_NAMESPACE::func_parser<T, std::decay_t<F>>(
        std::forward<F>(callable_arg)
    );
}

PNP_NS_END