#pragma once
#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <bit>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

PNP_INTERNAL_NS_BEGIN
template <typename T>
struct unsigned_type_for_size;

template <typename T>
requires (sizeof(T) == 1)
struct unsigned_type_for_size<T>
{
    using type = std::uint8_t;
};

template <typename T>
requires (sizeof(T) == 2)
struct unsigned_type_for_size<T>
{
    using type = std::uint16_t;
};

template <typename T>
requires (sizeof(T) == 4)
struct unsigned_type_for_size<T>
{
    using type = std::uint32_t;
};

template <typename T>
requires (sizeof(T) == 8)
struct unsigned_type_for_size<T>
{
    using type = std::uint64_t;
};

template <typename T>
using unsigned_type_for_size_t = typename unsigned_type_for_size<T>::type;

inline constexpr std::size_t bits_per_byte
    = std::numeric_limits<unsigned char>::digits;

// The number of digits max_arg needs in the base: the most any value up to it
// can need.
template <typename U>
constexpr std::size_t
    digits_for_value(
        U         max_arg,
        const int base_arg
    ) noexcept
{
    std::size_t digits = 0;
    do
    {
        max_arg /= static_cast<U>(base_arg);
        ++digits;
    }
    while (max_arg != 0);
    return digits;
}

constexpr std::u8string
    base_name(
        const int base_arg
    )
{
    switch (base_arg)
    {
    case 2:
        return u8"binary";
    case 8:
        return u8"octal";
    case 10:
        return u8"decimal";
    case 16:
        return u8"hexadecimal";
    default:
        return u8"base-" + size_to_u8string(static_cast<std::size_t>(base_arg));
    }
}

PNP_INTERNAL_NS_END

template <typename T>
concept bit_castable_to_unsigned_c
    = std::is_trivially_copyable_v<T>
      && (std::has_unique_object_representations_v<T>
          || std::is_floating_point_v<T>
          || std::same_as<std::remove_cv_t<T>, bool>)
      && requires {
             typename PNP_INTERNAL_NAMESPACE::unsigned_type_for_size_t<T>;
         };

// max_valid_pattern is optional. Some fixed width types - e.g. bool - are
// represneted as a some type, but not all values are valid.
template <typename T>
struct bit_pattern_validity
{
    static constexpr bool
        is_valid(const PNP_INTERNAL_NAMESPACE::unsigned_type_for_size_t<T>) noexcept
    {
        return true;
    }
};

template <>
struct bit_pattern_validity<bool>
{
    static constexpr std::u8string_view type_name = u8"bool";
    static constexpr PNP_INTERNAL_NAMESPACE::unsigned_type_for_size_t<bool>
        max_valid_pattern = 1;

    static constexpr bool
        is_valid(
            const PNP_INTERNAL_NAMESPACE::unsigned_type_for_size_t<bool>
                bits_arg
        ) noexcept
    {
        return bits_arg <= max_valid_pattern;
    }
};

// An enum with bool as its underlying type has exactly bool's values.
template <typename T>
requires std::is_enum_v<T> && std::same_as<std::underlying_type_t<T>, bool>
struct bit_pattern_validity<T> : bit_pattern_validity<bool>
{};

PNP_INTERNAL_NS_BEGIN

// The largest valid bit pattern of T: bit_pattern_validity's max_valid_pattern
// if it has one, otherwise every bit set.
template <typename T>
constexpr unsigned_type_for_size_t<T>
    max_valid_pattern() noexcept
{
    using validity = bit_pattern_validity<std::remove_cv_t<T>>;
    if constexpr (requires { validity::max_valid_pattern; })
    {
        return static_cast<unsigned_type_for_size_t<T>>(
            validity::max_valid_pattern
        );
    }
    else
    {
        return std::numeric_limits<unsigned_type_for_size_t<T>>::max();
    }
}

PNP_INTERNAL_NS_END

struct fixed_width_pnp_default_base
{
    static constexpr bool upper             = false;
    static constexpr bool fixed_size_output = false;
};

struct fixed_width_pnp_binary : fixed_width_pnp_default_base
{
    static constexpr int                base   = 2;
    static constexpr std::u8string_view prefix = u8"0b";
};

struct fixed_width_pnp_octal : fixed_width_pnp_default_base
{
    static constexpr int                base   = 8;
    static constexpr std::u8string_view prefix = u8"o";
};

struct fixed_width_pnp_unsigned_int : fixed_width_pnp_default_base
{
    static constexpr int                base   = 10;
    static constexpr std::u8string_view prefix = u8"";
};

struct fixed_width_pnp_unsigned_hex : fixed_width_pnp_default_base
{
    static constexpr int                base   = 16;
    static constexpr std::u8string_view prefix = u8"0x";
};

template <typename Fixed_Width_Pnp_Defaults>
struct fixed_width_pnp_options
{
    int           base     = Fixed_Width_Pnp_Defaults::base;
    std::u8string prefix   = std::u8string(Fixed_Width_Pnp_Defaults::prefix);
    bool          upper    = Fixed_Width_Pnp_Defaults::upper;
    bool fixed_size_output = Fixed_Width_Pnp_Defaults::fixed_size_output;

    // The first rule these options break, or nullptr if they're valid.
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;
};

template <typename Fixed_Width_Pnp_Defaults>
concept valid_fixed_width_pnp_c = valid_default_options_c<
    fixed_width_pnp_options<Fixed_Width_Pnp_Defaults>>;

template <
    typename T,
    typename Fixed_Width_Pnp_Defaults = fixed_width_pnp_unsigned_hex>
requires bit_castable_to_unsigned_c<T>
         && valid_fixed_width_pnp_c<Fixed_Width_Pnp_Defaults>
struct fixed_width_pnp
    : public pnp_configuration<
          fixed_width_pnp_options<Fixed_Width_Pnp_Defaults>>
{
public:
    using base_t
        = pnp_configuration<fixed_width_pnp_options<Fixed_Width_Pnp_Defaults>>;
    using options_type = typename base_t::options_type;
    using base_t::_options;
    constexpr fixed_width_pnp() = default;
    constexpr explicit fixed_width_pnp(const options_type& options_arg);
    constexpr explicit fixed_width_pnp(options_type&& options_arg);
    // Copies are exact: the options and the width that goes with them.
    constexpr fixed_width_pnp(const fixed_width_pnp&) = default;
    constexpr fixed_width_pnp&
        operator=(const fixed_width_pnp&)
        = default;

    // A move leaves the source with the default options (see
    // pnp_configuration), so its width is recomputed to match them.
    constexpr fixed_width_pnp(fixed_width_pnp&& other_arg) noexcept(std::is_nothrow_move_constructible_v<base_t>)
        : base_t(std::move(other_arg))
        , _max_width(other_arg._max_width)   // still the moved-from options' width: only the base was reset
    {
        other_arg._max_width = compute_max_width(other_arg._options.base);
    }

    constexpr fixed_width_pnp&
        operator=(fixed_width_pnp&& other_arg)
            noexcept(std::is_nothrow_move_assignable_v<base_t>)
    {
        if (this != &other_arg)
        {
            base_t::operator=(std::move(other_arg));
            _max_width           = other_arg._max_width;
            other_arg._max_width = compute_max_width(other_arg._options.base);
        }
        return *this;
    }

    constexpr void
        run_printer(
            const std::same_as<T> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const;
    constexpr basic_parser_return_value
        run_parser(
            parser_input&  input_arg,
            T&             object_arg,
            parser_run_key run_key_arg
        ) const;
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept;
private:
    using unsigned_t = PNP_INTERNAL_NAMESPACE::unsigned_type_for_size_t<T>;

    static constexpr std::size_t
        compute_max_width(
            const int base_arg
        ) noexcept
    {
        return PNP_INTERNAL_NAMESPACE::digits_for_value(
            PNP_INTERNAL_NAMESPACE::max_valid_pattern<T>(), base_arg
        );
    }

    std::size_t _max_width = compute_max_width(Fixed_Width_Pnp_Defaults::base);
};

PNP_SUPPORT_NS_END
PNP_NS_END

PNP_NS_BEGIN

PNP_SUPPORT_NS_BEGIN
template <typename Fixed_Width_Pnp_Defaults>
constexpr const char*
    fixed_width_pnp_options<Fixed_Width_Pnp_Defaults>::first_error(
    ) const noexcept
{
    using namespace PNP_INTERNAL_NAMESPACE;
    if (base < 2 || base > 36)
    {
        return "fixed_width_pnp_options base must be between 2 and 36";
    }
    // The prefix may be empty: the parser strips exactly what the printer
    // wrote.
    if (not valid_utf8(prefix))
    {
        return "fixed_width_pnp_options prefix must be valid UTF-8";
    }
    if (not no_surrounding_whitespace(prefix))
    {
        return "fixed_width_pnp_options prefix must not start or end with "
               "whitespace";
    }
    return nullptr;
}

template <typename T, typename Fixed_Width_Pnp_Defaults>
requires bit_castable_to_unsigned_c<T>
             && valid_fixed_width_pnp_c<Fixed_Width_Pnp_Defaults>
constexpr fixed_width_pnp<T, Fixed_Width_Pnp_Defaults>::fixed_width_pnp(
    const options_type& options_arg
)
    : base_t(options_arg)
    , _max_width(PNP_INTERNAL_NAMESPACE::digits_for_value(
          PNP_INTERNAL_NAMESPACE::max_valid_pattern<T>(),
          _options.base
      ))
{}

template <typename T, typename Fixed_Width_Pnp_Defaults>
requires bit_castable_to_unsigned_c<T>
             && valid_fixed_width_pnp_c<Fixed_Width_Pnp_Defaults>
constexpr fixed_width_pnp<T, Fixed_Width_Pnp_Defaults>::fixed_width_pnp(
    options_type&& options_arg
)
    : base_t(std::move(options_arg))
    , _max_width(PNP_INTERNAL_NAMESPACE::digits_for_value(
          PNP_INTERNAL_NAMESPACE::max_valid_pattern<T>(),
          _options.base
      ))
{}

template <typename T, typename Fixed_Width_Pnp_Defaults>
requires bit_castable_to_unsigned_c<T>
         && valid_fixed_width_pnp_c<Fixed_Width_Pnp_Defaults>
constexpr void
    fixed_width_pnp<T, Fixed_Width_Pnp_Defaults>::run_printer(
        const std::same_as<T> auto& object_arg,
        text_sink&                  sink_obj_arg
    ) const
{
    const unsigned_t unsigned_value = std::bit_cast<unsigned_t>(object_arg);
    // Base 2 is the longest output: one character per bit.
    char buffer[sizeof(unsigned_t) * PNP_INTERNAL_NAMESPACE::bits_per_byte];
    const char* ptr_to_end
        = std::to_chars(
              buffer, buffer + sizeof(buffer), unsigned_value, _options.base
        )
              .ptr;

    if (_options.base > 10)
    {
        if (_options.upper)
        {
            for (char* char_ptr = buffer; char_ptr != ptr_to_end; ++char_ptr)
            {
                if (*char_ptr >= 'a' && *char_ptr <= 'z')
                {
                    *char_ptr = static_cast<char>(*char_ptr - ('a' - 'A'));
                }
            }
        }
    }

    sink_obj_arg.write(std::u8string_view(_options.prefix));
    if (_options.fixed_size_output)
    {
        const auto digits = static_cast<std::size_t>(ptr_to_end - buffer);
        for (std::size_t idx = digits; idx < _max_width; ++idx)
        {
            sink_obj_arg.put(u8'0');
        }
    }
    sink_obj_arg.write(unicode_bridge::cast_string_view<char8_t>(
        std::string_view(buffer, ptr_to_end)
    ));
}

template <typename T, typename Fixed_Width_Pnp_Defaults>
requires bit_castable_to_unsigned_c<T>
         && valid_fixed_width_pnp_c<Fixed_Width_Pnp_Defaults>
constexpr basic_parser_return_value
    fixed_width_pnp<T, Fixed_Width_Pnp_Defaults>::run_parser(
        parser_input&  input_arg,
        T&             object_arg,
        parser_run_key run_key_arg
    ) const
{
    using namespace std;
    using namespace unicode_bridge;
    if (not _options.prefix.empty()
        && not input_arg.advance_if_starts_with(_options.prefix))
    {
        return input_arg.fail(
            u8"Unable to detect prefix \"" + _options.prefix + u8"\"."
        );
    }

    const auto remaining_str_input = input_arg.peek_while(
        [&](auto char_arg)
        {
            return PNP_INTERNAL_NAMESPACE::is_base_digit(
                char_arg, _options.base
            );
        }
    );
    const auto  remaining_str = remaining_str_input.get();
    const char* ptr_begin     = cast_char_ptr<char>(remaining_str.data());
    const char* ptr_end
        = ptr_begin
          + (remaining_str.size() < _max_width ? remaining_str.size()
                                               : _max_width);

    unsigned_t value{};
    const auto [end_of_conversion_char_ptr, error_code]
        = from_chars(ptr_begin, ptr_end, value, _options.base);

    if (error_code == errc::invalid_argument)
    {
        u8string msg = u8"Expected a valid ";
        msg.append(PNP_INTERNAL_NAMESPACE::base_name(_options.base));
        msg.append(u8" integer");
        if (not _options.prefix.empty())
        {
            msg.append(u8" (with prefix \"");
            msg.append(_options.prefix);
            msg.append(u8"\")");
        }
        msg.append(u8", but no valid digits were found.");
        return input_arg.fail(std::move(msg));
    }
    // Though this is only used in the errors, its a string view, so cheap to
    // construct.
    const u8string_view digits_text(
        remaining_str.data(),
        static_cast<size_t>(end_of_conversion_char_ptr - ptr_begin)
    );
    if (error_code == errc::result_out_of_range)
    {
        u8string msg = u8"The value \"";
        msg.append(_options.prefix);
        msg.append(digits_text);
        msg.append(u8"\" is too large for a target type of ");
        msg.append(PNP_INTERNAL_NAMESPACE::size_to_u8string(sizeof(T)));
        msg.append(sizeof(T) == 1 ? u8" byte." : u8" bytes.");
        return input_arg.fail(std::move(msg));
    }

    using bit_pattern = bit_pattern_validity<std::remove_cv_t<T>>;
    if (not bit_pattern::is_valid(value))
    {
        u8string msg = u8"The value \"";
        msg.append(_options.prefix);
        msg.append(digits_text);
        msg.append(u8"\" is not a valid ");
        if constexpr (requires { bit_pattern::type_name; })
        {
            msg.append(bit_pattern::type_name);
        }
        else
        {
            msg.append(u8"value for this type");
        }
        msg.append(u8".");
        return input_arg.fail(std::move(msg));
    }
    input_arg.advance(digits_text.size());
    object_arg = bit_cast<T>(value);
    return {};
}

template <typename T, typename Fixed_Width_Pnp_Defaults>
requires bit_castable_to_unsigned_c<T>
         && valid_fixed_width_pnp_c<Fixed_Width_Pnp_Defaults>
constexpr std::optional<std::size_t>
    fixed_width_pnp<T, Fixed_Width_Pnp_Defaults>::fixed_output_width(
    ) const noexcept
{
    if (not _options.fixed_size_output)
    {
        return std::nullopt; // 0xa and 0xaf would run together
    }
    return _options.prefix.size() + _max_width;
}

PNP_SUPPORT_NS_END
PNP_NS_END