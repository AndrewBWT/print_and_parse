#pragma once
#include "pnp/core/parse/placeholder.hpp"
#include "pnp_tests_internal.hpp"

#include <string>

namespace pnp_tests::placeholder_value_tests
{
    // No default constructor; has a placeholder.
    struct no_default
    {
        constexpr explicit no_default(
            const int n_arg
        )
            : n(n_arg)
        {
        }

        int n;
    };

    // Both a default constructor (n = 1) and a placeholder (n = 2): the
    // placeholder must win.
    struct both
    {
        int n = 1;
    };

    // A default constructor (n = 3); no placeholder.
    struct plain
    {
        int n = 3;
    };

    // Neither: pnp can't make one.
    struct neither
    {
        constexpr explicit neither(
            const int n_arg
        )
            : n(n_arg)
        {
        }

        int n;
    };

    // Specialisations that don't count: value() returns the wrong type, or isn't
    // static.
    struct wrong_return_t
    {
        constexpr explicit wrong_return_t(
            const int n_arg
        )
            : n(n_arg)
        {
        }

        int n;
    };

    struct not_static
    {
        constexpr explicit not_static(
            const int n_arg
        )
            : n(n_arg)
        {
        }

        int n;
    };

    // A template with a generic placeholder, built from its part's placeholder: the
    // shape the pair and tuple placeholders will have.
    template <typename T>
    struct box
    {
        T inner;
    };
} // namespace pnp_tests::placeholder_value_tests

PNP_NS_BEGIN
template <>
struct placeholder_value<pnp_tests::placeholder_value_tests::no_default>
{
    static constexpr pnp_tests::placeholder_value_tests::no_default
        value()
    {
        return pnp_tests::placeholder_value_tests::no_default(7);
    }
};

template <>
struct placeholder_value<pnp_tests::placeholder_value_tests::both>
{
    static constexpr pnp_tests::placeholder_value_tests::both
        value()
    {
        return pnp_tests::placeholder_value_tests::both{ 2 };
    }
};

template <>
struct placeholder_value<pnp_tests::placeholder_value_tests::wrong_return_t>
{
    static constexpr int
        value()
    {
        return 0;
    }
};

template <>
struct placeholder_value<pnp_tests::placeholder_value_tests::not_static>
{
    constexpr pnp_tests::placeholder_value_tests::not_static
        value() const
    {
        return pnp_tests::placeholder_value_tests::not_static(9);
    }
};

// Constrained on its part: without the requires-clause, box<neither> would
// claim a placeholder it can't make.
template <typename T>
    requires parse_constructible_c<T>
struct placeholder_value<pnp_tests::placeholder_value_tests::box<T>>
{
    static constexpr pnp_tests::placeholder_value_tests::box<T>
        value()
    {
        return pnp_tests::placeholder_value_tests::box<T>{make_placeholder<T>()};
    }
};
PNP_NS_END

using namespace pnp_tests::placeholder_value_tests;

TEST_CASE(
    "placeholder_value: which types have a placeholder",
    "[placeholder_value]"
)
{
    STATIC_CHECK(pnp::has_placeholder_value_c<no_default>);
    STATIC_CHECK(pnp::has_placeholder_value_c<both>);
    STATIC_CHECK(pnp::has_placeholder_value_c<box<int>>);
    STATIC_CHECK(pnp::has_placeholder_value_c<box<no_default>>);

    // No specialisation.
    STATIC_CHECK(not pnp::has_placeholder_value_c<plain>);
    STATIC_CHECK(not pnp::has_placeholder_value_c<neither>);
    STATIC_CHECK(not pnp::has_placeholder_value_c<int>);
    // A specialisation that doesn't count.
    STATIC_CHECK(not pnp::has_placeholder_value_c<wrong_return_t>);
    STATIC_CHECK(not pnp::has_placeholder_value_c<not_static>);
    // A generic specialisation whose part can't be made.
    STATIC_CHECK(not pnp::has_placeholder_value_c<box<neither>>);
}

TEST_CASE(
    "placeholder_value: which types pnp can make",
    "[placeholder_value]"
)
{
    // A placeholder, a default constructor, or both.
    STATIC_CHECK(pnp::parse_constructible_c<no_default>);
    STATIC_CHECK(pnp::parse_constructible_c<both>);
    STATIC_CHECK(pnp::parse_constructible_c<plain>);
    STATIC_CHECK(pnp::parse_constructible_c<int>);
    STATIC_CHECK(pnp::parse_constructible_c<std::string>);
    STATIC_CHECK(pnp::parse_constructible_c<box<int>>);
    STATIC_CHECK(pnp::parse_constructible_c<box<no_default>>);

    // Neither.
    STATIC_CHECK(not pnp::parse_constructible_c<neither>);
    STATIC_CHECK(not pnp::parse_constructible_c<wrong_return_t>);
    STATIC_CHECK(not pnp::parse_constructible_c<not_static>);
    STATIC_CHECK(not pnp::parse_constructible_c<box<neither>>);
}

TEST_CASE(
    "make_placeholder: the placeholder, or a value-initialised T",
    "[placeholder_value]"
)
{
    // The placeholder, where there's no default constructor.
    STATIC_CHECK(pnp::make_placeholder<no_default>().n == 7);
    // The placeholder wins over a default constructor.
    STATIC_CHECK(pnp::make_placeholder<both>().n == 2);
    // The default constructor, where there's no placeholder.
    STATIC_CHECK(pnp::make_placeholder<plain>().n == 3);

    // Value-initialised: zero, not whatever was in memory.
    STATIC_CHECK(pnp::make_placeholder<int>() == 0);
    STATIC_CHECK(pnp::make_placeholder<double>() == 0.0);
    STATIC_CHECK(pnp::make_placeholder<int*>() == nullptr);
    CHECK(pnp::make_placeholder<std::string>().empty());

    // A generic placeholder uses its part's: the placeholder, or the default.
    STATIC_CHECK(pnp::make_placeholder<box<no_default>>().inner.n == 7);
    STATIC_CHECK(pnp::make_placeholder<box<both>>().inner.n == 2);
    STATIC_CHECK(pnp::make_placeholder<box<int>>().inner == 0);
}

// Whether make_placeholder<T>() can be called: false when its constraints
// reject T.
template <typename T>
concept can_make_placeholder_c = requires { pnp::make_placeholder<T>(); };

TEST_CASE(
    "placeholder_value: C arrays",
    "[placeholder_value]"
)
{
    // An array can be made if its elements can, in any number of dimensions.
    STATIC_CHECK(pnp::parse_constructible_c<int[3]>);
    STATIC_CHECK(pnp::parse_constructible_c<int[2][3]>);
    STATIC_CHECK(pnp::parse_constructible_c<no_default[3]>);
    STATIC_CHECK(pnp::parse_constructible_c<both[2][2]>);
    STATIC_CHECK(not pnp::parse_constructible_c<neither[3]>);
    STATIC_CHECK(not pnp::parse_constructible_c<neither[2][2]>);
    // An array of unknown bound has no size to make, whatever its elements.
    STATIC_CHECK(not pnp::parse_constructible_c<int[]>);
    STATIC_CHECK(not pnp::parse_constructible_c<no_default[]>);

    // An array never has a placeholder of its own: only its elements do.
    STATIC_CHECK(not pnp::has_placeholder_value_c<no_default[3]>);

    // make_placeholder can't return an array; parse_outcome_with_value builds
    // those in place instead.
    STATIC_CHECK(can_make_placeholder_c<int>);
    STATIC_CHECK(can_make_placeholder_c<no_default>);
    STATIC_CHECK(not can_make_placeholder_c<int[3]>);
    STATIC_CHECK(not can_make_placeholder_c<no_default[3]>);
    STATIC_CHECK(not can_make_placeholder_c<neither>);
}