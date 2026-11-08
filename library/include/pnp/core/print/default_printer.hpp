#pragma once

#include "pnp/internal/macros.hpp"

PNP_NS_BEGIN

/*!
 * @brief Defines the default printer object. It is designed to contain the
 * logic to print elements of type T.
 *
 * It deliberately is set to false so that the concept default_printable_c (see
 * pnp/core/concepts.hpp) will reject any default_printer<T> for which T has not
 * been specialised.
 * @tparam T The type of object the default printer works on.
 */
template <typename T>
struct default_printer
{
    static constexpr bool is_specialized = false;
};

PNP_NS_END