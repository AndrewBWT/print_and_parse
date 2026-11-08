#pragma once
#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <charconv>
#include <concepts>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

struct integer_pnp_default
{
    static constexpr bool use_plus_for_positive_numbers = false;
};

template <typename Integer_Pnp_Defaults>
struct integer_pnp_options
{
    bool use_plus_for_positive_numbers
        = Integer_Pnp_Defaults::use_plus_for_positive_numbers;
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;
};

template <typename Integer_Pnp_Defaults>
concept valid_integer_pnp_c
    = valid_default_options_c<integer_pnp_options<Integer_Pnp_Defaults>>;

template <typename T, typename Integer_Pnp_Defaults = integer_pnp_default>
    requires std::integral<T> && (not std::same_as<std::remove_cv_t<T>, bool>)
&& valid_integer_pnp_c<Integer_Pnp_Defaults>
struct integer_pnp : public pnp_configuration<integer_pnp_options<Integer_Pnp_Defaults>>
{
public:
    using base_t = pnp_configuration<integer_pnp_options<Integer_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;
    constexpr void
        run_printer(
            const std::same_as<T> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            T&             object_answer,
            parser_run_key run_key_arg
        ) const;
};

template <typename T>
inline constexpr bool enable_default_integer_pnp = false;

PNP_SUPPORT_NS_END

template <typename T>
requires PNP_SUPPORT_NAMESPACE::enable_default_integer_pnp<T>
struct default_printer<T> : public PNP_SUPPORT_NAMESPACE::integer_pnp<T>
{
    using base_t = PNP_SUPPORT_NAMESPACE::integer_pnp<T>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

template <typename T>
requires PNP_SUPPORT_NAMESPACE::enable_default_integer_pnp<T>
struct default_parser<T> : public PNP_SUPPORT_NAMESPACE::integer_pnp<T>
{
    using base_t = PNP_SUPPORT_NAMESPACE::integer_pnp<T>;
    using base_t::base_t;
    static constexpr bool is_specialized = true;
};

PNP_NS_END

PNP_NS_BEGIN

PNP_SUPPORT_NS_BEGIN
template <typename Integer_Pnp_Defaults>
constexpr const char*
    integer_pnp_options<Integer_Pnp_Defaults>::first_error() const noexcept
{
    return nullptr;
}

template <typename T, typename Integer_Pnp_Defaults>
    requires std::integral<T> && (not std::same_as<std::remove_cv_t<T>, bool>)
             && valid_integer_pnp_c<Integer_Pnp_Defaults>
    constexpr void integer_pnp<T, Integer_Pnp_Defaults>::run_printer(
        const std::same_as<T> auto& object_arg,
        text_sink&     sink_obj_arg
    ) const
{
    if (_options.use_plus_for_positive_numbers && object_arg >= 0)
    {
        sink_obj_arg.put(u8'+');
    }
    char buffer
        [std::numeric_limits<T>::digits10 + 1 + (std::is_signed_v<T> ? 1 : 0)];
    const char* ptr_to_end
        = std::to_chars(buffer, buffer + sizeof(buffer), object_arg).ptr;
    sink_obj_arg.write(unicode_bridge::cast_string_view<char8_t>(
        std::string_view(buffer, ptr_to_end)
    ));
}

template <typename T, typename Integer_Pnp_Defaults>
                     requires std::integral<T> && (not std::same_as<std::remove_cv_t<T>, bool>) && valid_integer_pnp_c<Integer_Pnp_Defaults>
                 constexpr basic_parser_return_value
                     integer_pnp<T, Integer_Pnp_Defaults>::run_parser(parser_input& parser_input_arg, T& object_answer, parser_run_key run_key_arg) const
{
    using namespace std;

    // The whole number, however it's split: a sign or two at the start (two
    // only so "+-" can be reported as such), then digits.
    size_t     seen = 0;
    const auto text = parser_input_arg.peek_while(
        [&seen](const char8_t code_unit_arg)
        {
            const bool sign_position = seen++ < 2;
            return PNP_INTERNAL_NAMESPACE::is_digit(code_unit_arg)
                   || (sign_position
                       && (code_unit_arg == u8'+' || code_unit_arg == u8'-'));
        }
    );

    if constexpr (is_unsigned_v<T>)
    {
        if (text.get().starts_with(u8'-'))
        {
            return parser_input_arg.fail(
                u8"Negative values cannot be stored in an unsigned type."
            );
        }
    }
    const size_t sign_chars = text.get().starts_with(u8'+') ? 1 : 0;
    if (sign_chars == 1 && text.get().substr(1).starts_with(u8'-'))
    {
        return parser_input_arg.fail(u8"Expected a number, but found \"+-\".");
    }

    const auto narrow
        = unicode_bridge::cast_string_view<char>(text.get().substr(sign_chars));
    T parsed{};
    const auto [ptr, ec]
        = from_chars(narrow.data(), narrow.data() + narrow.size(), parsed);

    if (ec == errc::invalid_argument)
    {
        return parser_input_arg.fail(
            u8"Expected a valid integer, but no valid digits were found."
        );
    }
    if (ec == errc::result_out_of_range)
    {
        u8string msg = u8"The value \"";
        msg.append(text.get().substr(
            0, sign_chars + static_cast<size_t>(ptr - narrow.data())
        ));
        msg.append(u8"\" is out of range for ");
        msg.append(is_signed_v<T> ? u8"a signed" : u8"an unsigned");
        msg.append(u8" type of ");
        msg.append(PNP_INTERNAL_NAMESPACE::size_to_u8string(sizeof(T)));
        msg.append(sizeof(T) == 1 ? u8" byte." : u8" bytes.");
        return parser_input_arg.fail(std::move(msg));
    }

    object_answer = parsed;
    parser_input_arg.advance(
        sign_chars + static_cast<size_t>(ptr - narrow.data())
    );
    return {};
}

PNP_SUPPORT_NS_END
PNP_NS_END