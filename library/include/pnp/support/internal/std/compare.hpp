#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/alternatives.hpp"
#include "pnp/support/internal/pnp_options_base.hpp"
#include "pnp/support/internal/utility.hpp"

#include <array>
#include <compare>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unicode_bridge.hpp>

PNP_NS_BEGIN

// The ordering types have no default constructor, so parsing starts from these.
template <>
struct placeholder_value<std::strong_ordering>
{
    static constexpr std::strong_ordering
        value()
    {
        return std::strong_ordering::equal;
    }
};

template <>
struct placeholder_value<std::weak_ordering>
{
    static constexpr std::weak_ordering
        value()
    {
        return std::weak_ordering::equivalent;
    }
};

template <>
struct placeholder_value<std::partial_ordering>
{
    static constexpr std::partial_ordering
        value()
    {
        return std::partial_ordering::equivalent;
    }
};

PNP_SUPPORT_NS_BEGIN

template <typename T>
concept std_ordering_c = std::same_as<T, std::strong_ordering>
                         || std::same_as<T, std::weak_ordering>
                         || std::same_as<T, std::partial_ordering>;

// ============================================================ policies

// The standard's own names: less, equal, greater.
struct strong_ordering_pnp_default
{
    static constexpr std::u8string_view less_word    = u8"less";
    static constexpr std::u8string_view equal_word   = u8"equal";
    static constexpr std::u8string_view greater_word = u8"greater";
};

// less, equivalent, greater.
struct weak_ordering_pnp_default
{
    static constexpr std::u8string_view less_word    = u8"less";
    static constexpr std::u8string_view equal_word   = u8"equivalent";
    static constexpr std::u8string_view greater_word = u8"greater";
};

// less, equivalent, greater, unordered.
struct partial_ordering_pnp_default : weak_ordering_pnp_default
{
    static constexpr std::u8string_view unordered_word = u8"unordered";
};

// One character each, for any ordering type: <, =, >, and ? for unordered. A
// fixed width of 1, so orderings can be written back to back: <=>.
struct ordering_pnp_symbols
{
    static constexpr std::u8string_view less_word      = u8"<";
    static constexpr std::u8string_view equal_word     = u8"=";
    static constexpr std::u8string_view greater_word   = u8">";
    static constexpr std::u8string_view unordered_word = u8"?";
};

PNP_INTERNAL_NS_BEGIN
template <typename O>
struct ordering_traits;

template <>
struct ordering_traits<std::strong_ordering>
{
    using default_policy = strong_ordering_pnp_default;
    static constexpr std::array<std::strong_ordering, 3> values{
        std::strong_ordering::less,
        std::strong_ordering::equal,
        std::strong_ordering::greater
    };
    static constexpr std::array<std::u8string_view, 3> descriptions{
        u8"word for less",
        u8"word for equal",
        u8"word for greater"
    };
};

template <>
struct ordering_traits<std::weak_ordering>
{
    using default_policy = weak_ordering_pnp_default;
    static constexpr std::array<std::weak_ordering, 3> values{
        std::weak_ordering::less,
        std::weak_ordering::equivalent,
        std::weak_ordering::greater
    };
    static constexpr std::array<std::u8string_view, 3> descriptions{
        u8"word for less",
        u8"word for equivalent",
        u8"word for greater"
    };
};

template <>
struct ordering_traits<std::partial_ordering>
{
    using default_policy = partial_ordering_pnp_default;
    static constexpr std::array<std::partial_ordering, 4> values{
        std::partial_ordering::less,
        std::partial_ordering::equivalent,
        std::partial_ordering::greater,
        std::partial_ordering::unordered
    };
    static constexpr std::array<std::u8string_view, 4> descriptions{
        u8"word for less",
        u8"word for equivalent",
        u8"word for greater",
        u8"word for unordered"
    };
};

// The rules every ordering's words follow, or nullptr: each non-empty, valid
// UTF-8, with no whitespace at either end, and none a prefix of another, so at
// most one can match.
template <std::size_t K>
constexpr const char*
    ordering_words_error(
        const std::array<std::u8string_view, K>& words_arg
    ) noexcept
{
    for (const std::u8string_view word : words_arg)
    {
        if (word.empty())
        {
            return "ordering_pnp_options words must be non-empty";
        }
        if (not valid_utf8(word))
        {
            return "ordering_pnp_options words must be valid UTF-8";
        }
        if (not no_surrounding_whitespace(word))
        {
            return "ordering_pnp_options words must not start or end with "
                   "whitespace";
        }
    }
    if (not alternatives_distinguishable(words_arg))
    {
        return "ordering_pnp_options words must not be prefixes of each other";
    }
    return nullptr;
}

// Fixed when every word has the same length.
template <std::size_t K>
constexpr std::optional<std::size_t>
    ordering_words_width(
        const std::array<std::u8string_view, K>& words_arg
    ) noexcept
{
    for (const std::u8string_view word : words_arg)
    {
        if (word.size() != words_arg[0].size())
        {
            return std::nullopt;
        }
    }
    return words_arg[0].size();
}

PNP_INTERNAL_NS_END

// ============================================================ options

// The word each ordering prints as. For std::weak_ordering and
// std::partial_ordering, equal_word is the word for equivalent.
template <typename O, typename Ordering_Pnp_Defaults>
struct ordering_pnp_options
{
    std::u8string less_word  = std::u8string(Ordering_Pnp_Defaults::less_word);
    std::u8string equal_word = std::u8string(Ordering_Pnp_Defaults::equal_word);
    std::u8string greater_word
        = std::u8string(Ordering_Pnp_Defaults::greater_word);

    // The words in the order of ordering_traits<O>::values.
    [[nodiscard]]
    constexpr std::array<std::u8string_view, 3>
        words() const noexcept
    {
        return {less_word, equal_word, greater_word};
    }

    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        return PNP_INTERNAL_NAMESPACE::ordering_words_error(words());
    }

    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept
    {
        return PNP_INTERNAL_NAMESPACE::ordering_words_width(words());
    }
};

// std::partial_ordering has a fourth value, unordered.
template <typename Ordering_Pnp_Defaults>
struct ordering_pnp_options<std::partial_ordering, Ordering_Pnp_Defaults>
{
    std::u8string less_word  = std::u8string(Ordering_Pnp_Defaults::less_word);
    std::u8string equal_word = std::u8string(Ordering_Pnp_Defaults::equal_word);
    std::u8string greater_word
        = std::u8string(Ordering_Pnp_Defaults::greater_word);
    std::u8string unordered_word
        = std::u8string(Ordering_Pnp_Defaults::unordered_word);

    [[nodiscard]]
    constexpr std::array<std::u8string_view, 4>
        words() const noexcept
    {
        return {less_word, equal_word, greater_word, unordered_word};
    }

    [[nodiscard]]
    constexpr const char*
        first_error() const noexcept
    {
        return PNP_INTERNAL_NAMESPACE::ordering_words_error(words());
    }

    [[nodiscard]]
    constexpr std::optional<std::size_t>
        fixed_output_width() const noexcept
    {
        return PNP_INTERNAL_NAMESPACE::ordering_words_width(words());
    }
};

template <typename O, typename Ordering_Pnp_Defaults>
concept valid_ordering_pnp_c
    = std_ordering_c<O>
      && valid_default_options_c<
          ordering_pnp_options<O, Ordering_Pnp_Defaults>>;

// ============================================================ the pnp

// Prints and parses std::strong_ordering, std::weak_ordering or
// std::partial_ordering as one word per value.
//
//   pnp::ordering_pnp<std::strong_ordering> p; // less
//   pnp::ordering_pnp<std::strong_ordering, pnp::ordering_pnp_symbols> q; // <
template <
    typename O,
    typename Ordering_Pnp_Defaults =
        typename PNP_INTERNAL_NAMESPACE::ordering_traits<O>::default_policy>
requires valid_ordering_pnp_c<O, Ordering_Pnp_Defaults>
struct ordering_pnp
    : public pnp_configuration<ordering_pnp_options<O, Ordering_Pnp_Defaults>>
{
public:
    using base_t
        = pnp_configuration<ordering_pnp_options<O, Ordering_Pnp_Defaults>>;
    using base_t::_options;
    using base_t::base_t;
    constexpr void
        run_printer(
            const std::same_as<O> auto& object_arg,
            text_sink&                  sink_obj_arg
        ) const;

    constexpr basic_parser_return_value
        run_parser(
            parser_input&  parser_input_arg,
            O&             object_answer,
            parser_run_key run_key_arg
        ) const;
};

PNP_SUPPORT_NS_END
PNP_NS_END

// ============================================================ definitions

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

template <typename O, typename Ordering_Pnp_Defaults>
requires valid_ordering_pnp_c<O, Ordering_Pnp_Defaults>
constexpr void
    ordering_pnp<O, Ordering_Pnp_Defaults>::run_printer(
        const std::same_as<O> auto& object_arg,
        text_sink&                  sink_obj_arg
    ) const
{
    // In the order of the words: less, equal, greater, then (partial only)
    // unordered.
    const auto  words = _options.words();
    std::size_t index = words.size() - 1;
    if (object_arg < 0)
    {
        index = 0;
    }
    else if (object_arg == 0)
    {
        index = 1;
    }
    else if (object_arg > 0)
    {
        index = 2;
    }
    sink_obj_arg.write(words[index]);
}

template <typename O, typename Ordering_Pnp_Defaults>
requires valid_ordering_pnp_c<O, Ordering_Pnp_Defaults>
constexpr basic_parser_return_value
    ordering_pnp<O, Ordering_Pnp_Defaults>::run_parser(
        parser_input&  parser_input_arg,
        O&             object_answer,
        parser_run_key run_key_arg
    ) const
{
    using traits     = PNP_INTERNAL_NAMESPACE::ordering_traits<O>;

    // Each word is an alternative with nothing after it.
    const auto words = _options.words();
    std::array<PNP_INTERNAL_NAMESPACE::alternative_text, traits::values.size()>
        alternatives{};
    for (std::size_t idx = 0; idx < alternatives.size(); ++idx)
    {
        alternatives[idx]
            = {.marker_description = traits::descriptions[idx],
               .open_delimiter     = words[idx]};
    }

    std::size_t chosen = 0;
    if (auto result = PNP_INTERNAL_NAMESPACE::parse_alternatives(
            alternatives,
            parser_input_arg,
            chosen,
            [](std::size_t, parser_input&) -> basic_parser_return_value
            {
                return {};
            }
        );
        result.failed())
    {
        return result;
    }
    object_answer = traits::values[chosen];
    return {};
}

PNP_SUPPORT_NS_END
PNP_NS_END