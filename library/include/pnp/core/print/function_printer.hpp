#pragma once

#include "pnp/internal/macros.hpp"
#include "pnp/core/print/text_sink.hpp"

#include <concepts>
#include <type_traits>
#include <utility>

PNP_NS_BEGIN

/*!
 * @brief A callable that can be a printer's body: called as
 * func(const T&, text_sink&), through a const reference, returning nothing.
 */
template <typename T, typename F>
concept function_printer_callable_c
    = requires (const F& func_arg, const T& object_arg, text_sink& sink_arg) {
          { func_arg(object_arg, sink_arg) } -> std::same_as<void>;
      };
PNP_INTERNAL_NS_BEGIN

// These are in the internal namespace, as it is intended for the user to write
// auto fp = function_printer<int>([](args){...}); instead of having to name
// the callable type.

/*!
 * @brief Allows the user to create printers from any callable function.
 * @tparam T The type the printer prints.
 * @tparam F The function type.
 */
template <typename T, typename F>
requires function_printer_callable_c<T, F>
struct func_printer
{
public:
    /*!
     * @brief Constructor for lvalue and rvalue arguments.
     *
     * Constraints ensure that it doesn't overlap with copy constructors.
     *
     * @param callable_arg The callable object used internally as the object's
     * printer. It matches both lvalues (copied) and rvalues (moved).
     */
    template <typename U>
    requires (not std::same_as<std::remove_cvref_t<U>, func_printer<T, F>>)
             && std::constructible_from<F, U&&>
    constexpr explicit func_printer(U&& callable_arg)
        noexcept(std::is_nothrow_constructible_v<F, U&&>);
    /*!
     * @brief Runs the internal function object as a printer.
     *
     * Anything the function throws passes through unchanged.
     *
     * @param object_arg The object to print.
     * @param sink_arg The text_sink to print to.
     */
    constexpr void
        run_printer(const T& object_arg, text_sink& sink_arg) const;
private:
    F _callable;
};

PNP_INTERNAL_NS_END
/*!
 * @brief Helper function to create a function_printer object. By using this
 * function, the user doesn't have to know the type of the callable object.
 * @tparam T The type parameter of the internal printer.
 * @tparam F The type parameter of the callable type.
 * @param callable_arg The callable object.
 * @return A func_printer containing callable_arg as a captured callable object.
 */
template <typename T, typename F>
requires function_printer_callable_c<T, std::decay_t<F>>
constexpr auto
    function_printer(F&& callable_arg);
PNP_NS_END

PNP_NS_BEGIN
PNP_INTERNAL_NS_BEGIN

template <typename T, typename F>
    requires function_printer_callable_c<T, F>
template <typename U>
    requires (not std::same_as<std::remove_cvref_t<U>, func_printer<T, F>>)
&& std::constructible_from<F, U&&>
constexpr func_printer<T, F>::func_printer(
    U&& callable_arg
) noexcept(std::is_nothrow_constructible_v<F, U&&>)
    : _callable(std::forward<U>(callable_arg))
{}

template <typename T, typename F>
requires function_printer_callable_c<T, F>
constexpr void
    func_printer<T, F>::run_printer(
        const T&   object_arg,
        text_sink& sink_arg
    ) const
{
    _callable(object_arg, sink_arg);
}

PNP_INTERNAL_NS_END

template <typename T, typename F>
requires function_printer_callable_c<T, std::decay_t<F>>
constexpr auto
    function_printer(
        F&& callable_arg
    )
{
    // Decay_t removes any const/volatile, references etc on F, as well as
    // turning a function into a function pointer.
    return PNP_INTERNAL_NAMESPACE::func_printer<T, std::decay_t<F>>(
        std::forward<F>(callable_arg)
    );
}

PNP_NS_END