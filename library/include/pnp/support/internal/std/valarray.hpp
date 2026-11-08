#pragma once

#include "pnp/support/internal/sequences.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <valarray>
#include <vector>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// A valarray can't grow, so the elements are collected first, and the valarray is built from
// them once the sequence is complete.
template <typename T>
struct valarray_appender
{
public:
    constexpr explicit valarray_appender(std::valarray<T>& valarray_arg) noexcept
        : _valarray(&valarray_arg)
    {
    }

    std::optional<std::u8string>
        operator()(T&& value_arg)
    {
        _elements.push_back(std::move(value_arg));
        return std::nullopt;
    }

    // Builds the valarray from the collected elements.
    std::optional<std::u8string>
        finish()
    {
        *_valarray = std::valarray<T>(_elements.data(), _elements.size());
        return std::nullopt;
    }

private:
    std::valarray<T>* _valarray;
    std::vector<T>    _elements;
};

template <typename T>
struct default_appender<std::valarray<T>> : public valarray_appender<T>
{
    using valarray_appender<T>::valarray_appender;
};

PNP_SUPPORT_NS_END
PNP_NS_END