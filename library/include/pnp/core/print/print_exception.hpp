#pragma once
#include "pnp/internal/macros.hpp"
#include <stdexcept>
#include <string>
#include <unicode_bridge.hpp>
#include <string_view>

PNP_NS_BEGIN

/*!
 * @brief Exception type for errors thrown during printing.
 *
 * Printers written by users should throw this exception when some error
 * is found that they want to be caught. An example of its use may be a
 * printer not supporting certain values.
 *
 */
    struct print_exception : public std::runtime_error
{
    /*!
     * @brief Constructor with a user-defined message.
     * @param message_arg The message to populate the exception with.
     */
    explicit print_exception(const std::string& message_arg);
    /*!
     * @brief Constructor with a user-defined message.
     *
     * The UTF-8 string is cast to a std::string, holding the same bytes, so the
     * message returned by what() is UTF-8 encoded.
     *
     * @param message_arg The message to populate the exception with.
     */
    explicit print_exception(const std::u8string_view message_arg);
};

PNP_NS_END

PNP_NS_BEGIN
inline print_exception::print_exception(
    const std::string& message_arg
)
    : std::runtime_error(message_arg)
{
}

inline print_exception::print_exception(
    const std::u8string_view message_arg
)
    : std::runtime_error(std::string{
          UNICODE_BRIDGE_NAMESPACE::cast_char_ptr<char>(message_arg.data()),
          message_arg.size()
        })
{
}

PNP_NS_END