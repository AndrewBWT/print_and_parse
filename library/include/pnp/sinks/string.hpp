#pragma once

#include "pnp/core/print/text_sink.hpp"
#include "pnp/internal/macros.hpp"
#include <concepts>
#include <string>
#include <string_view>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN
/*!
 * @brief Concept that describes the types of string that string_sink
 * supports - such as std::string, std::u8string, or their std::pmr equivalents.
 */
template <typename String>
concept output_string_c
    = (std::same_as<typename String::value_type, char>
       || std::same_as<typename String::value_type, char8_t>)
      && std::same_as<
          String,
          std::basic_string<
              typename String::value_type,
              std::char_traits<typename String::value_type>,
              typename String::allocator_type>>;

/*!
 * @brief text_sink that appends to a std::basic_string accepted by
 * output_string_c. The UTF-8 is appended as it is given, with no conversion.
 *
 * Failures can be raised by the underlying string; for example, std::bad_alloc
 * or std::length_error.
 *
 * The string given to the constructor must outlive the sink.
 * @tparam String The type of the underlying string.
 */
template <typename String>
requires output_string_c<String>
struct string_sink : public text_sink
{
public:
    /*!
     * @brief Constructor taking the string to write to as the argument.
     * @param string_arg Either a std::u8string or a std::string used to write
     * the text to.
     */
    explicit string_sink(String& string_arg) noexcept;
private:
    //! The string being appended to.
    String* _string;
    /*!
     * @brief Writes a single char8_t to the underlying string.
     * @param char_arg The char8_t to write to the string.
     */
    void
        put_char(const char8_t char_arg) override;
    /*!
     * @brief Writes a std::u8string_view to the underlying string.
     *
     * text_arg is written as is, with no conversion.
     *
     * @param text_arg The std::u8string_view to write to the underlying string.
     */
    void
        write_text(const std::u8string_view text_arg) override;
};

PNP_NS_END

PNP_NS_BEGIN

template <typename String>
requires output_string_c<String>
inline string_sink<String>::string_sink(
    String& string_arg
) noexcept
    : _string(&string_arg)
{}

template <typename String>
requires output_string_c<String>
inline void
    string_sink<String>::put_char(
        const char8_t char_arg
    )
{
    _string->push_back(static_cast<typename String::value_type>(char_arg));
}

template <typename String>
requires output_string_c<String>
inline void
    string_sink<String>::write_text(
        const std::u8string_view text_arg
    )
{
    _string->append(
        UNICODE_BRIDGE_NAMESPACE::cast_char_ptr<typename String::value_type>(
            text_arg.data()
        ),
        text_arg.size()
    );
}

PNP_NS_END