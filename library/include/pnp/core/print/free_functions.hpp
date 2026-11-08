#pragma once
#include "pnp/core/concepts.hpp"
#include "pnp/core/print/text_sink.hpp"
#include "pnp/sinks/string.hpp"
#include "pnp/sinks/ostream.hpp"
#include "pnp/internal/macros.hpp"

#include <ostream>
#include <string>
#include <type_traits>

PNP_NS_BEGIN
/*!
 * @brief A concept that ensures that Printer is a printer, and that Printer is
 * copy constructible.
 *
 * If Printer is not copy constructible, the stream() functions will still work.
 */
template <typename Printer, typename T>
concept stream_proxy_printer_c
    = printer_c<Printer, T> && std::copy_constructible<Printer>;

// Forward declarations.
template <typename T>
struct default_printer;
template <typename Printer, typename T>
requires stream_proxy_printer_c<Printer, T>
struct stream_proxy;

template <typename Printer, typename T>
requires stream_proxy_printer_c<Printer, T>
std::ostream&
    operator<<(
        std::ostream&                   stream_arg,
        const stream_proxy<Printer, T>& proxy_arg
    );
// End forward declarations.
/*!
 * @brief Function to print a value using a Printer to a text_sink.
 *
 * If the printer throws an exception, whatever was written before it stays in
 * sink_arg.
 *
 * @tparam Printer The type of the printer used.
 * @tparam T The type to be printed.
 * @param printer_arg The Printer.
 * @param value_arg The value.
 * @param sink_arg The text_sink.
 */
template <typename Printer, typename T>
requires printer_c<Printer, T>
constexpr void
    print(const Printer& printer_arg, const T& value_arg, text_sink& sink_arg);
/*!
 * @brief Prints a value T using the default_printer for T.
 *
 * If the printer throws an exception, whatever was written before it stays in
 * sink_arg.
 *
 * @tparam T The type of the value to be printed.
 * @param value_arg The value being printed.
 * @param sink_arg The text_sink used to print.
 */
template <typename T>
requires default_printable_c<T>
constexpr void
    print(const T& value_arg, text_sink& sink_arg);

/*!
 * @brief Prints an object to a text_sink.
 *
 * The Printer is given as a template argument, which must be default
 * constructible.
 *
 * If the printer throws an exception, whatever was written before it stays in
 * sink_arg.
 *
 * @tparam Printer The type of the printer used.
 * @tparam T The type of the value to be printed.
 * @param value_arg The value being printed.
 * @param sink_arg The text_sink used to print.
 */
template <typename Printer, typename T>
requires std::is_default_constructible_v<Printer> && printer_c<Printer, T>
constexpr void
    print(const T& value_arg, text_sink& sink_arg);
/*!
 * @brief Function to append the result of a printer to a String.
 *
 * String can be either std::u8string or std::string.
 *
 * If the printer throws an exception, whatever was written before it stays in
 * string_arg.
 *
 * @tparam Printer The type of the printer.
 * @tparam T The type of the value to be printed.
 * @tparam String The type of string used.
 * @param printer_arg The printer used to print.
 * @param value_arg The value to print.
 * @param string_arg The string to append the printed result to.
 */
template <typename Printer, typename T, typename String>
requires printer_c<Printer, T> && output_string_c<String>
constexpr void
    append_str(
        const Printer& printer_arg,
        const T&       value_arg,
        String&        string_arg
    );
/*!
 * @brief Function to append the result of a printer to a String.
 *
 * The default_printer for T is used.
 *
 * String can be either std::u8string or std::string.
 *
 * If the printer throws an exception, whatever was written before it stays in
 * string_arg.
 *
 * @tparam T The type of the value to be printed.
 * @tparam String The type of string used.
 * @param value_arg The value to print.
 * @param string_arg The string to append the printed result to.
 */
template <typename T, typename String>
requires default_printable_c<T> && output_string_c<String>
constexpr void
    append_str(const T& value_arg, String& string_arg);
/*!
 * @brief Function to append the result of a printer to a String.
 *
 * Printer must be default constructible.
 *
 * String can be either std::u8string or std::string.
 *
 * If the printer throws an exception, whatever was written before it stays in
 * string_arg.
 *
 * @tparam Printer The type of the printer.
 * @tparam T The type of the value to be printed.
 * @tparam String The type of string used.
 * @param value_arg The value to print.
 * @param string_arg The string to append the printed result to.
 */
template <typename Printer, typename T, typename String>
requires std::is_default_constructible_v<Printer> && printer_c<Printer, T>
         && output_string_c<String>
constexpr void
    append_str(const T& value_arg, String& string_arg);
/*!
 * @brief Gives the result of a Printer printing a value as a std::u8string.
 * @tparam Printer The type of the printer.
 * @tparam T The type of the value to be printed.
 * @param printer_arg The printer used.
 * @param value_arg The value used.
 * @return The result of running printer_arg on value_arg.
 */
template <typename Printer, typename T>
requires printer_c<Printer, T>
constexpr std::u8string
    to_u8string(const Printer& printer_arg, const T& value_arg);
/*!
 * @brief Gives the result of a printer printing a value as a std::u8string.
 *
 * The printer used is the default_printer for T.
 *
 * @tparam T The type of the value to be printed.
 * @param value_arg The value used.
 * @return The result of running the default_printer on value_arg.
 */
template <typename T>
requires default_printable_c<T>
constexpr std::u8string
    to_u8string(const T& value_arg);
/*!
 * @brief Gives the result of a printer printing a value as a std::u8string.
 *
 * Printer is given as a template argument, and must be default constructible.
 *
 * @tparam Printer The type of the printer.
 * @tparam T The type of the value to be printed.
 * @param value_arg The value used.
 * @return The result of running the default constructed printer on value_arg.
 */
template <typename Printer, typename T>
requires std::is_default_constructible_v<Printer> && printer_c<Printer, T>
constexpr std::u8string
    to_u8string(const T& value_arg);
/*!
 * @brief Gives the result of a Printer printing a value as a std::string.
 *
 * The std::string is encoded as a UTF-8 string.
 *
 * @tparam Printer The type of the printer.
 * @tparam T The type of the value to be printed.
 * @param printer_arg The printer used.
 * @param value_arg The value used.
 * @return The result of running printer_arg on value_arg, given as a
 * std::string.
 */
template <typename Printer, typename T>
requires printer_c<Printer, T>
constexpr std::string
    to_native_string(const Printer& printer_arg, const T& value_arg);
/*!
 * @brief Gives the result of a printer printing a value as a std::string.
 *
 * The std::string is encoded as a UTF-8 string.
 *
 * The printer used is the default_printer for T.
 *
 * @tparam T The type of the value to be printed.
 * @param value_arg The value used.
 * @return The result of running the default printer on value_arg, given as a
 * std::string.
 */
template <typename T>
requires default_printable_c<T>
constexpr std::string
    to_native_string(const T& value_arg);
/*!
 * @brief Gives the result of a printer printing a value as a std::string.
 *
 * The std::string is encoded as a UTF-8 string.
 *
 * Printer is given as a template argument, and must be default constructible.
 *
 * @tparam Printer The type of the printer.
 * @tparam T The type of the value to be printed.
 * @param value_arg The value used.
 * @return The result of running the default constructed Printer on value_arg,
 * given as a std::string.
 */
template <typename Printer, typename T>
requires std::is_default_constructible_v<Printer> && printer_c<Printer, T>
constexpr std::string
    to_native_string(const T& value_arg);
/*!
 * @brief A function that allows the sending of the result of a Printer to an
 * std::ostream.
 *
 * The output is encoded as UTF-8, and the bytes are written as that. A terminal
 * (e.g. default Windows) that does not understand UTF-8 may not display the
 * characters properly.
 *
 * If the printer throws an exception, whatever was written before it stays in
 * stream_arg.
 *
 * @tparam Printer The type of the printer used.
 * @tparam T The type of the value to be printed.
 * @param printer_arg The printer used to print.
 * @param value_arg The value being printed.
 * @param stream_arg The std::ostream where the printed string is sent to.
 */
template <typename Printer, typename T>
requires printer_c<Printer, T>
void
    stream(
        const Printer& printer_arg,
        const T&       value_arg,
        std::ostream&  stream_arg
    );
/*!
 * @brief A function that allows the sending of the result of a Printer to an
 * std::ostream.
 *
 * The default_printer for T is used.
 *
 * The output is encoded as UTF-8, and the bytes are written as that. A terminal
 * (e.g. default Windows) that does not understand UTF-8 may not display the
 * characters properly.
 *
 * If the printer throws an exception, whatever was written before it stays in
 * stream_arg.
 *
 * @tparam T The type of the value to be printed.
 * @param value_arg The value being printed.
 * @param stream_arg The std::ostream where the printed string is sent to.
 */
template <typename T>
requires default_printable_c<T>
void
    stream(const T& value_arg, std::ostream& stream_arg);
/*!
 * @brief A function that allows the sending of the result of a Printer to an
 * std::ostream.
 *
 * Printer is given as a template argument, and must be default constructible.
 *
 * The output is encoded as UTF-8, and the bytes are written as that. A terminal
 * (e.g. default Windows) that does not understand UTF-8 may not display the
 * characters properly.
 *
 * If the printer throws an exception, whatever was written before it stays in
 * stream_arg.
 *
 * @tparam Printer The type of the printer used.
 * @tparam T The type of the value to be printed.
 * @param value_arg The value being printed.
 * @param stream_arg The std::ostream where the printed string is sent to.
 */
template <typename Printer, typename T>
requires std::is_default_constructible_v<Printer> && printer_c<Printer, T>
void
    stream(const T& value_arg, std::ostream& stream_arg);

/*!
 * @brief A helper object used to send the text output from printing the value
 * of an object, allowing for idiomatic C++ code, when used with the to_stream
 * functions.
 *
 * std::cout << to_stream(1) << std::endl;
 * std::cout << to_stream<int_printer>(1) << std::endl;
 * std::cout << to_stream(int_printer{.use_hex = true}, 1) << std::endl;
 *
 * @tparam Printer The type of the Printer used to print the value.
 * @tparam T The type of the value itself.
 */
template <typename Printer, typename T>
requires stream_proxy_printer_c<Printer, T>
struct stream_proxy
{
public:
    /*!
     * @brief Constructor for stream_proxy.
     * @param printer_arg The printer used to print the object.
     * @param value_arg The object itself. A pointer is taken of the object - so
     * it should stay in memory for the duration stream_proxy is used.
     */
    constexpr stream_proxy(const Printer& printer_arg, const T& value_arg);
private:
    /*!
     * @brief Streaming operator for a stream_proxy. Used to send the input
     * printed using the printer to an std::ostream.
     *
     * If the printer throws an exception, whatever was written before it stays
     * in stream_arg.
     *
     * @param stream_arg The ostream where the output is sent to.
     * @param proxy_arg The stream_proxy object containing the printer and a
     * pointer to the value to be printed.
     * @return The std::ostream given as input. Defined in this manner, it
     * allows the chaining of operations.
     */
    friend std::ostream& operator<< <Printer, T>(
        std::ostream&       stream_arg,
        const stream_proxy& proxy_arg
    );
    //! The Printer object used to print.
    Printer _printer;
    //! Pointer to value to print.
    const T* _value;
};

/*!
 * @brief Creates a stream_proxy object containing the printer and value given
 * as arguments.
 * @tparam Printer The type of the printer.
 * @tparam T The type of the object to be printed.
 * @param printer_arg The printer argument.
 * @param value_arg The value to be printed. Must stay in memory for the
 * duration that the returned stream_proxy is used.
 * @return A stream_proxy object containing the printer and value, which can be
 * used to stream to an std::ostream.
 */
template <typename Printer, typename T>
requires stream_proxy_printer_c<Printer, T>
constexpr stream_proxy<Printer, T>
    to_stream(const Printer& printer_arg, const T& value_arg);
/*!
 * @brief Creates a stream_proxy object containing the default_printer for T and
 * the value given as an argument.
 * @tparam T The type of the object to be printed.
 * @param value_arg The value to be printed. Must stay in memory for the
 * duration that the returned stream_proxy is used.
 * @return A stream_proxy object containing the default_printer<T> and the
 * value, which can be used to stream to an std::ostream.
 */
template <typename T>
requires default_printable_c<T> && stream_proxy_printer_c<default_printer<T>, T>
constexpr stream_proxy<default_printer<T>, T>
    to_stream(const T& value_arg);
/*!
 * @brief Creates a stream_proxy object containing a default constructed printer
 * and a value given as an argument.
 * @tparam Printer The type of the printer. Must be default constructible.
 * @tparam T The type of the object to be printed.
 * @param value_arg The value to be printed. Must stay in memory for the
 * duration that the returned stream_proxy is used.
 * @return A stream_proxy object containing a default constructed Printer and
 * the value to be printed, which can be used to stream to an std::ostream.
 */
template <typename Printer, typename T>
requires std::is_default_constructible_v<Printer>
         && stream_proxy_printer_c<Printer, T>
constexpr stream_proxy<Printer, T>
    to_stream(const T& value_arg);

PNP_NS_END

PNP_NS_BEGIN
template <typename Printer, typename T>
requires printer_c<Printer, T>
constexpr void
    print(
        const Printer& printer_arg,
        const T&       value_arg,
        text_sink&     sink_arg
    )
{
    printer_arg.run_printer(value_arg, sink_arg);
}

template <typename T>
requires default_printable_c<T>
constexpr void
    print(
        const T&   value_arg,
        text_sink& sink_arg
    )
{
    print(default_printer<T>{}, value_arg, sink_arg);
}

template <typename Printer, typename T>
requires std::is_default_constructible_v<Printer> && printer_c<Printer, T>
constexpr void
    print(
        const T&   value_arg,
        text_sink& sink_arg
    )
{
    print(Printer{}, value_arg, sink_arg);
}

template <typename Printer, typename T, typename String>
requires printer_c<Printer, T> && output_string_c<String>
constexpr void
    append_str(
        const Printer& printer_arg,
        const T&       value_arg,
        String&        string_arg
    )
{
    string_sink sink(string_arg);
    print(printer_arg, value_arg, sink);
}

template <typename T, typename String>
requires default_printable_c<T> && output_string_c<String>
constexpr void
    append_str(
        const T& value_arg,
        String&  string_arg
    )
{
    append_str(default_printer<T>{}, value_arg, string_arg);
}

template <typename Printer, typename T, typename String>
requires std::is_default_constructible_v<Printer> && printer_c<Printer, T>
         && output_string_c<String>
constexpr void
    append_str(
        const T& value_arg,
        String&  string_arg
    )
{
    append_str(Printer{}, value_arg, string_arg);
}

template <typename Printer, typename T>
requires printer_c<Printer, T>
constexpr std::u8string
    to_u8string(
        const Printer& printer_arg,
        const T&       value_arg
    )
{
    std::u8string result;
    append_str(printer_arg, value_arg, result);
    return result;
}

template <typename T>
requires default_printable_c<T>
constexpr std::u8string
    to_u8string(
        const T& value_arg
    )
{
    return to_u8string(default_printer<T>{}, value_arg);
}

template <typename Printer, typename T>
requires std::is_default_constructible_v<Printer> && printer_c<Printer, T>
constexpr std::u8string
    to_u8string(
        const T& value_arg
    )
{
    return to_u8string(Printer{}, value_arg);
}

template <typename Printer, typename T>
requires printer_c<Printer, T>
constexpr std::string
    to_native_string(
        const Printer& printer_arg,
        const T&       value_arg
    )
{
    std::string result;
    append_str(printer_arg, value_arg, result);
    return result;
}

template <typename T>
requires default_printable_c<T>
constexpr std::string
    to_native_string(
        const T& value_arg
    )
{
    return to_native_string(default_printer<T>{}, value_arg);
}

template <typename Printer, typename T>
requires std::is_default_constructible_v<Printer> && printer_c<Printer, T>
constexpr std::string
    to_native_string(
        const T& value_arg
    )
{
    return to_native_string(Printer{}, value_arg);
}

template <typename Printer, typename T>
requires printer_c<Printer, T>
void
    stream(
        const Printer& printer_arg,
        const T&       value_arg,
        std::ostream&  stream_arg
    )
{
    ostream_sink sink(stream_arg);
    print(printer_arg, value_arg, sink);
}

template <typename T>
requires default_printable_c<T>
void
    stream(
        const T&      value_arg,
        std::ostream& stream_arg
    )
{
    stream(default_printer<T>{}, value_arg, stream_arg);
}

template <typename Printer, typename T>
requires std::is_default_constructible_v<Printer> && printer_c<Printer, T>
void
    stream(
        const T&      value_arg,
        std::ostream& stream_arg
    )
{
    stream(Printer{}, value_arg, stream_arg);
}

template <typename Printer, typename T>
requires stream_proxy_printer_c<Printer, T>
constexpr stream_proxy<Printer, T>::stream_proxy(
    const Printer& printer_arg,
    const T&       value_arg
)
    : _printer(printer_arg), _value(&value_arg)
{}

template <typename Printer, typename T>
requires stream_proxy_printer_c<Printer, T>
std::ostream&
    operator<<(
        std::ostream&                   stream_arg,
        const stream_proxy<Printer, T>& proxy_arg
    )
{
    stream(proxy_arg._printer, *proxy_arg._value, stream_arg);
    return stream_arg;
}

template <typename Printer, typename T>
requires stream_proxy_printer_c<Printer, T>
constexpr stream_proxy<Printer, T>
    to_stream(
        const Printer& printer_arg,
        const T&       value_arg
    )
{
    return stream_proxy<Printer, T>(printer_arg, value_arg);
}

template <typename T>
requires default_printable_c<T> && stream_proxy_printer_c<default_printer<T>, T>
constexpr stream_proxy<default_printer<T>, T>
    to_stream(
        const T& value_arg
    )
{
    return to_stream(default_printer<T>{}, value_arg);
}

template <typename Printer, typename T>
requires std::is_default_constructible_v<Printer>
         && stream_proxy_printer_c<Printer, T>
constexpr stream_proxy<Printer, T>
    to_stream(
        const T& value_arg
    )
{
    return to_stream(Printer{}, value_arg);
}

PNP_NS_END