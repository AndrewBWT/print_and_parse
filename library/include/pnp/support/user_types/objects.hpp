#pragma once
// printer_c, parser_c.
#include "pnp/core/concepts.hpp"
// ASSUMED paths: default_parser, default_printer, print(), make_placeholder,
// and the std::tuple pnps.
#include "pnp/core/parse/default_parser.hpp"
#include "pnp/core/parse/parser_input.hpp"
#include "pnp/core/parse/placeholder.hpp"
#include "pnp/core/print/default_printer.hpp"
// pnp namespaces.
#include "pnp/internal/macros.hpp"

// std::convertible_to.
#include <concepts>
// The name.
#include <string>
#include <string_view>
// The fields.
#include <tuple>
// std::type_identity, std::remove_cvref_t.
#include <type_traits>
// std::move.
#include <utility>

PNP_NS_BEGIN
PNP_INTERNAL_NS_BEGIN
/*!
 * @brief Whether T is a std::tuple.
 * @tparam T The type to check.
 */
template <typename T>
inline constexpr bool is_std_tuple_v = false;
template <typename... Ts>
inline constexpr bool is_std_tuple_v<std::tuple<Ts...>> = true;

/*!
 * @brief The tuple pnp_to_tuple() returns for T.
 * @tparam T The object's type.
 */
template <typename T>
using to_tuple_t
    = std::remove_cvref_t<decltype(pnp_to_tuple(std::declval<const T&>()))>;

/*!
 * @brief A tuple's element types, without references or const.
 * @tparam Tuple The tuple.
 */
template <typename Tuple>
struct tuple_values;

template <typename... Ts>
struct tuple_values<std::tuple<Ts...>>
{
    //! The element types, as values.
    using type = std::tuple<std::remove_cvref_t<Ts>...>;
};

PNP_INTERNAL_NS_END

/*!
 * @brief The tuple of values an object of type T is written as, and parsed
 * into: what pnp_to_tuple() returns, without references or const.
 * @tparam T The object's type.
 */
template <typename T>
using object_values_t = typename PNP_INTERNAL_NAMESPACE::tuple_values<
    PNP_INTERNAL_NAMESPACE::to_tuple_t<T>>::type;

/*!
 * @brief Whether T is written and parsed as an object: its name, then its
 * fields as a tuple, e.g. rectangle(1, 2).
 *
 * T needs two functions, found by argument-dependent lookup:
 * - pnp_name(std::type_identity<T>), returning its name, convertible to
 *   std::u8string_view.
 * - pnp_to_tuple(const T&), returning its fields as a std::tuple. Return
 *   std::tuple{...} for values, such as the results of getters, or
 *   std::tie(...) for its members.
 *
 * Writing pnp_name is what opts T in: pnp_to_tuple alone does nothing. Once
 * both exist, default_printer<T> and default_parser<T> use object_pnp<T>.
 *
 * A T is built back from its fields with std::make_from_tuple, which calls a
 * constructor taking the fields in order, or aggregate initialises a struct.
 * If that doesn't fit, also provide pnp_from_tuple(std::type_identity<T>,
 * object_values_t<T>&&), returning a T.
 *
 * @tparam T The type to check.
 */
template <typename T>
concept object_c = requires (const T& object_arg) {
    {
        pnp_name(std::type_identity<T>{})
    } -> std::convertible_to<std::u8string_view>;
    pnp_to_tuple(object_arg);
} && PNP_INTERNAL_NAMESPACE::is_std_tuple_v<PNP_INTERNAL_NAMESPACE::to_tuple_t<T>>;

template <
    typename T,
    typename TuplePrinter = default_printer<object_values_t<T>>,
    typename TupleParser  = default_parser<object_values_t<T>>>
requires object_c<T>
struct object_pnp
{
public:
    /*!
     * @brief Prints object_arg: its name, then its fields.
     * @param object_arg The object to print.
     * @param sink_arg Where it's written.
     */
    void
        run_printer(const T& object_arg, text_sink& sink_arg) const;
    /*!
     * @brief Parses an object into object_arg.
     *
     * On failure, object_arg is unchanged.
     *
     * @param input_arg The input.
     * @param object_arg Where the parsed object is stored.
     * @return Success, or the failure of the part that didn't parse.
     */
    basic_parser_return_value
        run_parser(parser_input& input_arg, T& object_arg, parser_run_key)
            const;
    //! Prints the fields.
    TuplePrinter tuple_printer{};
    //! Parses the fields.
    TupleParser tuple_parser{};
private:
    /*!
     * @brief Builds a T from its parsed fields: with pnp_from_tuple if there
     * is one, or std::make_from_tuple.
     * @param values_arg The fields.
     * @return The object.
     */
    static T
        build(object_values_t<T>&& values_arg);
};
PNP_SUPPORT_NS_BEGIN
template <typename T>
inline constexpr bool enable_default_object_pnp = false;
PNP_SUPPORT_NS_END

/*!
 * @brief The default printer for an object that has opted in; see
 * enable_default_object_pnp.
 * @tparam T The object's type.
 */
template <typename T>
requires PNP_SUPPORT_NAMESPACE::enable_default_object_pnp<T>
struct default_printer<T> : public object_pnp<T>
{
    using base_t = object_pnp<T>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

/*!
 * @brief The default parser for an object that has opted in; see
 * enable_default_object_pnp.
 * @tparam T The object's type.
 */
template <typename T>
requires PNP_SUPPORT_NAMESPACE::enable_default_object_pnp<T>
struct default_parser<T> : public object_pnp<T>
{
    using base_t = object_pnp<T>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END

PNP_NS_BEGIN


template <typename T, typename TuplePrinter, typename TupleParser>
requires object_c<T>
inline void
    object_pnp<T, TuplePrinter, TupleParser>::run_printer(
        const T&   object_arg,
        text_sink& sink_arg
    ) const
{
    sink_arg.write(std::u8string_view(pnp_name(std::type_identity<T>{})));
    // The tuple pnp prints values, so references from std::tie are copied
    // into them first.
    const object_values_t<T> values(pnp_to_tuple(object_arg));
    print(tuple_printer, values, sink_arg);
}

template <typename T, typename TuplePrinter, typename TupleParser>
requires object_c<T>
inline basic_parser_return_value
    object_pnp<T, TuplePrinter, TupleParser>::run_parser(
        parser_input& input_arg,
        T&            object_arg,
        parser_run_key
    ) const
{
    const std::u8string_view name(pnp_name(std::type_identity<T>{}));
    if (not input_arg.advance_if_starts_with(name))
    {
        return input_arg.fail(u8"Expected \"" + std::u8string(name) + u8"\".");
    }
    input_arg.skip_whitespace();
    object_values_t<T>        values = make_placeholder<object_values_t<T>>();
    basic_parser_return_value result
        = input_arg.run_parser(tuple_parser, values);
    if (result.failed())
    {
        return result;
    }
    // Built only once every field has parsed.
    object_arg = build(std::move(values));
    return result;
}

template <typename T, typename TuplePrinter, typename TupleParser>
requires object_c<T>
inline T
    object_pnp<T, TuplePrinter, TupleParser>::build(
        object_values_t<T>&& values_arg
    )
{
    if constexpr (requires {
                      pnp_from_tuple(
                          std::type_identity<T>{}, std::move(values_arg)
                      );
                  })
    {
        return pnp_from_tuple(std::type_identity<T>{}, std::move(values_arg));
    }
    else
    {
        return std::make_from_tuple<T>(std::move(values_arg));
    }
}

PNP_NS_END