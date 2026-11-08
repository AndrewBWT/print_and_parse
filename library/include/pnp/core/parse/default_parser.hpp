#pragma once
#include "pnp/internal/macros.hpp"

PNP_NS_BEGIN

/*!
 * @brief Defines the default parser object. It is designed to contain the logic
 * to parse elements of type T.
 *
 * It deliberately is set to false so that the concept default_parseable_c (see
 * pnp/core/concepts.hpp) will reject any default_parser<T> for which T has not
 * been specialised.
 * @tparam T The type of object the default parser works on.
 */
template <typename T>
struct default_parser
{
    static constexpr bool is_specialized = false;
};

PNP_NS_END