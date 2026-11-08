#pragma once

#include "pnp/core/parse/parser_result.hpp"
#include "pnp/core/parse/parser_run_key.hpp"
#include "pnp/core/print/text_sink.hpp"
#include "pnp/internal/macros.hpp"

#include <concepts>

PNP_NS_BEGIN
// Forward declarations.
template <typename T>
struct default_printer;
template <typename T>
struct default_parser;
/*!
 * @brief Concept for printing.
 *
 * Requires the printer P to have a run_printer function that takes an object of
 * type T and a reference to a text_sink, and returns nothing.
 */
template <typename P, typename T>
concept printer_c
    = requires (const P& printer_arg, const T& obj_arg, text_sink& sink_arg) {
          { printer_arg.run_printer(obj_arg, sink_arg) } -> std::same_as<void>;
      };
/*!
 * @brief Concept for parsing.
 *
 * Check that the check_parser_takes_key's takes_key function returns true.
 *
 * See check_parser_takes_key.
 */
template <typename Parser, typename T>
concept parser_c = check_parser_takes_key::takes_key<Parser, T>;
/*!
 * @brief Concept for a default printer.
 *
 * User specialisations must declare `static constexpr bool is_specialized =
 * true`. The concept also checks that default_printer<T>
 * is a printer, and that default_printer is default initialisable.
 *
 */
template <typename T>
concept default_printable_c = default_printer<T>::is_specialized == true
                              && printer_c<default_printer<T>, T>
                              && std::default_initializable<default_printer<T>>;

/*!
 * @brief Concept for a default parser.
 *
 * User specialisations must declare `static constexpr bool is_specialized =
 * true`. The concept also checks that default_parser<T>
 * is a parser, and that default_parser is default initialisable.
 *
 */
template <typename T>
concept default_parseable_c = default_parser<T>::is_specialized == true
                              && parser_c<default_parser<T>, T>
                              && std::default_initializable<default_parser<T>>;

/*!
 * @brief A concept ensuring that a given T is both default_printable and
 * default_parseable.
 */
template <typename T>
concept default_printable_and_parseable_c
    = default_printable_c<T> && default_parseable_c<T>;
PNP_NS_END