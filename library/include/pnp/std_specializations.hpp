#pragma once

#include "pnp/internal/macros.hpp"
#include "pnp/support.hpp"
#include "pnp/support/std.hpp"

/*_BEGIN_ABC_UTILITY_PRINTER_NS

// ════════════════════════════════════════════════════════════════════
// Concepts
// ════════════════════════════════════════════════════════════════════

template <typename T>
concept map_like = std::ranges::range<T>
&& requires { typename T::key_type; typename T::mapped_type; };

template <typename T>
concept string_like =
std::same_as<T, std::string>
|| std::same_as<T, std::wstring>
|| std::same_as<T, std::u8string>
|| std::same_as<T, std::u16string>
|| std::same_as<T, std::u32string>;

template <typename T>
concept string_view_like =
std::same_as<T, std::string_view>
|| std::same_as<T, std::wstring_view>
|| std::same_as<T, std::u8string_view>
|| std::same_as<T, std::u16string_view>
|| std::same_as<T, std::u32string_view>;

template <typename T>
concept character_like =
std::same_as<T, char>
|| std::same_as<T, char8_t>
|| std::same_as<T, char16_t>
|| std::same_as<T, char32_t>;

template <typename T>
concept printable_range = std::ranges::range<T>
&& (!string_like<T>)
&& (!string_view_like<T>)
&& (!map_like<T>);

// ════════════════════════════════════════════════════════════════════
// bool
// ════════════════════════════════════════════════════════════════════

template <>
struct default_printer_t<bool>
{
    __constexpr std::u8string
        run_printer(const bool& _a_object) const
    {
        return _a_object ? u8"true" : u8"false";
    }
};

// ════════════════════════════════════════════════════════════════════
// Characters: char, char8_t, char16_t, char32_t
// ════════════════════════════════════════════════════════════════════

template <character_like T>
struct default_printer_t<T>
{
    __constexpr
        default_printer_t(std::u8string _a_surrounding_str = std::u8string{})
        : _m_surrounding_str(std::move(_a_surrounding_str))
    {
    }

    __constexpr std::u8string
        run_printer(const T& _a_object) const
    {
        using namespace _ABC_NS_UTILITY_STR;
        std::u8string _l_char_str;
        if constexpr (std::same_as<T, char>)
        {
            _l_char_str = ascii_char_to_u8string(_a_object);
        }
        else
        {
            _l_char_str = unicode_char_to_u8string(_a_object);
        }
        return fmt::format(u8"{0}{1}{0}", _m_surrounding_str, _l_char_str);
    }

private:
    std::u8string _m_surrounding_str;
};

// ════════════════════════════════════════════════════════════════════
// Numeric types via std::to_chars
// ════════════════════════════════════════════════════════════════════

template <typename T>
    requires to_chars_convertable_c<T> && (!std::same_as<T, char>)
struct default_printer_t<T>
{
    __constexpr std::u8string
        run_printer(const T& _a_object) const
    {
        using namespace _ABC_NS_UTILITY_STR;
        std::array<char, 64> _l_buf{};
        auto [ptr, ec] = std::to_chars(
            _l_buf.data(), _l_buf.data() + _l_buf.size(), _a_object
        );
        return cast_string_to_u8string(
            std::string_view(_l_buf.data(), ptr)
        );
    }
};

// ════════════════════════════════════════════════════════════════════
// Enums with enum_helper
// ════════════════════════════════════════════════════════════════════

template <typename T>
    requires enum_has_list_c<T>
struct default_printer_t<T>
{
    __constexpr
        default_printer_t(
            const enum_helper_string_type_e _a_case
            = enum_helper_string_type_e::lower
        )
        : _m_case(_a_case)
    {
    }

    __constexpr std::u8string
        run_printer(const T& _a_element) const
    {
        return _ABC_NS_UTILITY::get_thread_local_enumerate_enum_helper<T>()
            .print(_a_element, _m_case);
    }

private:
    enum_helper_string_type_e _m_case;
};

// ════════════════════════════════════════════════════════════════════
// Strings: std::basic_string<CharT>
// ════════════════════════════════════════════════════════════════════

namespace detail
{
    template <typename CharT>
    __constexpr std::u8string
        print_str(const std::basic_string_view<CharT> _a_str)
    {
        using namespace _ABC_NS_UTILITY_STR;
        if constexpr (std::same_as<CharT, char>)
        {
            std::u8string                _l_rv{};
            default_printer_t<char> _l_printer;
            for (const char _l_char : _a_str)
            {
                _l_rv.append(_l_printer.run_printer(_l_char));
            }
            return _l_rv;
        }
        else if constexpr (std::same_as<CharT, char8_t>
            || std::same_as<CharT, char16_t>
            || std::same_as<CharT, char32_t>
            || std::same_as<CharT, wchar_t>)
        {
            return unicode_string_to_u8string(_a_str);
        }
    }
} // namespace detail

template <typename T>
    requires string_like<std::basic_string<T>>
struct default_printer_t<std::basic_string<T>>
{
    __constexpr
        default_printer_t(std::u8string_view _a_surrounding_str = u8"\"")
        : _m_surrounding_str(_a_surrounding_str)
    {
    }

    __constexpr std::u8string
        run_printer(const std::basic_string<T>& _a_object) const
    {
        return fmt::format(
            u8"{0}{1}{0}",
            _m_surrounding_str,
            detail::print_str<T>(_a_object)
        );
    }

private:
    std::u8string _m_surrounding_str;
};

template <typename T>
    requires string_view_like<std::basic_string_view<T>>
struct default_printer_t<std::basic_string_view<T>>
{
    __constexpr
        default_printer_t(std::u8string_view _a_surrounding_str = u8"\"")
        : _m_surrounding_str(_a_surrounding_str)
    {
    }

    __constexpr std::u8string
        run_printer(const std::basic_string_view<T>& _a_object) const
    {
        return fmt::format(
            u8"{0}{1}{0}",
            _m_surrounding_str,
            detail::print_str<T>(_a_object)
        );
    }

private:
    std::u8string _m_surrounding_str;
};

// Non-standard character types fall back to range-based printing.
template <typename T>
    requires (!string_like<std::basic_string<T>>)
struct default_printer_t<std::basic_string<T>>
    : public default_printer_t<void> // will be caught by range specialisation
{
};

// ════════════════════════════════════════════════════════════════════
// Ranges (vector, list, deque, set, forward_list, etc.)
// ════════════════════════════════════════════════════════════════════

template <printable_range T>
struct default_printer_t<T>
{
    using value_t = std::ranges::range_value_t<T>;

    __constexpr
        default_printer_t(
            default_printer_t<value_t>        _a_inner_printer = default_printer_t<value_t>{},
            const range_based_printer_opts_t& _a_opts = range_based_printer_opts_t{}
        ) noexcept
        : _m_inner_printer(std::move(_a_inner_printer))
        , _m_opts(_a_opts)
    {
    }

    __constexpr std::u8string
        run_printer(const T& _a_object) const
    {
        std::u8string _l_rv{ _m_opts.begin_str };
        auto          _l_itt = std::ranges::begin(_a_object);
        const auto    _l_end = std::ranges::end(_a_object);

        for (; _l_itt != _l_end; ++_l_itt)
        {
            _l_rv.append(_m_inner_printer.run_printer(*_l_itt));
            if (std::next(_l_itt) != _l_end)
            {
                _l_rv.append(_m_opts.delimiter);
            }
        }
        _l_rv.append(_m_opts.end_str);
        return _l_rv;
    }

private:
    default_printer_t<value_t> _m_inner_printer;
    range_based_printer_opts_t _m_opts;
};

// ════════════════════════════════════════════════════════════════════
// Map-like containers (map, unordered_map, multimap, etc.)
// ════════════════════════════════════════════════════════════════════

template <map_like T>
struct default_printer_t<T>
{
    using key_t = typename T::key_type;
    using mapped_t = typename T::mapped_type;

    __constexpr
        default_printer_t(
            default_printer_t<key_t>    _a_key_printer = default_printer_t<key_t>{},
            default_printer_t<mapped_t> _a_mapped_printer = default_printer_t<mapped_t>{}
        ) noexcept
        : _m_key_printer(std::move(_a_key_printer))
        , _m_mapped_printer(std::move(_a_mapped_printer))
    {
    }

    __constexpr std::u8string
        run_printer(const T& _a_object) const
    {
        std::u8string _l_str{ u8"{" };
        auto          _l_itt = std::ranges::begin(_a_object);
        const auto    _l_end = std::ranges::end(_a_object);

        for (; _l_itt != _l_end; ++_l_itt)
        {
            _l_str.append(_m_key_printer.run_printer(_l_itt->first));
            _l_str.append(u8":");
            _l_str.append(_m_mapped_printer.run_printer(_l_itt->second));
            if (std::next(_l_itt) != _l_end)
            {
                _l_str.append(u8", ");
            }
        }
        _l_str.append(u8"}");
        return _l_str;
    }

private:
    default_printer_t<key_t>    _m_key_printer;
    default_printer_t<mapped_t> _m_mapped_printer;
};

// ════════════════════════════════════════════════════════════════════
// std::pair
// ════════════════════════════════════════════════════════════════════

template <typename T, typename U>
struct default_printer_t<std::pair<T, U>>
{
    using value_type = std::pair<T, U>;

    __constexpr
        default_printer_t(
            default_printer_t<T> _a_first_printer = default_printer_t<T>{},
            default_printer_t<U> _a_second_printer = default_printer_t<U>{}
        )
        : _m_first_printer(std::move(_a_first_printer))
        , _m_second_printer(std::move(_a_second_printer))
    {
    }

    __constexpr std::u8string
        run_printer(const value_type& _a_object) const
    {
        std::u8string _l_str{ u8"(" };
        _l_str.append(_m_first_printer.run_printer(_a_object.first));
        _l_str.append(u8", ");
        _l_str.append(_m_second_printer.run_printer(_a_object.second));
        _l_str.append(u8")");
        return _l_str;
    }

private:
    default_printer_t<T> _m_first_printer;
    default_printer_t<U> _m_second_printer;
};

// ════════════════════════════════════════════════════════════════════
// std::tuple
// ════════════════════════════════════════════════════════════════════

template <typename... Ts>
struct default_printer_t<std::tuple<Ts...>>
{
    using value_type = std::tuple<Ts...>;

    __constexpr
        default_printer_t(
            default_printer_t<Ts>... _a_printers
        )
        : _m_printers(std::move(_a_printers)...)
    {
    }

    __constexpr
        default_printer_t()
        : _m_printers(default_printer_t<Ts>{}...)
    {
    }

    __constexpr std::u8string
        run_printer(const value_type& _a_object) const
    {
        std::u8string _l_str{ u8"(" };
        run_element<0>(_l_str, _a_object);
        _l_str.append(u8")");
        return _l_str;
    }

    __constexpr std::tuple<default_printer_t<Ts>...>&
        get_printers_ref() noexcept
    {
        return _m_printers;
    }

private:
    std::tuple<default_printer_t<Ts>...> _m_printers;

    template <std::size_t I>
    __constexpr void
        run_element(std::u8string& _a_str, const value_type& _a_object) const
    {
        _a_str.append(
            std::get<I>(_m_printers).run_printer(std::get<I>(_a_object))
        );
        if constexpr (I + 1 < sizeof...(Ts))
        {
            _a_str.append(u8",");
            run_element<I + 1>(_a_str, _a_object);
        }
    }
};

// ════════════════════════════════════════════════════════════════════
// std::variant
// ════════════════════════════════════════════════════════════════════

template <typename... Ts>
struct default_printer_t<std::variant<Ts...>>
{
    using value_type = std::variant<Ts...>;

    __constexpr
        default_printer_t(
            const variant_print_parse_e _a_mode,
            default_printer_t<Ts>... _a_printers
        )
        : _m_mode(_a_mode)
        , _m_printers(std::move(_a_printers)...)
    {
    }

    __constexpr
        default_printer_t(const variant_print_parse_e _a_mode)
        : _m_mode(_a_mode)
        , _m_printers(default_printer_t<Ts>{}...)
    {
    }

    __constexpr
        default_printer_t()
        : _m_mode(variant_print_parse_e::use_indexes)
        , _m_printers(default_printer_t<Ts>{}...)
    {
    }

    __constexpr std::u8string
        run_printer(const value_type& _a_object) const
    {
        using namespace std;
        using enum variant_print_parse_e;
        const size_t   _l_idx{ _a_object.index() };
        const u8string _l_element_str{ visit_element<0>(_a_object, _l_idx) };

        switch (_m_mode)
        {
        case no_indexes:
            return _l_element_str;
        case use_indexes:
            return fmt::format(u8"({0}, {1})", _l_idx, _l_element_str);
        default:
            throw errors::unaccounted_for_enum_exception(_m_mode);
        }
    }

    __constexpr std::tuple<default_printer_t<Ts>...>&
        get_printers_ref() noexcept
    {
        return _m_printers;
    }

private:
    variant_print_parse_e                _m_mode;
    std::tuple<default_printer_t<Ts>...> _m_printers;

    template <std::size_t I>
    __constexpr std::u8string
        visit_element(
            const value_type& _a_object,
            const std::size_t _a_idx
        ) const
    {
        if (I == _a_idx)
        {
            return std::get<I>(_m_printers).run_printer(
                std::get<I>(_a_object)
            );
        }
        else if constexpr (I + 1 < sizeof...(Ts))
        {
            return visit_element<I + 1>(_a_object, _a_idx);
        }
        else
        {
            throw std::runtime_error(
                "variant index out of range in printer"
            );
        }
    }
};

// ════════════════════════════════════════════════════════════════════
// std::expected
// ════════════════════════════════════════════════════════════════════

template <typename T, typename U>
struct default_printer_t<std::expected<T, U>>
{
    using value_type = std::expected<T, U>;

    __constexpr
        default_printer_t(
            default_printer_t<T>    _a_value_printer = default_printer_t<T>{},
            default_printer_t<U>    _a_error_printer = default_printer_t<U>{},
            variant_print_parse_e   _a_mode = variant_print_parse_e::use_indexes
        )
        : _m_value_printer(std::move(_a_value_printer))
        , _m_error_printer(std::move(_a_error_printer))
        , _m_mode(_a_mode)
    {
    }

    __constexpr std::u8string
        run_printer(const value_type& _a_object) const
    {
        using namespace std;
        using enum variant_print_parse_e;
        const size_t _l_idx{ _a_object.has_value() ? size_t(0) : size_t(1) };
        const u8string _l_element_str{
            _a_object.has_value()
                ? _m_value_printer.run_printer(_a_object.value())
                : _m_error_printer.run_printer(_a_object.error())
        };

        switch (_m_mode)
        {
        case no_indexes:
            return _l_element_str;
        case use_indexes:
            return fmt::format(u8"({0}, {1})", _l_idx, _l_element_str);
        default:
            throw errors::unaccounted_for_enum_exception(_m_mode);
        }
    }

private:
    default_printer_t<T>  _m_value_printer;
    default_printer_t<U>  _m_error_printer;
    variant_print_parse_e _m_mode;
};

// ════════════════════════════════════════════════════════════════════
// std::optional
// ════════════════════════════════════════════════════════════════════

template <typename T>
struct default_printer_t<std::optional<T>>
{
    using value_type = std::optional<T>;

    __constexpr
        default_printer_t(
            default_printer_t<T> _a_printer = default_printer_t<T>{}
        )
        : _m_printer(std::move(_a_printer))
    {
    }

    __constexpr std::u8string
        run_printer(const value_type& _a_object) const
    {
        std::u8string _l_rv{ u8"{" };
        if (_a_object.has_value())
        {
            _l_rv.append(_m_printer.run_printer(_a_object.value()));
        }
        _l_rv.append(u8"}");
        return _l_rv;
    }

private:
    default_printer_t<T> _m_printer;
};

// ════════════════════════════════════════════════════════════════════
// std::monostate
// ════════════════════════════════════════════════════════════════════

template <>
struct default_printer_t<std::monostate>
{
    __constexpr std::u8string
        run_printer(const std::monostate&) const
    {
        return u8"std::monostate";
    }
};

// ════════════════════════════════════════════════════════════════════
// std::mt19937_64
// ════════════════════════════════════════════════════════════════════

template <>
struct default_printer_t<std::mt19937_64>
{
    __constexpr std::u8string
        run_printer(const std::mt19937_64&) const
    {
        return u8"std::mt19937_64";
    }
};

// ════════════════════════════════════════════════════════════════════
// std::filesystem::path
// ════════════════════════════════════════════════════════════════════

template <>
struct default_printer_t<std::filesystem::path>
{
    __constexpr std::u8string
        run_printer(const std::filesystem::path& _a_object) const
    {
        return _a_object.u8string();
    }
};

// ════════════════════════════════════════════════════════════════════
// Raw pointers
// ════════════════════════════════════════════════════════════════════

template <typename T>
struct default_printer_t<T*>
{
    __constexpr std::u8string
        run_printer(T* const& _a_object) const
    {
#ifdef UINTPTR_MAX
        using ptr_t = uintptr_t;
#else
        using ptr_t = std::size_t;
#endif
        const ptr_t _l_address{
            reinterpret_cast<ptr_t>(static_cast<void*>(_a_object))
        };
        return default_printer_t<ptr_t>{}.run_printer(_l_address);
    }
};

// ════════════════════════════════════════════════════════════════════
// std::shared_ptr
// ════════════════════════════════════════════════════════════════════

template <typename T>
struct default_printer_t<std::shared_ptr<T>>
{
    using value_type = std::shared_ptr<T>;

    __constexpr
        default_printer_t(
            default_printer_t<T>                  _a_printer = default_printer_t<T>{},
            enum_pointer_print_parse_type_t       _a_mode
            = enum_pointer_print_parse_type_t::AS_OBJECT
        )
        : _m_printer(std::move(_a_printer))
        , _m_mode(_a_mode)
    {
    }

    __constexpr std::u8string
        run_printer(const value_type& _a_object) const
    {
        using namespace _ABC_NS_UTILITY_STR;
        using enum enum_pointer_print_parse_type_t;
        switch (_m_mode)
        {
        case AS_OBJECT:
            return object_printer_with_field_names_and_custom_printers(
                object_printer_parser_t{},
                type_id<decltype(_a_object)>(),
                std::array<std::u8string_view, 2>{u8"address", u8"data"},
                std::make_tuple(
                    default_printer_t<T*>{},
                    _m_printer
                ),
                _a_object.get(),
                * (_a_object.get())
            );
        case JUST_DATA:
            return _m_printer.run_printer(*_a_object.get());
        default:
            throw errors::unaccounted_for_enum_exception(_m_mode);
        }
    }

private:
    default_printer_t<T>            _m_printer;
    enum_pointer_print_parse_type_t _m_mode;
};

// ════════════════════════════════════════════════════════════════════
// hex_printer — decorator that wraps any printer
// ════════════════════════════════════════════════════════════════════

template <typename T, typename InnerPrinter = default_printer_t<T>>
    requires printer_policy<InnerPrinter, T>
struct hex_printer_t
{
    __constexpr
        hex_printer_t(InnerPrinter _a_printer = InnerPrinter{})
        : _m_printer(std::move(_a_printer))
    {
    }

    __constexpr std::u8string
        run_printer(const T& _a_object) const
    {
        return abc::utility::str::to_hex(_m_printer.run_printer(_a_object));
    }

private:
    InnerPrinter _m_printer;
};

// ════════════════════════════════════════════════════════════════════
// Object printer free functions
// ════════════════════════════════════════════════════════════════════

namespace detail
{
    template <std::size_t I, typename PrinterTuple, typename ValueTuple>
    __constexpr void
        object_printer_element(
            const utility::object_printer_parser_t& _a_parser,
            std::u8string& _a_str,
            const std::optional<
            std::array<
            std::u8string_view,
            std::tuple_size_v<ValueTuple>
            >
            >& _a_names,
            const PrinterTuple& _a_printers,
            const ValueTuple& _a_values
        ) noexcept
    {
        using namespace _ABC_NS_UTILITY_STR;

        if (_a_names.has_value())
        {
            _a_str.append(std::get<I>(_a_names.value()));
            if (_a_parser.space_before_field_name_and_field_separator)
            {
                _a_str.append(u8" ");
            }
            _a_str.append(cast_string_to_u8string(
                std::string(1, _a_parser.delimiter_between_field_name_and_field)
            ));
            if (_a_parser.space_after_field_name_and_field_separator)
            {
                _a_str.append(u8" ");
            }
        }

        _a_str.append(
            std::get<I>(_a_printers).run_printer(std::get<I>(_a_values))
        );

        if constexpr (I + 1 < std::tuple_size_v<ValueTuple>)
        {
            if (_a_parser.space_before_field_delimiter)
            {
                _a_str.append(u8" ");
            }
            _a_str.append(cast_string_to_u8string(
                std::string(1, _a_parser.delimiter_between_fields)
            ));
            if (_a_parser.space_after_field_delimieter)
            {
                _a_str.append(u8" ");
            }
            object_printer_element<I + 1>(
                _a_parser, _a_str, _a_names, _a_printers, _a_values
            );
        }
    }

    template <typename PrinterTuple, typename... Ts>
    __constexpr std::u8string
        object_printer_impl(
            const utility::object_printer_parser_t& _a_parser,
            const std::u8string_view& _a_begin_str,
            const std::optional<
            std::array<std::u8string_view, sizeof...(Ts)>
            >& _a_names,
            const PrinterTuple& _a_printers,
            Ts...               _a_values
        )
    {
        using namespace _ABC_NS_UTILITY_STR;

        std::u8string _l_rv{ _a_begin_str };
        if (_a_parser.space_after_object_name)
        {
            _l_rv.append(u8" ");
        }
        _l_rv.append(cast_string_to_u8string(
            std::string(1, _a_parser.begin_char)
        ));

        auto _l_values = std::tie(_a_values...);
        object_printer_element<0>(
            _a_parser, _l_rv, _a_names, _a_printers, _l_values
        );

        _l_rv.append(cast_string_to_u8string(
            std::string(1, _a_parser.end_char)
        ));
        return _l_rv;
    }
} // namespace detail

template <typename... Ts>
__constexpr std::u8string
object_printer(
    const utility::object_printer_parser_t& _a_parser,
    const std::u8string_view& _a_begin_str,
    Ts... _a_values
)
{
    auto _l_printers = std::make_tuple(default_printer_t<Ts>{}...);
    return detail::object_printer_impl(
        _a_parser,
        _a_begin_str,
        std::optional<std::array<std::u8string_view, sizeof...(Ts)>>{},
        _l_printers,
        _a_values...
    );
}

template <typename... Ts, typename... Printers>
__constexpr std::u8string
object_printer_with_custom_printers(
    const utility::object_printer_parser_t& _a_parser,
    const std::u8string_view& _a_begin_str,
    std::tuple<Printers...>                 _a_printers,
    Ts... _a_values
)
{
    return detail::object_printer_impl(
        _a_parser,
        _a_begin_str,
        std::optional<std::array<std::u8string_view, sizeof...(Ts)>>{},
        _a_printers,
        _a_values...
    );
}

template <typename... Ts>
__constexpr std::u8string
object_printer_with_field_names(
    const utility::object_printer_parser_t& _a_parser,
    const std::u8string_view& _a_begin_str,
    const std::array<std::u8string_view, sizeof...(Ts)>& _a_names,
    Ts... _a_values
)
{
    auto _l_printers = std::make_tuple(default_printer_t<Ts>{}...);
    return detail::object_printer_impl(
        _a_parser,
        _a_begin_str,
        _a_names,
        _l_printers,
        _a_values...
    );
}

template <typename... Ts, typename... Printers>
__constexpr std::u8string
object_printer_with_field_names_and_custom_printers(
    const utility::object_printer_parser_t& _a_parser,
    const std::u8string_view& _a_begin_str,
    const std::array<std::u8string_view, sizeof...(Ts)>& _a_names,
    std::tuple<Printers...>                              _a_printers,
    Ts... _a_values
)
{
    return detail::object_printer_impl(
        _a_parser,
        _a_begin_str,
        _a_names,
        _a_printers,
        _a_values...
    );
}

_END_ABC_UTILITY_PRINTER_NS
*/