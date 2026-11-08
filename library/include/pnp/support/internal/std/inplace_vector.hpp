#pragma once

#include "pnp/support/internal/sequences.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <version>

#if defined(__cpp_lib_inplace_vector)
#include <inplace_vector>

PNP_NS_BEGIN
PNP_SUPPORT_NS_BEGIN

// An inplace_vector has a fixed capacity: an element past it is reported, rather than thrown
// as push_back would.
template <typename T, std::size_t N>
struct inplace_vector_appender
{
public:
    constexpr explicit inplace_vector_appender(std::inplace_vector<T, N>& vector_arg) noexcept
        : _vector(&vector_arg)
    {
    }

    constexpr std::optional<std::u8string>
        operator()(T&& value_arg)
    {
        if (_vector->try_push_back(std::move(value_arg)) == nullptr)
        {
            return u8"Too many elements; an inplace_vector of capacity " + PNP_INTERNAL_NAMESPACE::size_to_u8string(N)
                + u8" is full.";
        }
        return std::nullopt;
    }

private:
    std::inplace_vector<T, N>* _vector;
};

template <typename T, std::size_t N>
struct default_appender<std::inplace_vector<T, N>> : public inplace_vector_appender<T, N>
{
    using inplace_vector_appender<T, N>::inplace_vector_appender;
};

PNP_SUPPORT_NS_END
PNP_NS_END

#endif