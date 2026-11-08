#pragma once

#include "pnp/core.hpp"
#include "pnp/support/internal/fixed_width_pnp.hpp"
#include "pnp/support/internal/sequences.hpp"
#include "pnp/support/internal/utility.hpp"

#include <bitset>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// ============================================================ a bitset as a sequence of bits

// A std::bitset's bits as a sequence, most significant first, as std::bitset::to_string()
// writes them.
template <std::size_t N>
struct bitset_bits_view
{
    struct iterator
    {
        const std::bitset<N>* bits;
        std::size_t           remaining;   // the bits still to come; the next is bit remaining - 1

        constexpr bool
            operator*() const
        {
            return bits->test(remaining - 1);
        }

        constexpr iterator&
            operator++() noexcept
        {
            --remaining;
            return *this;
        }

        constexpr bool
            operator==(const iterator& other_arg) const noexcept
        {
            return remaining == other_arg.remaining;
        }
    };

    const std::bitset<N>* bits;

    constexpr iterator
        begin() const noexcept
    {
        return { bits, N };
    }

    constexpr iterator
        end() const noexcept
    {
        return { bits, 0 };
    }
};

template <std::size_t N>
struct sequence_elements<std::bitset<N>>
{
    static constexpr bitset_bits_view<N>
        view(const std::bitset<N>& object_arg) noexcept
    {
        return { &object_arg };
    }
};

PNP_INTERNAL_NS_BEGIN
template <std::size_t N>
struct sequence_value<std::bitset<N>>
{
    using type = bool;
};
PNP_INTERNAL_NS_END

// A bitset always has N bits, so with no delimiters, exactly N are read.
template <std::size_t N>
inline constexpr std::optional<std::size_t> sequence_fixed_count_v<std::bitset<N>> = N;

// Shifts each bit in from the low end, so a full bitset lands as printed, and a short one (only
// possible with delimiters, which mark where it ends) fills the low bits: [101] is 00000101.
template <std::size_t N>
struct bitset_appender
{
public:
    constexpr explicit bitset_appender(std::bitset<N>& bitset_arg) noexcept
        : _bitset(&bitset_arg)
    {
    }

    constexpr std::optional<std::u8string>
        operator()(bool&& bit_arg)
    {
        if (_count == N)
        {
            return u8"Too many bits; a bitset of " + PNP_INTERNAL_NAMESPACE::size_to_u8string(N) + u8" holds at most "
                + PNP_INTERNAL_NAMESPACE::size_to_u8string(N) + u8".";
        }
        *_bitset <<= 1;
        _bitset->set(0, bit_arg);
        ++_count;
        return std::nullopt;
    }

private:
    std::bitset<N>* _bitset;
    std::size_t     _count = 0;
};

template <std::size_t N>
struct default_appender<std::bitset<N>> : public bitset_appender<N>
{
    using bitset_appender<N>::bitset_appender;
};

// ============================================================ policies

// One bit as a binary digit with no prefix, padded to its width (one digit, since a bool's
// largest value is 1), so it has a fixed width: 0 or 1.
struct bitset_bit_default : fixed_width_pnp_binary
{
    static constexpr std::u8string_view prefix = u8"";
    static constexpr bool               fixed_size_output = true;
};

using bitset_bit_pnp = fixed_width_pnp<bool, bitset_bit_default>;

// No delimiters or separator: 00000101, exactly as std::bitset::to_string() writes it, and
// exactly N bits when parsing. Configure delimiters ([00000101]) to allow fewer bits when
// parsing, filling the low end.
struct bitset_pnp_default : sequence_pnp_default_base
{
    static constexpr std::u8string_view open_delimiter = u8"";
    static constexpr std::u8string_view close_delimiter = u8"";
    static constexpr std::u8string_view separator = u8"";
};

// The bitset pnp: a sequence of bits, with the bit pnp as its element part.
//
//   pnp::generic_bitset_pnp<8> p;                                                 // 00000101
//   pnp::generic_bitset_pnp<8> q({.open_delimiter = u8"[", .close_delimiter = u8"]"});   // [00000101], and [101] parses
template <std::size_t N, typename Bit_Pnp = bitset_bit_pnp, typename Bitset_Pnp_Defaults = bitset_pnp_default>
using generic_bitset_pnp = generic_sequence_pnp<std::bitset<N>, Bit_Pnp, Bitset_Pnp_Defaults>;

PNP_SUPPORT_NS_END
PNP_NS_END