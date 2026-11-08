#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <array>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unicode_bridge.hpp>
#include <utility>

PNP_NS_BEGIN

// A std::array of elements that can't be default-constructed, but can be parsed
// into (they have a parse_seed of their own, like std::strong_ordering): every
// element starts from its seed.
template <typename T, std::size_t N>
requires (not std::default_initializable<T>) && parse_constructible_c<T>
struct placeholder_value<std::array<T, N>>
{
    static constexpr std::array<T, N>
        value()
    {
        return []<std::size_t... Is>(std::index_sequence<Is...>)
        {
            return std::array<T, N>{
                (( void ) Is, placeholder_value<T>::value())...
            };
        }(std::make_index_sequence<N>{});
    }
};

PNP_SUPPORT_NS_BEGIN

// ============================================================ element types

PNP_INTERNAL_NS_BEGIN

/*!
 * @brief The element type of a sequence. Undefined for types that aren't
 * sequences.
 * @tparam T The sequence.
 */
template <typename T>
struct sequence_value
{};

/*!
 * @brief The element type of a standard-style container: its value_type.
 */
template <typename T>
requires requires { typename T::value_type; }
struct sequence_value<T>
{
    using type = typename T::value_type;
};

/*!
 * @brief The element type of a built-in array.
 */
template <typename T, std::size_t N>
struct sequence_value<T[N]>
{
    using type = T;
};

/*!
 * @brief The type each element is parsed as: the element type, except for maps,
 * whose element type (std::pair<const K, V>) can't be assigned, so elements are
 * parsed as std::pair<K, V>.
 */
template <typename T>
struct sequence_parse_value : sequence_value<T>
{};

template <typename T>
requires requires {
    typename T::key_type;
    typename T::mapped_type;
}
struct sequence_parse_value<T>
{
    using type = std::pair<typename T::key_type, typename T::mapped_type>;
};

PNP_INTERNAL_NS_END

/*!
 * @brief The element type of a sequence, as printed.
 */
template <typename T>
using sequence_value_t =
    typename PNP_INTERNAL_NAMESPACE::sequence_value<T>::type;

/*!
 * @brief The element type of a sequence, as parsed.
 */
template <typename T>
using sequence_parse_value_t =
    typename PNP_INTERNAL_NAMESPACE::sequence_parse_value<T>::type;

// Something a range-for can iterate over.
template <typename T>
concept iterable_c = requires (const T& sequence) {
    std::begin(sequence);
    std::end(sequence);
};

// The elements a sequence prints, in order. For anything iterable, that's the
// object itself; a type that isn't iterable but holds a sequence (std::bitset)
// specialises this to return a view that is.
template <typename T>
struct sequence_elements
{
    static constexpr const T&
        view(
            const T& object_arg
        ) noexcept
    {
        return object_arg;
    }
};

// A sequence that can be printed: its elements view can be iterated.
template <typename T>
concept printable_sequence_c
    = requires (const T& object) { sequence_elements<T>::view(object); }
      && iterable_c<std::remove_cvref_t<
          decltype(sequence_elements<T>::view(std::declval<const T&>()))>>;
template <typename T>
inline constexpr std::optional<std::size_t> sequence_fixed_count_v
    = std::nullopt;

template <typename E, std::size_t N>
inline constexpr std::optional<std::size_t> sequence_fixed_count_v<E[N]> = N;

template <typename E, std::size_t N>
inline constexpr std::optional<std::size_t>
    sequence_fixed_count_v<std::array<E, N>> = N;
// ============================================================ appenders

PNP_INTERNAL_NS_BEGIN
// Move-assigns, element by element for arrays (which can't be assigned
// directly).
template <typename T>
constexpr void
    move_assign(T& destination_arg, T&& source_arg);
PNP_INTERNAL_NS_END
// Adds parsed elements to a container. An appender returns std::nullopt on
// success, or a message describing why the element couldn't be added. It may
// also provide finish(), called after the last element, to report a problem
// with the sequence as a whole.
template <typename T>
struct default_appender; // users specialise for their own containers

// Containers with push_back: vector, deque, list, string-like containers.
template <typename T>
struct push_back_appender
{
public:
    constexpr explicit push_back_appender(T& container_arg) noexcept;

    constexpr std::optional<std::u8string>
        operator()(sequence_parse_value_t<T>&& value_arg);
private:
    T* _container;
};

// Appends with insert: sets, maps and their unordered and multi versions. For
// unique-key containers, a duplicate key is an error rather than being silently
// dropped.
template <typename T>
struct insert_appender
{
public:
    constexpr explicit insert_appender(T& container_arg) noexcept;

    constexpr std::optional<std::u8string>
        operator()(sequence_parse_value_t<T>&& value_arg);
private:
    T* _container;
};

// Appends with insert_after, after the last element so the order is kept:
// forward_list.
template <typename T>
struct insert_after_appender
{
public:
    constexpr explicit insert_after_appender(T& container_arg);

    constexpr std::optional<std::u8string>
        operator()(sequence_parse_value_t<T>&& value_arg);
private:
    T* _container;
    typename T::iterator
        _last; // the element to insert after: before_begin() while empty
};

// Each container's default, chosen by what it supports.
template <typename T>
requires requires (T& container, sequence_parse_value_t<T>&& value) {
    container.push_back(std::move(value));
}
struct default_appender<T> : public push_back_appender<T>
{
    using push_back_appender<T>::push_back_appender;
};

template <typename T>
requires (not requires (T& container, sequence_parse_value_t<T>&& value) {
             container.push_back(std::move(value));
         }) && requires (T& container, sequence_parse_value_t<T>&& value) {
    container.insert(std::move(value));
}
struct default_appender<T> : public insert_appender<T>
{
    using insert_appender<T>::insert_appender;
};

template <typename T>
requires (not requires (T& container, sequence_parse_value_t<T>&& value) {
             container.push_back(std::move(value));
         }) && requires (T& container, sequence_parse_value_t<T>&& value) {
    container.insert_after(container.before_begin(), std::move(value));
}
struct default_appender<T> : public insert_after_appender<T>
{
    using insert_after_appender<T>::insert_after_appender;
};

template <typename T>
constexpr push_back_appender<T>::push_back_appender(
    T& container_arg
) noexcept
    : _container(&container_arg)
{}

template <typename T>
constexpr std::optional<std::u8string>
    push_back_appender<T>::operator()(
        sequence_parse_value_t<T>&& value_arg
    )
{
    _container->push_back(std::move(value_arg));
    return std::nullopt;
}

template <typename T>
constexpr insert_appender<T>::insert_appender(
    T& container_arg
) noexcept
    : _container(&container_arg)
{}

template <typename T>
constexpr std::optional<std::u8string>
    insert_appender<T>::operator()(
        sequence_parse_value_t<T>&& value_arg
    )
{
    // Unique containers return std::pair<iterator, bool>; multi containers
    // return just an iterator, since every insertion succeeds.
    if constexpr (requires { _container->insert(std::move(value_arg)).second; })
    {
        if (not _container->insert(std::move(value_arg)).second)
        {
            return u8"An element with the same key is already in the "
                   u8"container.";
        }
    }
    else
    {
        _container->insert(std::move(value_arg));
    }
    return std::nullopt;
}

template <typename T>
constexpr insert_after_appender<T>::insert_after_appender(
    T& container_arg
)
    : _container(&container_arg), _last(container_arg.before_begin())
{}

template <typename T>
constexpr std::optional<std::u8string>
    insert_after_appender<T>::operator()(
        sequence_parse_value_t<T>&& value_arg
    )
{
    _last = _container->insert_after(_last, std::move(value_arg));
    return std::nullopt;
}

// Containers of a fixed size: built-in arrays and std::array.
template <typename T, typename E, std::size_t N>
struct fixed_size_appender
{
public:
    constexpr explicit fixed_size_appender(T& container_arg) noexcept;

    constexpr std::optional<std::u8string>
        operator()(E&& value_arg);

    // Reports a sequence with too few elements.
    constexpr std::optional<std::u8string>
        finish() const;
private:
    T*          _container;
    std::size_t _count = 0;
};

template <typename E, std::size_t N>
struct default_appender<E[N]> : public fixed_size_appender<E[N], E, N>
{
    using fixed_size_appender<E[N], E, N>::fixed_size_appender;
};

template <typename E, std::size_t N>
struct default_appender<std::array<E, N>>
    : public fixed_size_appender<std::array<E, N>, E, N>
{
    using fixed_size_appender<std::array<E, N>, E, N>::fixed_size_appender;
};

template <typename A, typename T>
concept appender_c
    = std::constructible_from<A, T&>
      && requires (A appender, sequence_parse_value_t<T>&& value) {
             {
                 appender(std::move(value))
             } -> std::convertible_to<std::optional<std::u8string>>;
         };

// ============================================================ policies and
// options

PNP_INTERNAL_NS_BEGIN

// The first rule a sequence configuration breaks, or nullptr. Shared by the
// policy concept (which has no element type to build options with) and the
// options themselves.
constexpr const char*
    sequence_tokens_error(
        const std::u8string_view open_delimiter_arg,
        const std::u8string_view close_delimiter_arg,
        const std::u8string_view separator_arg,
        const std::u8string_view separator_spacing_arg,
        const std::u8string_view delimiter_spacing_arg
    ) noexcept
{
    for (const std::u8string_view token :
         {open_delimiter_arg, close_delimiter_arg, separator_arg})
    {
        if (not valid_utf8(token))
        {
            return "sequence_pnp_options delimiters and separator must be "
                   "valid UTF-8";
        }
        if (not no_surrounding_whitespace(token))
        {
            return "sequence_pnp_options delimiters and separator must not "
                   "start or end with whitespace";
        }
    }
    // After each element, the parser checks for the close delimiter before the
    // separator.
    if (not close_delimiter_arg.empty() && not separator_arg.empty()
        && (close_delimiter_arg.starts_with(separator_arg)
            || separator_arg.starts_with(close_delimiter_arg)))
    {
        return "sequence_pnp_options close delimiter and separator must not be "
               "prefixes of each other";
    }
    if (not only_whitespace(separator_spacing_arg))
    {
        return "sequence_pnp_options separator spacing must contain only "
               "whitespace";
    }
    if (not only_whitespace(delimiter_spacing_arg))
    {
        return "sequence_pnp_options delimiter spacing must contain only "
               "whitespace";
    }
    return nullptr;
}

// Prints a range in the sequence format: delimiters, spacing and separators
// from the options, each element through print_element_arg. Shared by the
// sequence pnp and the range printer, so containers and views print
// identically.
template <typename Options, typename Range, typename Print_Element>
constexpr void
    print_sequence(
        const Options&  options_arg,
        Range&&         range_arg,
        Print_Element&& print_element_arg,
        text_sink&      sink_obj_arg
    );
PNP_INTERNAL_NS_END

// The defaults every sequence policy shares: compact output, strict parsing.
struct sequence_pnp_default_base
{
    static constexpr std::u8string_view separator_spacing        = u8"";
    static constexpr std::u8string_view delimiter_spacing        = u8"";
    static constexpr bool               skip_whitespace          = false;
    static constexpr bool               allow_trailing_separator = false;
};

// Sequences in square brackets, separated by commas: [1,2,3].
struct sequence_pnp_default : sequence_pnp_default_base
{
    static constexpr std::u8string_view open_delimiter  = u8"[";
    static constexpr std::u8string_view close_delimiter = u8"]";
    static constexpr std::u8string_view separator       = u8",";
};

// Sets in braces: {1,2,3}.
struct set_pnp_default : sequence_pnp_default_base
{
    static constexpr std::u8string_view open_delimiter  = u8"{";
    static constexpr std::u8string_view close_delimiter = u8"}";
    static constexpr std::u8string_view separator       = u8",";
};

// Maps in braces, as a sequence of entries: {1:"a",2:"b"}. The entries
// themselves are pairs, printed by the tuple pnp with map_entry_pnp_default (in
// tuples.hpp).
struct map_pnp_default : sequence_pnp_default_base
{
    static constexpr std::u8string_view open_delimiter  = u8"{";
    static constexpr std::u8string_view close_delimiter = u8"}";
    static constexpr std::u8string_view separator       = u8",";
};

template <typename Sequence_Pnp_Defaults>
concept valid_sequence_pnp_c = requires {
    requires std::
        convertible_to<decltype(Sequence_Pnp_Defaults::skip_whitespace), bool>;
    requires std::convertible_to<
        decltype(Sequence_Pnp_Defaults::allow_trailing_separator),
        bool>;
    requires PNP_INTERNAL_NAMESPACE::sequence_tokens_error(
                 Sequence_Pnp_Defaults::open_delimiter,
                 Sequence_Pnp_Defaults::close_delimiter,
                 Sequence_Pnp_Defaults::separator,
                 Sequence_Pnp_Defaults::separator_spacing,
                 Sequence_Pnp_Defaults::delimiter_spacing
             ) == nullptr;
};

struct varying_sequence
{};

// The sequence's own configuration, plus the element pnp as a part. The element
// validated itself when it was built, so first_error() only checks the
// sequence's own fields.
//
// Delimiters and the separator mark structure, and must not contain surrounding
// whitespace. Layout belongs in the spacing fields, which must contain only
// whitespace:
//   separator_spacing  printed after each separator:            [1, 2, 3]
//   delimiter_spacing  printed inside both delimiters:          [ 1, 2, 3 ]
// An empty sequence prints as just the delimiters, with no spacing: [].
//
// Parsing only:
//   skip_whitespace           accept any whitespace around elements, delimiters
//   and
//                             separators, instead of exactly the configured
//                             spacing
//   allow_trailing_separator  accept a separator after the last element:
//   [1,2,3,]
template <
    typename Element_Pnp,
    typename Sequence_Pnp_Defaults,
    typename Sequence>
requires valid_sequence_pnp_c<Sequence_Pnp_Defaults>
struct sequence_pnp_options
{
    using element_part_type = Element_Pnp;
    static constexpr std::optional<std::size_t> element_count
        = sequence_fixed_count_v<Sequence>;
    std::u8string open_delimiter
        = std::u8string(Sequence_Pnp_Defaults::open_delimiter);
    element_part_type element = {};
    std::u8string separator   = std::u8string(Sequence_Pnp_Defaults::separator);
    std::u8string close_delimiter
        = std::u8string(Sequence_Pnp_Defaults::close_delimiter);
    std::u8string separator_spacing
        = std::u8string(Sequence_Pnp_Defaults::separator_spacing);
    std::u8string delimiter_spacing
        = std::u8string(Sequence_Pnp_Defaults::delimiter_spacing);
    bool skip_whitespace = Sequence_Pnp_Defaults::skip_whitespace;
    bool allow_trailing_separator
        = Sequence_Pnp_Defaults::allow_trailing_separator;
    // static constexpr std::optional<std::size_t> element_count =
    // sequence_fixed_count_v<Sequence>;
    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept;
    // Fixed for a fixed number of fixed-width elements, parsed strictly.
    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept;
};

// ============================================================ the pnp

// Prints and parses a sequence of elements with an element pnp. run_printer
// exists when T is iterable and the element pnp can print; run_parser when the
// element pnp can parse, elements can be constructed to parse into, and the
// appender fits T.
//
//   using vector_pnp = pnp::generic_sequence_pnp<std::vector<int>,
//   pnp::default_printer<int>>; vector_pnp p; // [1,2,3] vector_pnp
//   q({.separator_spacing = u8" "});                           // [1, 2, 3]
//   vector_pnp r({.skip_whitespace = true});                              //
//   parses "[ 1 , 2,3 ]"
template <
    typename T,
    typename Element_Pnp,
    typename Sequence_Pnp_Defaults = sequence_pnp_default,
    typename Appender = default_appender<T>>
    requires valid_sequence_pnp_c<Sequence_Pnp_Defaults>
&& (printer_c<Element_Pnp, sequence_value_t<T>> || parser_c<Element_Pnp, sequence_parse_value_t<T>>)
struct generic_sequence_pnp : public pnp_configuration<sequence_pnp_options<Element_Pnp, Sequence_Pnp_Defaults, T>>
{
public:
    using base_t = pnp_configuration<
        sequence_pnp_options<Element_Pnp, Sequence_Pnp_Defaults, T>>;
    using base_t::_options;
    using base_t::base_t;

    using element_part_type = Element_Pnp;
    constexpr void
        run_printer(
            const std::same_as<T> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const
    requires printable_sequence_c<T>
             && printer_c<Element_Pnp, sequence_value_t<T>>;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            T&             object_answer,
            parser_run_key run_key_arg
        ) const
    requires parser_c<Element_Pnp, sequence_parse_value_t<T>>
             && parse_constructible_c<sequence_parse_value_t<T>>
             && parse_constructible_c<T> && appender_c<Appender, T>;
private:
    // Consumes the closing delimiter (with the spacing before it, in strict
    // mode) and returns true, or leaves the input alone and returns false.
    constexpr bool
        try_close(parser_input& parser_input_arg) const;

    // Handles the whitespace after a separator: skipped when lenient, the
    // configured spacing when strict. Returns a failure if the strict spacing
    // is missing.
    constexpr basic_parser_return_value
        consume_separator_spacing(parser_input& parser_input_arg) const;

    // After an element: the failure for input that is neither a separator nor
    // the close.
    constexpr basic_parser_return_value
        missing_separator_or_close(parser_input& parser_input_arg) const;
};

// ============================================================ container
// adaptors

PNP_INTERNAL_NS_BEGIN
// The container an adaptor (stack, queue, priority_queue) wraps. The standard
// adaptors keep it in a protected member, c, reachable through a pointer to
// member of a derived class.
template <typename Adaptor>
constexpr const typename Adaptor::container_type&
    underlying_container(const Adaptor& adaptor_arg) noexcept;

// The comparator an adaptor needs to be rebuilt (priority_queue), or an empty
// placeholder.
template <typename Adaptor>
struct adaptor_comparator
{
    struct none
    {};

    using type = none;
};

template <typename Adaptor>
requires requires { typename Adaptor::value_compare; }
struct adaptor_comparator<Adaptor>
{
    using type = typename Adaptor::value_compare;
};

template <typename Adaptor>
inline constexpr bool has_comparator_v
    = requires { typename Adaptor::value_compare; };
PNP_INTERNAL_NS_END

// The underlying container's pnp as a part, plus the comparator a
// priority_queue is rebuilt with. The part validated itself, and a comparator
// can't be invalid, so no first_error().
template <typename Adaptor, typename Container_Pnp>
struct adaptor_pnp_options
{
    using container_part_type = Container_Pnp;
    using comparator_type =
        typename PNP_INTERNAL_NAMESPACE::adaptor_comparator<Adaptor>::type;

    container_part_type container  = {};
    comparator_type     comparator = {}; // used only by adaptors that have one

    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept
    {
        return fixed_output_width_of(container);
    }

    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        return nullptr;
    }
};

// Prints and parses a container adaptor (stack, queue, priority_queue) through
// its underlying container, so it prints exactly as that container does. A
// priority_queue prints in its internal heap order, not sorted order; parsing
// rebuilds the heap.
//
//   using stack_pnp = pnp::generic_adaptor_pnp<std::stack<int>,
//   pnp::default_printer<std::deque<int>>>; stack_pnp p; // [1,2,3], bottom to
//   top
template <typename Adaptor, typename Container_Pnp>
requires printer_c<Container_Pnp, typename Adaptor::container_type>
         || parser_c<Container_Pnp, typename Adaptor::container_type>
struct generic_adaptor_pnp
    : public pnp_configuration<adaptor_pnp_options<Adaptor, Container_Pnp>>
{
public:
    using base_t
        = pnp_configuration<adaptor_pnp_options<Adaptor, Container_Pnp>>;
    using base_t::_options;
    using base_t::base_t;

    using container_part_type = Container_Pnp;
    constexpr void
        run_printer(
            const std::same_as<Adaptor> auto& object_arg,
            text_sink&                        sink_obj_arg
        ) const
    requires printer_c<Container_Pnp, typename Adaptor::container_type>;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            Adaptor&       object_answer,
            parser_run_key run_key_arg
        ) const
    requires parser_c<Container_Pnp, typename Adaptor::container_type>
             && parse_constructible_c<typename Adaptor::container_type>;
};

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

PNP_INTERNAL_NS_BEGIN
template <typename T>
constexpr void
    move_assign(
        T&  destination_arg,
        T&& source_arg
    )
{
    if constexpr (std::is_array_v<T>)
    {
        for (std::size_t idx = 0; idx < std::extent_v<T>; ++idx)
        {
            move_assign(destination_arg[idx], std::move(source_arg[idx]));
        }
    }
    else
    {
        destination_arg = std::move(source_arg);
    }
}

template <typename Adaptor>
constexpr const typename Adaptor::container_type&
    underlying_container(
        const Adaptor& adaptor_arg
    ) noexcept
{
    struct access : Adaptor
    {
        static constexpr const typename Adaptor::container_type&
            get(
                const Adaptor& adaptor_arg
            ) noexcept
        {
            return adaptor_arg.*(&access::c);
        }
    };

    return access::get(adaptor_arg);
}

PNP_INTERNAL_NS_END

// ------------------------------------------------------------ appenders

template <typename T, typename E, std::size_t N>
constexpr fixed_size_appender<T, E, N>::fixed_size_appender(
    T& container_arg
) noexcept
    : _container(&container_arg)
{}

template <typename T, typename E, std::size_t N>
constexpr std::optional<std::u8string>
    fixed_size_appender<T, E, N>::operator()(
        E&& value_arg
    )
{
    if (_count == N)
    {
        return u8"Too many elements; expected exactly "
               + PNP_INTERNAL_NAMESPACE::size_to_u8string(N) + u8".";
    }
    PNP_INTERNAL_NAMESPACE::move_assign(
        (*_container)[_count], std::move(value_arg)
    );
    ++_count;
    return std::nullopt;
}

template <typename T, typename E, std::size_t N>
constexpr std::optional<std::u8string>
    fixed_size_appender<T, E, N>::finish() const
{
    if (_count != N)
    {
        return u8"Expected " + PNP_INTERNAL_NAMESPACE::size_to_u8string(N)
               + u8" elements, but found "
               + PNP_INTERNAL_NAMESPACE::size_to_u8string(_count) + u8".";
    }
    return std::nullopt;
}

PNP_INTERNAL_NS_BEGIN
template <typename Options, typename Range, typename Print_Element>
constexpr void
    print_sequence(
        const Options&  options_arg,
        Range&&         range_arg,
        Print_Element&& print_element_arg,
        text_sink&      sink_obj_arg
    )
{
    sink_obj_arg.write(std::u8string_view(options_arg.open_delimiter));
    bool first = true;
    for (auto&& element : range_arg)
    {
        if (first)
        {
            // Spacing only goes inside a non-empty sequence: an empty one
            // prints as [].
            sink_obj_arg.write(std::u8string_view(options_arg.delimiter_spacing)
            );
        }
        else
        {
            sink_obj_arg.write(std::u8string_view(options_arg.separator));
            sink_obj_arg.write(std::u8string_view(options_arg.separator_spacing)
            );
        }
        first = false;
        print_element_arg(std::forward<decltype(element)>(element));
    }
    if (not first)
    {
        sink_obj_arg.write(std::u8string_view(options_arg.delimiter_spacing));
    }
    sink_obj_arg.write(std::u8string_view(options_arg.close_delimiter));
}

PNP_INTERNAL_NS_END

// ------------------------------------------------------------ options


template <
    typename Element_Pnp,
    typename Sequence_Pnp_Defaults,
    typename Sequence>
requires valid_sequence_pnp_c<Sequence_Pnp_Defaults>
constexpr const char*
    sequence_pnp_options<Element_Pnp, Sequence_Pnp_Defaults, Sequence>::
        first_error() const noexcept
{
    if (const char* error = PNP_INTERNAL_NAMESPACE::sequence_tokens_error(
            open_delimiter,
            close_delimiter,
            separator,
            separator_spacing,
            delimiter_spacing
        );
        error != nullptr)
    {
        return error;
    }

    const bool fixed_width_elements
        = fixed_output_width_of(element).has_value();
    if (separator.empty() && not fixed_width_elements)
    {
        return "sequence_pnp_options separator may only be empty when every "
               "element prints with the same width";
    }
    if ((open_delimiter.empty() || close_delimiter.empty())
        && not (element_count.has_value() && fixed_width_elements))
    {
        return "sequence_pnp_options delimiters may only be empty for a fixed "
               "number of elements that all print with "
               "the same width";
    }
    return nullptr;
}

template <
    typename Element_Pnp,
    typename Sequence_Pnp_Defaults,
    typename Sequence>
requires valid_sequence_pnp_c<Sequence_Pnp_Defaults>
constexpr std::optional<std::size_t>
    sequence_pnp_options<Element_Pnp, Sequence_Pnp_Defaults, Sequence>::
        fixed_output_width() const noexcept
{
    if constexpr (not element_count.has_value())
    {
        return std::nullopt;
    }
    else
    {
        // Lenient parsing consumes a varying amount of whitespace.
        const std::optional<std::size_t> element_width
            = fixed_output_width_of(element);
        if (skip_whitespace || not element_width.has_value())
        {
            return std::nullopt;
        }
        constexpr std::size_t count = element_count.value();
        std::size_t width = open_delimiter.size() + close_delimiter.size();
        if constexpr (count != 0)
        {
            // Spacing only goes inside a non-empty sequence.
            width += 2 * delimiter_spacing.size()
                     + count * element_width.value()
                     + (count - 1)
                           * (separator.size() + separator_spacing.size());
        }
        return width;
    }
}

// ------------------------------------------------------------ the sequence pnp

template <typename T, typename Element_Pnp, typename Sequence_Pnp_Defaults, typename Appender>
    requires valid_sequence_pnp_c<Sequence_Pnp_Defaults>
&& (printer_c<Element_Pnp, sequence_value_t<T>> || parser_c<Element_Pnp, sequence_parse_value_t<T>>)
constexpr void
generic_sequence_pnp<T, Element_Pnp, Sequence_Pnp_Defaults, Appender>::run_printer(
    const std::same_as<T> auto& object_arg,
    text_sink& sink_obj_arg
) const requires printable_sequence_c<T>
&& printer_c<Element_Pnp, sequence_value_t<T>>
{
    PNP_INTERNAL_NAMESPACE::print_sequence(
        _options,
        sequence_elements<T>::view(object_arg),
        [&](const auto& element_arg)
        {
            _options.element.run_printer(element_arg, sink_obj_arg);
        },
        sink_obj_arg
    );
}

template <typename T, typename Element_Pnp, typename Sequence_Pnp_Defaults, typename Appender>
    requires valid_sequence_pnp_c<Sequence_Pnp_Defaults>
&& (printer_c<Element_Pnp, sequence_value_t<T>> || parser_c<Element_Pnp, sequence_parse_value_t<T>>)
constexpr bool
generic_sequence_pnp<T, Element_Pnp, Sequence_Pnp_Defaults, Appender>::try_close(
    parser_input& parser_input_arg
) const
{
    if (_options.skip_whitespace)
    {
        parser_input_arg.skip_whitespace();
        return parser_input_arg.advance_if_starts_with(_options.close_delimiter
        );
    }

    // Strict: the spacing and the closing delimiter must both be there, so look
    // ahead before consuming either. Otherwise the input is left for the
    // separator check.
    if (parser_input_arg.starts_with(
            {_options.delimiter_spacing, _options.close_delimiter}
        ))
    {
        parser_input_arg.advance(
            _options.delimiter_spacing.size() + _options.close_delimiter.size()
        );
        return true;
    }
    return false;
}

template <typename T, typename Element_Pnp, typename Sequence_Pnp_Defaults, typename Appender>
    requires valid_sequence_pnp_c<Sequence_Pnp_Defaults>
&& (printer_c<Element_Pnp, sequence_value_t<T>> || parser_c<Element_Pnp, sequence_parse_value_t<T>>)
constexpr basic_parser_return_value
generic_sequence_pnp<T, Element_Pnp, Sequence_Pnp_Defaults, Appender>::consume_separator_spacing(
    parser_input& parser_input_arg
) const
{
    if (_options.skip_whitespace)
    {
        parser_input_arg.skip_whitespace();
        return {};
    }
    if (not parser_input_arg.advance_if_starts_with(_options.separator_spacing))
    {
        return parser_input_arg.fail(
            u8"Expected the separator spacing "
            + PNP_INTERNAL_NAMESPACE::quoted(_options.separator_spacing)
            + u8" after the separator."
        );
    }
    return {};
}

template <typename T, typename Element_Pnp, typename Sequence_Pnp_Defaults, typename Appender>
    requires valid_sequence_pnp_c<Sequence_Pnp_Defaults>
&& (printer_c<Element_Pnp, sequence_value_t<T>> || parser_c<Element_Pnp, sequence_parse_value_t<T>>)
constexpr basic_parser_return_value
generic_sequence_pnp<T, Element_Pnp, Sequence_Pnp_Defaults, Appender>::missing_separator_or_close(
    parser_input& parser_input_arg
) const
{
    // In strict mode, a closing delimiter without the spacing before it gets
    // its own message: "expected ]" would be confusing when the ] is right
    // there.
    if (not _options.skip_whitespace && not _options.delimiter_spacing.empty()
        && parser_input_arg.starts_with(_options.close_delimiter))
    {
        return parser_input_arg.fail(
            u8"Expected the delimiter spacing "
            + PNP_INTERNAL_NAMESPACE::quoted(_options.delimiter_spacing)
            + u8" before the closing delimiter "
            + PNP_INTERNAL_NAMESPACE::quoted(_options.close_delimiter) + u8"."
        );
    }
    return parser_input_arg.fail(
        u8"Expected separator "
        + PNP_INTERNAL_NAMESPACE::quoted(_options.separator)
        + u8" or closing delimiter "
        + PNP_INTERNAL_NAMESPACE::quoted(_options.close_delimiter) + u8"."
    );
}

template <typename T, typename Element_Pnp, typename Sequence_Pnp_Defaults, typename Appender>
    requires valid_sequence_pnp_c<Sequence_Pnp_Defaults>
&& (printer_c<Element_Pnp, sequence_value_t<T>> || parser_c<Element_Pnp, sequence_parse_value_t<T>>)
constexpr basic_parser_return_value
generic_sequence_pnp<T, Element_Pnp, Sequence_Pnp_Defaults, Appender>::run_parser(
    parser_input& parser_input_arg,
    T& object_answer,
    parser_run_key run_key_arg
) const
    requires parser_c<Element_Pnp, sequence_parse_value_t<T>>&& parse_constructible_c<sequence_parse_value_t<T>>
&& parse_constructible_c<T>&& appender_c<Appender, T>
{
    using value_type = sequence_parse_value_t<T>;

    if (not parser_input_arg.advance_if_starts_with(_options.open_delimiter))
    {
        return parser_input_arg.fail(
            u8"Expected opening delimiter "
            + PNP_INTERNAL_NAMESPACE::quoted(_options.open_delimiter) + u8"."
        );
    }

    // Parsed into a temporary, so a failed parse leaves the caller's object
    // untouched.
    // Parsed into a temporary, so a failed parse leaves the caller's object
    // untouched. It starts from T's parse seed if T can't be
    // default-constructed (an array of orderings, say).
    PNP_INTERNAL_NAMESPACE::parse_target<T> target;
    T&                                      result = target.value;
    Appender                                append(result);

    // An empty sequence: just the delimiters (with any whitespace between, when
    // lenient).
    if (_options.skip_whitespace)
    {
        parser_input_arg.skip_whitespace();
    }
    // With no close delimiter (only allowed for a fixed count), the count says
    // when to stop, and an empty close delimiter can't be used to spot an empty
    // sequence: it always matches.
    constexpr std::optional<std::size_t> count = sequence_fixed_count_v<T>;
    const bool                           count_driven
        = count.has_value() && _options.close_delimiter.empty();

    const bool empty = count_driven ? count.value() == 0
                                    : parser_input_arg.advance_if_starts_with(
                                          _options.close_delimiter
                                      );

    if (not empty)
    {
        // Strict: a non-empty sequence starts with the delimiter spacing.
        if (not _options.skip_whitespace
            && not parser_input_arg.advance_if_starts_with(
                _options.delimiter_spacing
            ))
        {
            return parser_input_arg.fail(
                u8"Expected the delimiter spacing "
                + PNP_INTERNAL_NAMESPACE::quoted(_options.delimiter_spacing)
                + u8" after the opening delimiter."
            );
        }

        for (std::size_t idx = 0;; ++idx)
        {
            // Recorded so append errors point at the element, not past it.
            const std::size_t element_start = parser_input_arg.position();

            PNP_INTERNAL_NAMESPACE::parse_target<value_type> target;
            value_type& element = target.value;
            if (auto element_result
                = parser_input_arg.run_parser(_options.element, element);
                element_result.failed())
            {
                element_result.add_msg(
                    u8"while parsing element "
                    + PNP_INTERNAL_NAMESPACE::size_to_u8string(idx)
                    + u8" of a sequence"
                );
                return element_result;
            }

            if (auto append_error = append(std::move(element));
                append_error.has_value())
            {
                return parser_input_arg.fail(
                    u8"Could not add element "
                        + PNP_INTERNAL_NAMESPACE::size_to_u8string(idx)
                        + u8" to the sequence: " + append_error.value(),
                    element_start
                );
            }
            if (count_driven)
            {
                // The last element: only the spacing before the (empty) close
                // delimiter is left.
                if (idx + 1 == count.value())
                {
                    if (_options.skip_whitespace)
                    {
                        parser_input_arg.skip_whitespace();
                    }
                    else if (not parser_input_arg.advance_if_starts_with(
                                 _options.delimiter_spacing
                             ))
                    {
                        return parser_input_arg.fail(
                            u8"Expected the delimiter spacing "
                            + PNP_INTERNAL_NAMESPACE::quoted(
                                _options.delimiter_spacing
                            )
                            + u8" after the last element."
                        );
                    }
                    break;
                }
                // Otherwise, a separator (possibly empty) and the next element.
                if (not parser_input_arg.advance_if_starts_with(
                        _options.separator
                    ))
                {
                    return parser_input_arg.fail(
                        u8"Expected separator "
                        + PNP_INTERNAL_NAMESPACE::quoted(_options.separator)
                        + u8" after element "
                        + PNP_INTERNAL_NAMESPACE::size_to_u8string(idx) + u8"."
                    );
                }
                if (auto spacing_result
                    = consume_separator_spacing(parser_input_arg);
                    spacing_result.failed())
                {
                    return spacing_result;
                }
                continue;
            }
            // The close delimiter is checked first; the two are validated never
            // to be prefixes of each other, so the order can't cause a misread.
            if (try_close(parser_input_arg))
            {
                break;
            }
            if (not parser_input_arg.advance_if_starts_with(_options.separator))
            {
                return missing_separator_or_close(parser_input_arg);
            }
            if (auto spacing_result
                = consume_separator_spacing(parser_input_arg);
                spacing_result.failed())
            {
                return spacing_result;
            }

            // A trailing separator: the closing delimiter straight after a
            // separator.
            if (_options.allow_trailing_separator
                && try_close(parser_input_arg))
            {
                break;
            }
        }
    }

    if constexpr (requires { append.finish(); })
    {
        if (auto finish_error = append.finish(); finish_error.has_value())
        {
            return parser_input_arg.fail(std::move(finish_error.value()));
        }
    }

    PNP_INTERNAL_NAMESPACE::move_assign(object_answer, std::move(result));
    return {};
}

// ------------------------------------------------------------ the adaptor pnp

template <typename Adaptor, typename Container_Pnp>
requires printer_c<Container_Pnp, typename Adaptor::container_type>
         || parser_c<Container_Pnp, typename Adaptor::container_type>
         constexpr void
         generic_adaptor_pnp<Adaptor, Container_Pnp>::run_printer(
             const std::same_as<Adaptor> auto& object_arg,
             text_sink&                        sink_obj_arg
         ) const
         requires printer_c<Container_Pnp, typename Adaptor::container_type>
{
    _options.container.run_printer(
        PNP_INTERNAL_NAMESPACE::underlying_container(object_arg), sink_obj_arg
    );
}

template <typename Adaptor, typename Container_Pnp>
requires printer_c<Container_Pnp, typename Adaptor::container_type>
         || parser_c<Container_Pnp, typename Adaptor::container_type>
         constexpr basic_parser_return_value
         generic_adaptor_pnp<Adaptor, Container_Pnp>::run_parser(
             parser_input&  parser_input_arg,
             Adaptor&       object_answer,
             parser_run_key run_key_arg
         ) const
         requires parser_c<Container_Pnp, typename Adaptor::container_type>
                  && parse_constructible_c<typename Adaptor::container_type>
{
    PNP_INTERNAL_NAMESPACE::parse_target<typename Adaptor::container_type>
        target;
    if (auto container_result
        = parser_input_arg.run_parser(_options.container, target.value);
        container_result.failed())
    {
        return container_result;
    }

    // Rebuilt from the parsed container; a priority_queue also takes its
    // comparator, and re-establishes its heap order from the elements.
    if constexpr (PNP_INTERNAL_NAMESPACE::has_comparator_v<Adaptor>)
    {
        object_answer = Adaptor(_options.comparator, std::move(target.value));
    }
    else
    {
        object_answer = Adaptor(std::move(target.value));
    }
    return {};
}

PNP_SUPPORT_NS_END
PNP_NS_END