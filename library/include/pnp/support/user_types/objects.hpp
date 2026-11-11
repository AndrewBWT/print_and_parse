#pragma once
// printer_c, parser_c.
#include "pnp/core/concepts.hpp"
// default_parser, parser_input, make_placeholder and parse_constructible_c.
#include "pnp/core/parse/default_parser.hpp"
#include "pnp/core/parse/parser_input.hpp"
#include "pnp/core/parse/placeholder.hpp"
// default_printer and text_sink.
#include "pnp/core/print/default_printer.hpp"
// pnp namespaces.
#include "pnp/internal/macros.hpp"

// std::convertible_to, std::constructible_from, std::assignable_from.
#include <concepts>
// The name.
#include <string>
#include <string_view>
// The fields, and std::make_from_tuple.
#include <tuple>
// std::remove_cvref_t, std::is_constructible_v.
#include <type_traits>
// std::declval, std::move.
#include <utility>
/*!
 * @file
 * @brief Functionality for writing printers and parsers for user-defined
 * objects.
 *
 * pnp provides object_pnp, which allows the user to quickly define
 * printer/parser functionality for their own objects. The hierarchy is quite
 * complicated, so we will describe the simple case first so the reader can
 * understand the idea.
 *
 * We strived to supply a simple way of doing this, and settled on the user
 * being able to convert their objects to/from tuples, then use printing/parsing
 * machinery from each component type. The idea being that pnp provides
 * printers/parsers for all the base types, so the user can build up their own
 * printers/parsers from the provided data types (or their own, if they want to
 * go down that route).
 *
 * object_pnp takes several parameters; T (the object), Traits (an object that
 * has the required to tuple functionality), and TuplePrinter and TupleParser
 * (the printers/parsers for the component types of T).
 *
 * The "from tuple" functionality is supplied by std::make_from_tuple.
 *
 * Traits defaults to default_object_traits, an object with a way of turning T
 * into a tuple, and a name for the class. It uses the free functions to_tuple
 * and consistent_name to provide this functionaliity.
 *
 * So, if the user provides those, they get object_pnp for free.
 *
 * default_printer/default_parser require the constexpr variable
 * enable_default_object_pnp to be true for T. If that is true, and there is a
 * object_pnp instance, then they get a default printer/default_parser for their
 * object.
 *
 * That's the common case. Traits can be the users own object, so they can
 * define an object_pnp with their own, bespoke ways of going to/from a tuple -
 * this way, different constructors can be used if required.
 *
 * The printers/parsers for the objects variables are defined as
 * default_printer/default_parser in object_pnp, though the objects themselves
 * are public variables, so the user can easily override them.
 */

PNP_NS_BEGIN

/*!
 * @brief The traits that make T an object: its name and its fields.
 *
 * The primary template is empty, so no type is an object until object_traits
 * is specialised for it. A specialisation needs, callable on a const
 * object_traits<T>:
 * - consistent_name(), returning T's name: anything convertible to
 *   std::u8string_view.
 * - to_tuple(const T&), returning T's fields as a std::tuple.
 *
 * See has_object_traits_c.
 *
 * @tparam T The object's type.
 */
template <typename T>
struct object_traits
{};

PNP_INTERNAL_NS_BEGIN
/*!
 * @brief Checks whether T is a tuple.
 * @tparam T The type to check.
 */
template <typename T>
inline constexpr bool is_std_tuple_v = false;
template <typename... Ts>
inline constexpr bool is_std_tuple_v<std::tuple<Ts...>> = true;

/*!
 * @brief Check whether T is a std::tuple, ignoring references and const.
 * @tparam T The type to check.
 */
template <typename T>
concept std_tuple_c = is_std_tuple_v<std::remove_cvref_t<T>>;
PNP_INTERNAL_NS_END

/*!
 * @brief Whether Checks that Traits matches the functions required of
 * object_traits.
 *
 * A const Traits needs:
 * - consistent_name(), returning T's name: anything convertible to
 *   std::u8string_view.
 * - to_tuple(const T&), returning T's fields as a std::tuple.
 *
 * Either may be static. Being an object doesn't mean T can be printed or
 * parsed; see object_printable_c and object_parsable_c.
 *
 * @tparam T The object's type.
 * @tparam Traits Gives the name and fields. Defaults to object_traits<T>.
 */
template <typename T, typename Traits = object_traits<T>>
concept has_object_traits_c
    = requires (const Traits& traits_arg, const T& object_arg) {
          traits_arg.consistent_name();
          requires std::convertible_to<
              decltype(traits_arg.consistent_name()),
              std::u8string_view>;
          traits_arg.to_tuple(object_arg);
          requires PNP_INTERNAL_NAMESPACE::std_tuple_c<
              decltype(traits_arg.to_tuple(object_arg))>;
      };

PNP_INTERNAL_NS_BEGIN
/*!
 * @brief A type synonym for the return type of to_tuple.
 * @tparam T The object's type.
 * @tparam Traits Gives the fields.
 */
template <typename T, typename Traits>
using traits_tuple_t = std::remove_cvref_t<
    decltype(std::declval<const Traits&>().to_tuple(std::declval<const T&>()))>;

/*!
 * @brief Type parameter in a tuple.
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

/*!
 * @brief Object which refers to the type of a to_tuple function. Can just give
 * the template params, and get back the type.
 *
 * e.g. object_values<rectangle, rectangle_info>::type = std::tuple<int,int>.
 */
template <typename T, typename Traits>
struct object_values
{
    //! Not an object.
    using type = void;
};

template <typename T, typename Traits>
requires has_object_traits_c<T, Traits>
struct object_values<T, Traits>
{
    //! The values of the fields.
    using type = typename tuple_values<traits_tuple_t<T, Traits>>::type;
};

/*!
 * @brief Whether T can be constructed from Tuple's elements as rvalues; that
 * is, whether std::make_from_tuple<T> works with a Tuple.
 * @tparam T The type to construct.
 * @tparam Tuple The tuple of values.
 */
template <typename T, typename Tuple>
inline constexpr bool constructible_from_tuple_v = false;
template <typename T, typename... Ts>
inline constexpr bool constructible_from_tuple_v<T, std::tuple<Ts...>>
    = std::is_constructible_v<T, Ts&&...>;
PNP_INTERNAL_NS_END

/*!
 * @brief Type synonym for an objets type values.
 *
 * void if T isn't an object with Traits; see has_object_traits_c.
 *
 * @tparam T The object's type.
 * @tparam Traits Gives the name and fields. Defaults to object_traits<T>.
 */
template <typename T, typename Traits = object_traits<T>>
using object_values_t =
    typename PNP_INTERNAL_NAMESPACE::object_values<T, Traits>::type;

/*!
 * @brief Everything required for a printable object.
 *
 * The values of its fields must be constructible from what Traits::to_tuple
 * returns, and TuplePrinter must print them.
 *
 * @tparam T The object's type.
 * @tparam Traits Gives the name and fields.
 * @tparam TuplePrinter Prints the fields.
 */
template <typename T, typename Traits, typename TuplePrinter>
concept object_printable_c
    = has_object_traits_c<T, Traits>
      && std::constructible_from<
          object_values_t<T, Traits>,
          PNP_INTERNAL_NAMESPACE::traits_tuple_t<T, Traits>>
      && printer_c<TuplePrinter, object_values_t<T, Traits>>;

/*!
 * @brief Everything required for a parsable object. Whether an object of type T
 * can be parsed with Traits and TupleParser.
 *
 * TupleParser must parse the values of its fields, those values must be parse
 * constructible, and T must be constructible from them with
 * std::make_from_tuple, and move assignable.
 *
 * @tparam T The object's type.
 * @tparam Traits Gives the name and fields.
 * @tparam TupleParser Parses the fields.
 */
template <typename T, typename Traits, typename TupleParser>
concept object_parsable_c
    = has_object_traits_c<T, Traits>
      && parser_c<TupleParser, object_values_t<T, Traits>>
      && parse_constructible_c<object_values_t<T, Traits>>
      && PNP_INTERNAL_NAMESPACE::
          constructible_from_tuple_v<T, object_values_t<T, Traits>>
      && std::assignable_from<T&, T>;

/*!
 * @brief A printer and parser for an object: its name, then its fields,
 * printed and parsed as a tuple.
 *
 * The fields are printed and parsed by TuplePrinter and TupleParser, so they
 * look exactly like a tuple: by default, rectangle(3,4). Configure the
 * brackets and separators by giving configured tuple pnps.
 *
 * run_printer is only available when object_printable_c holds, and run_parser
 * when object_parsable_c holds, so an object whose fields can be printed but
 * not parsed can still be printed.
 *
 * A failed parse leaves the output unchanged: the object is only built, and
 * stored, once its fields have all parsed.
 *
 * @tparam T The object's type.
 * @tparam Traits Gives the name and fields. Defaults to object_traits<T>.
 * @tparam TuplePrinter Prints the fields. Defaults to the default printer for
 * object_values_t<T, Traits>.
 * @tparam TupleParser Parses the fields. Defaults to the default parser for
 * object_values_t<T, Traits>.
 */
template <
    typename T,
    typename Traits       = object_traits<T>,
    typename TuplePrinter = default_printer<object_values_t<T, Traits>>,
    typename TupleParser  = default_parser<object_values_t<T, Traits>>>
requires has_object_traits_c<T, Traits>
struct object_pnp
{
public:
    /*!
     * @brief Prints object_arg: its name, then its fields.
     * @param object_arg The object to print.
     * @param sink_arg Where it's written.
     */
    void
        run_printer(const T& object_arg, text_sink& sink_arg) const
    requires object_printable_c<T, Traits, TuplePrinter>;
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
        run_parser(parser_input& input_arg, T& object_arg, parser_run_key) const
    requires object_parsable_c<T, Traits, TupleParser>;
    //! Gives the name and fields of an object.
    Traits traits{};
    //! Prints the fields.
    TuplePrinter tuple_printer{};
    //! Parses the fields.
    TupleParser tuple_parser{};
};

PNP_SUPPORT_NS_BEGIN
/*!
 * @brief Set to true to make object_pnp<T> the default printer and parser for
 * T.
 *
 * T must be an object with object_traits<T>; see has_object_traits_c. The
 * default printer exists if T can be printed that way, and the default parser
 * if it can be parsed; see object_printable_c and object_parsable_c.
 *
 * @tparam T The object's type.
 */
template <typename T>
inline constexpr bool enable_default_object_printer = false;
template <typename T>
inline constexpr bool enable_default_object_parser = false;
PNP_SUPPORT_NS_END

/*!
 * @brief The default printer for an object that has opted in; see
 * enable_default_object_pnp.
 * @tparam T The object's type.
 */
template <typename T>
requires PNP_SUPPORT_NAMESPACE::enable_default_object_printer<T>
         && object_printable_c<
             T,
             object_traits<T>,
             default_printer<object_values_t<T>>>
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
requires PNP_SUPPORT_NAMESPACE::enable_default_object_parser<T>
         && object_parsable_c<
             T,
             object_traits<T>,
             default_parser<object_values_t<T>>>
struct default_parser<T> : public object_pnp<T>
{
    using base_t = object_pnp<T>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END

PNP_NS_BEGIN

template <
    typename T,
    typename Traits,
    typename TuplePrinter,
    typename TupleParser>
requires has_object_traits_c<T, Traits>
inline void
    object_pnp<T, Traits, TuplePrinter, TupleParser>::run_printer(
        const T&   object_arg,
        text_sink& sink_arg
    ) const
requires object_printable_c<T, Traits, TuplePrinter>
{
    // auto keeps a returned std::u8string alive while it's viewed.
    const auto name_holder = traits.consistent_name();
    sink_arg.write(std::u8string_view(name_holder));
    // The tuple pnp prints values, so references from std::tie are copied
    // into values first.
    const object_values_t<T, Traits> values(traits.to_tuple(object_arg));
    print(tuple_printer, values, sink_arg);
}

template <
    typename T,
    typename Traits,
    typename TuplePrinter,
    typename TupleParser>
requires has_object_traits_c<T, Traits>
inline basic_parser_return_value
    object_pnp<T, Traits, TuplePrinter, TupleParser>::run_parser(
        parser_input& input_arg,
        T&            object_arg,
        parser_run_key
    ) const
requires object_parsable_c<T, Traits, TupleParser>
{
    // auto keeps a returned std::u8string alive while it's viewed.
    const auto               name_holder = traits.consistent_name();
    const std::u8string_view name(name_holder);
    if (not input_arg.advance_if_starts_with(name))
    {
        return input_arg.fail(u8"Expected \"" + std::u8string(name) + u8"\".");
    }
    input_arg.skip_whitespace();
    object_values_t<T, Traits> values
        = make_placeholder<object_values_t<T, Traits>>();
    basic_parser_return_value result
        = input_arg.run_parser(tuple_parser, values);
    if (result.failed())
    {
        return result;
    }
    // Built only once every field has parsed.
    object_arg = std::make_from_tuple<T>(std::move(values));
    return result;
}

PNP_NS_END