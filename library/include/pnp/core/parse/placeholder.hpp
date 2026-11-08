#pragma once
#include "pnp/internal/macros.hpp"

#include <concepts>
#include <type_traits>

PNP_NS_BEGIN
/*!
 * @brief Specialise to give a type a value(), which acts as a default value for
 * the value-based parsers in pnp that require a dummy initial value.
 *
 * While std::is_default_constructible could be used, there are some cases where
 * objects in the std do not have default constructors. This allows the parsing
 * functions to work with those objects. There may also be types that the user
 * does not control, or does not want to have a default constructor - so a
 * placeholder_value is a viable alternative.
 *
 * The primary template has no definition, only specialisations count.
 *
 * When used in conjunction with make_placeholder, the specialisation of
 * placeholder_value will take precedence over any std::is_default_constructible
 * object.
 *
 * As with any template, a partial specialisation's constraints should match
 * what its value() needs.
 *
 * @tparam T The type of the created value.
 */
template <typename T>
struct placeholder_value;

/*!
 * @brief Concept with a value() function, representing a placeholder_value;
 * something that can be overridden by a parser. Checks that it has a valid
 * placeholder_value<T>::value() function, that returns a T object.
 *
 * The specialisation placeholder_value<T>::value() must create an object of
 * type T. As an example
 *
 * @code
 * template <>
 * struct pnp::placeholder_value<my_type>
 * {
 *     static constexpr my_type value() { return my_type(0); }
 * };
 * @endcode
 */
template <typename T>
concept has_placeholder_value_c = requires {
    { placeholder_value<T>::value() } -> std::same_as<T>;
};
/*!
 * @brief Generic concept that captures values that are either
 * std::is_default_constructible or have a placeholder_value specialisation.
 *
 * Cannot be an unbounded C array - no way to construct such an object.
 *
 * If T is a C array, its elements must be either std::is_default_constructible,
 * or have a placeholder_value specialisation.
 */
template <typename T>
concept parse_constructible_c
    = (not std::is_unbounded_array_v<T>)
      && (has_placeholder_value_c<std::remove_all_extents_t<T>>
          || std::is_default_constructible_v<std::remove_all_extents_t<T>>);

/*!
 * @brief Free function to create a placeholder value, for a type that either
 * has a placeholder_value, or is std::is_default_constructible, and is not a
 * C array.
 *
 * If a type is both, then the placeholder_value::value() function is used, not
 * the default constructor.
 *
 * @tparam T The type of value being created.
 * @return The created placeholder value.
 */
template <typename T>
requires parse_constructible_c<T> && ( not std::is_array_v<T> )
constexpr T make_placeholder();
PNP_NS_END

PNP_NS_BEGIN
template <typename T>
    requires parse_constructible_c<T> && (not std::is_array_v<T>)
constexpr T
    make_placeholder()
{
    if constexpr (has_placeholder_value_c<T>)
    {
        return placeholder_value<T>::value();
    }
    else
    {
        return T{};
    }
}

PNP_NS_END