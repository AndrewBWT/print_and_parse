#pragma once
#include "pnp/support/user_types/objects.hpp"
#include "pnp/sinks/string.hpp"
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <catch2/catch_test_macros.hpp>

#include <concepts>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// ============================================================ types used by
// the tests
//
// Their names and fields are given by object_traits specialisations, further
// down.

namespace objects_tests
{
    // A class with private members, a constructor and getters: the usual case.
    // It has no placeholder, so it's parsed into an existing value rather than
    // with try_parse.
    struct rectangle
    {
    public:
        constexpr rectangle(
            const int width_arg,
            const int height_arg
        ) noexcept
            : _width(width_arg)
            , _height(height_arg)
        {
        }

        [[nodiscard]]
        constexpr int
            width() const noexcept
        {
            return _width;
        }

        [[nodiscard]]
        constexpr int
            height() const noexcept
        {
            return _height;
        }

        friend constexpr bool
            operator==(const rectangle&, const rectangle&)
            = default;
    private:
        int _width;
        int _height;
    };

    // An aggregate, giving references to its members with std::tie, and built
    // back by aggregate initialisation.
    struct point
    {
        int x = 0;
        int y = 0;

        friend constexpr bool
            operator==(const point&, const point&)
            = default;
    };

    // An object whose fields are objects.
    struct line
    {
        point start;
        point end;

        friend constexpr bool
            operator==(const line&, const line&)
            = default;
    };

    // A class template, made an object by one partial specialisation, with a
    // name built as a std::u8string.
    template <typename T>
    struct box
    {
        T value{};

        friend constexpr bool
            operator==(const box&, const box&)
            = default;
    };

    // Holds a reference: printed as what it refers to, but can't be parsed,
    // as there's nothing for a parsed reference to refer to.
    struct holds_reference
    {
        std::reference_wrapper<const int> value;
    };

    // Prints a derived field, so no constructor takes its fields: printable,
    // not parseable.
    struct doubled
    {
    public:
        constexpr explicit doubled(
            const int value_arg
        ) noexcept
            : _value(value_arg)
        {
        }

        [[nodiscard]]
        constexpr int
            value() const noexcept
        {
            return _value;
        }
    private:
        int _value;
    };

    // A const member, so it can be built but not assigned: printable, not
    // parseable.
    struct fixed_id
    {
        const int id;
    };

    // A type with a default printer, written below, but no default parser.
    struct print_only_field
    {
    };

    // An object with a print_only_field: printable, not parseable.
    struct holds_print_only
    {
        print_only_field field;
    };

    // A type with no pnp at all.
    struct opaque
    {
    };

    // An object with an opaque field: neither printable nor parseable, but
    // still an object.
    struct holds_opaque
    {
        opaque field;
    };

    // No object_traits specialisation: an object only with traits of its own.
    struct size
    {
        int width = 0;
        int height = 0;

        friend constexpr bool
            operator==(const size&, const size&)
            = default;
    };

    // Traits for a size, with non-static members.
    struct size_traits
    {
        [[nodiscard]]
        constexpr std::u8string_view
            consistent_name() const noexcept
        {
            return u8"size";
        }

        [[nodiscard]]
        constexpr std::tuple<int, int>
            to_tuple(
                const size& size_arg
            ) const noexcept
        {
            return { size_arg.width, size_arg.height };
        }
    };

    // Traits for a size, adding offset to each field: traits with state.
    struct offset_size_traits
    {
        int offset = 0;

        [[nodiscard]]
        constexpr std::u8string_view
            consistent_name() const noexcept
        {
            return u8"size";
        }

        [[nodiscard]]
        constexpr std::tuple<int, int>
            to_tuple(
                const size& size_arg
            ) const noexcept
        {
            return { size_arg.width + offset, size_arg.height + offset };
        }
    };

    // An object that hasn't opted in to the defaults.
    struct not_opted_in
    {
        int value = 0;
    };

    // Traits with to_tuple, but no name: not an object.
    struct no_name
    {
        int value = 0;
    };

    // Traits whose name isn't text: not an object.
    struct bad_name
    {
        int value = 0;
    };

    // Traits whose to_tuple doesn't return a std::tuple: not an object.
    struct not_a_tuple
    {
        int first = 0;
        int second = 0;
    };

    // A tuple printer of its own, writing <a|b>, to configure an object_pnp with.
    struct angle_tuple_printer
    {
        void
            run_printer(
                const std::tuple<int, int>& tuple_arg,
                pnp::text_sink& sink_arg
            ) const
        {
            sink_arg.write(u8"<");
            sink_arg.write(pnp::to_u8string(std::get<0>(tuple_arg)));
            sink_arg.write(u8"|");
            sink_arg.write(pnp::to_u8string(std::get<1>(tuple_arg)));
            sink_arg.write(u8">");
        }
    };

    // Whether object_pnp<T> names a type: false when T isn't an object.
    template <typename T>
    concept object_pnp_names_a_type = requires {
        typename pnp::object_pnp<T>;
    };

    // What an object of name_arg with these fields should print as: its name,
    // then the fields printed exactly as a tuple.
    template <typename... Ts>
    std::u8string
        expected_text(
            const std::u8string_view name_arg,
            const Ts&... fields_arg
        )
    {
        return std::u8string(name_arg)
            + pnp::to_u8string(std::tuple{ fields_arg... });
    }

    // Prints value_arg with printer_arg, into a string.
    template <typename Printer, typename T>
    std::u8string
        print_with(
            const Printer& printer_arg,
            const T& value_arg
        )
    {
        std::u8string    text;
        pnp::string_sink sink(text);
        pnp::print(printer_arg, value_arg, sink);
        return text;
    }
} // namespace objects_tests

// ============================================================ object_traits
// specialisations

template <>
struct PNP_NAMESPACE::object_traits<objects_tests::rectangle>
{
    static constexpr std::u8string_view
        consistent_name() noexcept
    {
        return u8"rectangle";
    }

    static constexpr std::tuple<int, int>
        to_tuple(
            const objects_tests::rectangle& rectangle_arg
        ) noexcept
    {
        return { rectangle_arg.width(), rectangle_arg.height() };
    }
};

template <>
struct PNP_NAMESPACE::object_traits<objects_tests::point>
{
    static constexpr std::u8string_view
        consistent_name() noexcept
    {
        return u8"point";
    }

    static constexpr auto
        to_tuple(
            const objects_tests::point& point_arg
        ) noexcept
    {
        return std::tie(point_arg.x, point_arg.y);
    }
};

template <>
struct PNP_NAMESPACE::object_traits<objects_tests::line>
{
    static constexpr std::u8string_view
        consistent_name() noexcept
    {
        return u8"line";
    }

    static constexpr auto
        to_tuple(
            const objects_tests::line& line_arg
        ) noexcept
    {
        return std::tie(line_arg.start, line_arg.end);
    }
};

// Every box<T>.
template <typename T>
struct PNP_NAMESPACE::object_traits<objects_tests::box<T>>
{
    static std::u8string
        consistent_name()
    {
        return std::u8string(u8"box");
    }

    static constexpr auto
        to_tuple(
            const objects_tests::box<T>& box_arg
        ) noexcept
    {
        return std::tie(box_arg.value);
    }
};

template <>
struct PNP_NAMESPACE::object_traits<objects_tests::holds_reference>
{
    static constexpr std::u8string_view
        consistent_name() noexcept
    {
        return u8"holds_reference";
    }

    static constexpr auto
        to_tuple(
            const objects_tests::holds_reference& object_arg
        ) noexcept
    {
        return std::tie(object_arg.value);
    }
};

template <>
struct PNP_NAMESPACE::object_traits<objects_tests::doubled>
{
    static constexpr std::u8string_view
        consistent_name() noexcept
    {
        return u8"doubled";
    }

    static constexpr std::tuple<int, int>
        to_tuple(
            const objects_tests::doubled& doubled_arg
        ) noexcept
    {
        return { doubled_arg.value(), doubled_arg.value() * 2 };
    }
};

template <>
struct PNP_NAMESPACE::object_traits<objects_tests::fixed_id>
{
    static constexpr std::u8string_view
        consistent_name() noexcept
    {
        return u8"fixed_id";
    }

    static constexpr auto
        to_tuple(
            const objects_tests::fixed_id& fixed_id_arg
        ) noexcept
    {
        return std::tie(fixed_id_arg.id);
    }
};

template <>
struct PNP_NAMESPACE::object_traits<objects_tests::holds_print_only>
{
    static constexpr std::u8string_view
        consistent_name() noexcept
    {
        return u8"holds_print_only";
    }

    static constexpr auto
        to_tuple(
            const objects_tests::holds_print_only& object_arg
        ) noexcept
    {
        return std::tie(object_arg.field);
    }
};

template <>
struct PNP_NAMESPACE::object_traits<objects_tests::holds_opaque>
{
    static constexpr std::u8string_view
        consistent_name() noexcept
    {
        return u8"holds_opaque";
    }

    static constexpr auto
        to_tuple(
            const objects_tests::holds_opaque& object_arg
        ) noexcept
    {
        return std::tie(object_arg.field);
    }
};

template <>
struct PNP_NAMESPACE::object_traits<objects_tests::not_opted_in>
{
    static constexpr std::u8string_view
        consistent_name() noexcept
    {
        return u8"not_opted_in";
    }

    static constexpr std::tuple<int>
        to_tuple(
            const objects_tests::not_opted_in& object_arg
        ) noexcept
    {
        return { object_arg.value };
    }
};

template <>
struct PNP_NAMESPACE::object_traits<objects_tests::no_name>
{
    static constexpr std::tuple<int>
        to_tuple(
            const objects_tests::no_name& object_arg
        ) noexcept
    {
        return { object_arg.value };
    }
};

template <>
struct PNP_NAMESPACE::object_traits<objects_tests::bad_name>
{
    static constexpr int
        consistent_name() noexcept
    {
        return 0;
    }

    static constexpr std::tuple<int>
        to_tuple(
            const objects_tests::bad_name& object_arg
        ) noexcept
    {
        return { object_arg.value };
    }
};

template <>
struct PNP_NAMESPACE::object_traits<objects_tests::not_a_tuple>
{
    static constexpr std::u8string_view
        consistent_name() noexcept
    {
        return u8"not_a_tuple";
    }

    static constexpr std::pair<int, int>
        to_tuple(
            const objects_tests::not_a_tuple& object_arg
        ) noexcept
    {
        return { object_arg.first, object_arg.second };
    }
};

// print_only_field's printer. It has no parser.
template <>
struct PNP_NAMESPACE::default_printer<objects_tests::print_only_field>
{
    static constexpr bool is_specialized = true;

    void
        run_printer(
            const objects_tests::print_only_field&,
            pnp::text_sink& sink_arg
        ) const
    {
        sink_arg.write(u8"field");
    }
};

// ============================================================ the opt-ins
//
// Not for not_opted_in, no_name, bad_name or not_a_tuple.

template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_printer<objects_tests::rectangle>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_parser<objects_tests::rectangle>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_printer<objects_tests::point>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_parser<objects_tests::point>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_printer<objects_tests::line>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_parser<objects_tests::line>
= true;
// Every box<T>.
template <typename T>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_printer<objects_tests::box<T>>
= true;
template <typename T>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_parser<objects_tests::box<T>>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_printer<objects_tests::holds_reference>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_printer<objects_tests::doubled>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_printer<objects_tests::fixed_id>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_printer<objects_tests::holds_print_only>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_printer<objects_tests::holds_opaque>
= true;
// Opted in, but with no object_traits specialisation, so not an object.
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_printer<objects_tests::size>
= true;

namespace objects_tests
{
    // A pnp of its own, built on object_pnp, adding a comment after the object.
    // After the specialisations, as naming object_pnp<rectangle> needs
    // object_traits<rectangle>.
    struct commented_rectangle_pnp : public pnp::object_pnp<rectangle>
    {
        void
            run_printer(
                const rectangle& rectangle_arg,
                pnp::text_sink& sink_arg
            ) const
        {
            pnp::object_pnp<rectangle>::run_printer(rectangle_arg, sink_arg);
            sink_arg.write(u8" // area ");
            sink_arg.write(
                pnp::to_u8string(rectangle_arg.width() * rectangle_arg.height())
            );
        }
    };

    // ============================================================ static
    // properties: what an object is

    // A specialisation with a name and a tuple makes an object.
    static_assert(pnp::has_object_traits_c<rectangle>);
    static_assert(pnp::has_object_traits_c<point>);
    static_assert(pnp::has_object_traits_c<not_opted_in>);
    static_assert(pnp::has_object_traits_c<box<int>>);
    static_assert(pnp::has_object_traits_c<box<point>>);

    // Without a name, with a name that isn't text, or with to_tuple not
    // returning a std::tuple, it doesn't.
    static_assert(not pnp::has_object_traits_c<no_name>);
    static_assert(not pnp::has_object_traits_c<bad_name>);
    static_assert(not pnp::has_object_traits_c<not_a_tuple>);

    // Nor does anything without a specialisation.
    static_assert(not pnp::has_object_traits_c<int>);
    static_assert(not pnp::has_object_traits_c<size>);

    // But traits of its own can make it one, with or without state.
    static_assert(pnp::has_object_traits_c<size, size_traits>);
    static_assert(pnp::has_object_traits_c<size, offset_size_traits>);

    // Being an object doesn't depend on whether its fields can be printed or
    // parsed.
    static_assert(pnp::has_object_traits_c<holds_opaque>);
    static_assert(pnp::has_object_traits_c<holds_print_only>);
    static_assert(pnp::has_object_traits_c<holds_reference>);

    // ============================================================ static
    // properties: the values

    // References and const are removed.
    static_assert(
        std::same_as<pnp::object_values_t<point>, std::tuple<int, int>>
        );
    static_assert(
        std::same_as<pnp::object_values_t<rectangle>, std::tuple<int, int>>
        );
    static_assert(std::same_as<pnp::object_values_t<fixed_id>, std::tuple<int>>);
    static_assert(
        std::same_as<pnp::object_values_t<box<point>>, std::tuple<point>>
        );
    static_assert(
        std::same_as<pnp::object_values_t<size, size_traits>, std::tuple<int, int>>
        );

    // A std::reference_wrapper is a value like any other, not unwrapped.
    static_assert(std::same_as<
        pnp::object_values_t<holds_reference>,
        std::tuple<std::reference_wrapper<const int>>>);

    // void for anything that isn't an object, rather than an error.
    static_assert(std::is_void_v<pnp::object_values_t<int>>);
    static_assert(std::is_void_v<pnp::object_values_t<not_a_tuple>>);
    static_assert(std::is_void_v<pnp::object_values_t<no_name>>);
    static_assert(std::is_void_v<pnp::object_values_t<size>>);

    // So object_pnp is a plain constraint failure for a non-object.
    static_assert(object_pnp_names_a_type<point>);
    static_assert(not object_pnp_names_a_type<int>);
    static_assert(not object_pnp_names_a_type<not_a_tuple>);
    static_assert(not object_pnp_names_a_type<size>);

    // ============================================================ static
    // properties: object_pnp

    // Configured by aggregate initialisation.
    static_assert(std::is_aggregate_v<pnp::object_pnp<rectangle>>);

    // A printer and a parser, whether or not T opted in.
    static_assert(pnp::printer_c<pnp::object_pnp<not_opted_in>, not_opted_in>);
    static_assert(pnp::parser_c<pnp::object_pnp<not_opted_in>, not_opted_in>);

    // With traits of its own.
    static_assert(pnp::printer_c<pnp::object_pnp<size, size_traits>, size>);
    static_assert(pnp::parser_c<pnp::object_pnp<size, size_traits>, size>);

    // A class template.
    static_assert(pnp::printer_c<pnp::object_pnp<box<int>>, box<int>>);
    static_assert(pnp::parser_c<pnp::object_pnp<box<int>>, box<int>>);

    // Printable but not parseable: a reference has nothing to refer to...
    static_assert(
        pnp::printer_c<pnp::object_pnp<holds_reference>, holds_reference>
        );
    static_assert(
        not pnp::parser_c<pnp::object_pnp<holds_reference>, holds_reference>
        );
    // ...no constructor takes the fields...
    static_assert(pnp::printer_c<pnp::object_pnp<doubled>, doubled>);
    static_assert(not pnp::parser_c<pnp::object_pnp<doubled>, doubled>);
    // ...it can't be assigned...
    static_assert(pnp::printer_c<pnp::object_pnp<fixed_id>, fixed_id>);
    static_assert(not pnp::parser_c<pnp::object_pnp<fixed_id>, fixed_id>);
    // ...or a field has no parser.
    static_assert(
        pnp::printer_c<pnp::object_pnp<holds_print_only>, holds_print_only>
        );
    static_assert(
        not pnp::parser_c<pnp::object_pnp<holds_print_only>, holds_print_only>
        );

    // A field with no pnp: neither, but object_pnp itself still exists, and can
    // be constructed.
    static_assert(not pnp::printer_c<pnp::object_pnp<holds_opaque>, holds_opaque>);
    static_assert(not pnp::parser_c<pnp::object_pnp<holds_opaque>, holds_opaque>);
    static_assert(std::is_default_constructible_v<pnp::object_pnp<holds_opaque>>);

    // A tuple printer of its own makes no difference to parsing.
    static_assert(pnp::parser_c<
        pnp::object_pnp<
        rectangle,
        pnp::object_traits<rectangle>,
        angle_tuple_printer>,
        rectangle>);

    // ============================================================ static
    // properties: the defaults

    // Only for types that opted in.
    static_assert(pnp::default_printable_c<rectangle>);
    static_assert(pnp::default_parseable_c<rectangle>);
    static_assert(pnp::default_printer<rectangle>::is_specialized);
    static_assert(pnp::default_parser<rectangle>::is_specialized);
    static_assert(not pnp::default_printable_c<not_opted_in>);
    static_assert(not pnp::default_parseable_c<not_opted_in>);

    // A partially specialised opt-in covers every box<T>.
    static_assert(pnp::default_printable_c<box<int>>);
    static_assert(pnp::default_parseable_c<box<int>>);
    static_assert(pnp::default_printable_c<box<point>>);
    static_assert(pnp::default_parseable_c<box<point>>);

    // Each direction exists only if it works.
    static_assert(pnp::default_printable_c<holds_reference>);
    static_assert(not pnp::default_parseable_c<holds_reference>);
    static_assert(pnp::default_printable_c<doubled>);
    static_assert(not pnp::default_parseable_c<doubled>);
    static_assert(pnp::default_printable_c<fixed_id>);
    static_assert(not pnp::default_parseable_c<fixed_id>);
    static_assert(pnp::default_printable_c<holds_print_only>);
    static_assert(not pnp::default_parseable_c<holds_print_only>);
    static_assert(not pnp::default_printable_c<holds_opaque>);
    static_assert(not pnp::default_parseable_c<holds_opaque>);

    // Opting in doesn't help without an object_traits specialisation.
    static_assert(not pnp::default_printable_c<size>);
    static_assert(not pnp::default_parseable_c<size>);

    // And containers follow. These depend on the container pnps requiring
    // their elements to be default printable or parseable.
    static_assert(pnp::default_printable_c<std::vector<point>>);
    static_assert(pnp::default_parseable_c<std::vector<point>>);
    static_assert(pnp::default_printable_c<std::vector<holds_print_only>>);
    static_assert(not pnp::default_parseable_c<std::vector<holds_print_only>>);
    static_assert(not pnp::default_printable_c<std::vector<holds_opaque>>);
    static_assert(not pnp::default_parseable_c<std::vector<holds_opaque>>);

    // Without a placeholder, try_parse isn't available; parse is.
    static_assert(not pnp::parse_constructible_c<rectangle>);
    static_assert(pnp::parse_constructible_c<point>);

    // ============================================================ printing

    TEST_CASE(
        "objects: printed as their name, then their fields as a tuple",
        "[objects][print]"
    )
    {
        SECTION("fields from getters")
        {
            CHECK(
                pnp::to_u8string(rectangle(3, 4))
                == expected_text(u8"rectangle", 3, 4)
            );
        }
        SECTION("fields from std::tie")
        {
            CHECK(
                pnp::to_u8string(point{ .x = -1, .y = 7 })
                == expected_text(u8"point", -1, 7)
            );
        }
        SECTION("fields that are objects")
        {
            const line value{ .start = { 1, 2 }, .end = { 3, 4 } };
            CHECK(
                pnp::to_u8string(value)
                == u8"line"
                + pnp::to_u8string(std::tuple{ point{ 1, 2 }, point{ 3, 4 } })
            );
            // And each field is written as an object.
            CHECK(
                pnp::to_u8string(value).find(pnp::to_u8string(point{ 1, 2 }))
                != std::u8string::npos
            );
        }
        SECTION("a class template, with a std::u8string name")
        {
            CHECK(
                pnp::to_u8string(box<int>{.value = 5 })
                == expected_text(u8"box", 5)
            );
            CHECK(
                pnp::to_u8string(box<point>{.value = { 1, 2 } })
                == expected_text(u8"box", point{ 1, 2 })
            );
        }
        SECTION("in a container")
        {
            const std::vector<point> points{
                { 1, 2 },
                { 3, 4 }
            };
            const std::u8string text = pnp::to_u8string(points);
            CHECK(
                text.find(pnp::to_u8string(point{ 1, 2 })) != std::u8string::npos
            );
            CHECK(
                text.find(pnp::to_u8string(point{ 3, 4 })) != std::u8string::npos
            );
        }
        SECTION("an object that hasn't opted in, through object_pnp directly")
        {
            CHECK(
                print_with(pnp::object_pnp<not_opted_in>{}, not_opted_in{ 5 })
                == expected_text(u8"not_opted_in", 5)
            );
        }
    }

    TEST_CASE(
        "objects: objects that can't be parsed still print",
        "[objects][print]"
    )
    {
        SECTION("a reference")
        {
            const int value = 42;
            CHECK(
                pnp::to_u8string(holds_reference{ .value = std::cref(value) })
                == expected_text(u8"holds_reference", 42)
            );
        }
        SECTION("no constructor takes the fields")
        {
            CHECK(pnp::to_u8string(doubled(3)) == expected_text(u8"doubled", 3, 6));
        }
        SECTION("a const member")
        {
            CHECK(
                pnp::to_u8string(fixed_id{ .id = 7 })
                == expected_text(u8"fixed_id", 7)
            );
        }
        SECTION("a field with a printer but no parser")
        {
            CHECK(
                pnp::to_u8string(holds_print_only{})
                == expected_text(u8"holds_print_only", print_only_field{})
            );
        }
    }

    // ============================================================ parsing

    TEST_CASE(
        "objects: parse back what was printed",
        "[objects][parse]"
    )
    {
        SECTION("a class built with its constructor")
        {
            // No placeholder for rectangle, so parse into an existing value.
            const rectangle original(3, 4);
            rectangle       value(0, 0);
            const auto outcome = pnp::parse(pnp::to_u8string(original), value);
            REQUIRE_FALSE(outcome.failed());
            CHECK(value == original);
        }
        SECTION("an aggregate")
        {
            const point original{ .x = -1, .y = 7 };
            const auto  result = pnp::try_parse<point>(pnp::to_u8string(original));
            REQUIRE_FALSE(result.outcome.failed());
            CHECK(result.value == original);
        }
        SECTION("an object whose fields are objects")
        {
            const line original{ .start = { 1, 2 }, .end = { 3, 4 } };
            const auto result = pnp::try_parse<line>(pnp::to_u8string(original));
            REQUIRE_FALSE(result.outcome.failed());
            CHECK(result.value == original);
        }
        SECTION("a class template")
        {
            const box<point> original{ .value = { 5, 6 } };
            const auto result
                = pnp::try_parse<box<point>>(pnp::to_u8string(original));
            REQUIRE_FALSE(result.outcome.failed());
            CHECK(result.value == original);
        }
        SECTION("objects in a container")
        {
            const std::vector<point> original{
                { 1, 2 },
                { 3, 4 },
                { 5, 6 }
            };
            const auto result
                = pnp::try_parse<std::vector<point>>(pnp::to_u8string(original));
            REQUIRE_FALSE(result.outcome.failed());
            CHECK(result.value == original);
        }
    }

    TEST_CASE(
        "objects: whitespace around the name and fields is allowed",
        "[objects][parse]"
    )
    {
        const std::u8string fields = pnp::to_u8string(std::tuple{ 3, 4 });
        const std::vector<std::u8string> texts{
            u8"rectangle" + fields,
            u8"rectangle " + fields,
            u8"  rectangle" + fields + u8"  ",
        };
        for (const std::u8string& text : texts)
        {
            CAPTURE(std::string(text.begin(), text.end()));
            rectangle  value(0, 0);
            const auto outcome = pnp::parse(text, value);
            REQUIRE_FALSE(outcome.failed());
            CHECK(value == rectangle(3, 4));
        }
    }

    TEST_CASE(
        "objects: wrong text fails, and leaves the value alone",
        "[objects][parse][errors]"
    )
    {
        const std::u8string two_fields = pnp::to_u8string(std::tuple{ 3, 4 });
        const std::u8string one_field = pnp::to_u8string(std::tuple{ 3 });
        const std::vector<std::u8string> texts{
            u8"square" + two_fields,   // the wrong name
            u8"rect" + two_fields,     // part of the name
            two_fields,                // no name
            u8"rectangle",             // no fields
            u8"rectangle" + one_field, // too few fields
            u8"rectangle(x, y)",       // fields of the wrong type
        };
        for (const std::u8string& text : texts)
        {
            CAPTURE(std::string(text.begin(), text.end()));
            rectangle  value(9, 9);
            const auto outcome = pnp::parse(text, value);
            CHECK(outcome.failed());
            CHECK(value == rectangle(9, 9));
        }
    }

    TEST_CASE(
        "objects: a failure inside a nested object is reported where it happened",
        "[objects][parse][errors]"
    )
    {
        // Printed by pnp, then the second point's last field made invalid, so
        // the text matches pnp's own separators and spacing.
        std::u8string text
            = pnp::to_u8string(line{ .start = { 1, 2 }, .end = { 3, 4 } });
        const std::size_t bad_point = text.rfind(u8"point");
        const std::size_t bad_field = text.rfind(u8'4');
        REQUIRE(bad_point != std::u8string::npos);
        REQUIRE(bad_field > bad_point);
        text[bad_field] = u8'x';

        line       value{ .start = { 9, 9 }, .end = { 9, 9 } };
        const auto outcome = pnp::parse(text, value);
        REQUIRE(outcome.failed());
        // Inside the second point, not at the start of the line.
        CHECK(outcome.position >= bad_point);
        CHECK(value == line{ .start = { 9, 9 }, .end = { 9, 9 } });
    }

    // ============================================================ configuring

    TEST_CASE(
        "objects: traits of its own, for a type without an object_traits "
        "specialisation",
        "[objects][configure]"
    )
    {
        using size_pnp = pnp::object_pnp<size, size_traits>;
        const size original{ .width = 3, .height = 4 };
        SECTION("prints")
        {
            CHECK(print_with(size_pnp{}, original) == expected_text(u8"size", 3, 4));
        }
        SECTION("parses back what it printed")
        {
            const auto result
                = pnp::try_parse<size, size_pnp>(print_with(size_pnp{}, original));
            REQUIRE_FALSE(result.outcome.failed());
            CHECK(result.value == original);
        }
    }

    TEST_CASE(
        "objects: traits with state, set by aggregate initialisation",
        "[objects][configure]"
    )
    {
        const pnp::object_pnp<size, offset_size_traits> printer{
            .traits = {.offset = 10 }
        };
        CHECK(
            print_with(printer, size{ .width = 3, .height = 4 })
            == expected_text(u8"size", 13, 14)
        );
    }

    TEST_CASE(
        "objects: a tuple printer of its own, set by aggregate initialisation",
        "[objects][configure]"
    )
    {
        using angle_pnp = pnp::object_pnp<
            rectangle,
            pnp::object_traits<rectangle>,
            angle_tuple_printer>;
        SECTION("default constructed")
        {
            CHECK(print_with(angle_pnp{}, rectangle(3, 4)) == u8"rectangle<3|4>");
        }
        SECTION("by name, leaving the others as defaults")
        {
            const angle_pnp printer{ .tuple_printer = angle_tuple_printer{} };
            CHECK(print_with(printer, rectangle(3, 4)) == u8"rectangle<3|4>");
        }
    }

    TEST_CASE(
        "objects: a pnp of its own, built on object_pnp",
        "[objects][configure]"
    )
    {
        CHECK(
            print_with(commented_rectangle_pnp{}, rectangle(3, 4))
            == expected_text(u8"rectangle", 3, 4) + u8" // area 12"
        );
        // Its parser is object_pnp's, unchanged.
        rectangle  value(0, 0);
        const auto outcome = pnp::parse(
            pnp::to_u8string(rectangle(5, 6)), value, commented_rectangle_pnp{}
        );
        REQUIRE_FALSE(outcome.failed());
        CHECK(value == rectangle(5, 6));
    }

    // ============================================================ split text

    TEST_CASE(
        "objects: parse the same however the text is split",
        "[objects][parse][check_split]"
    )
    {
        // check_parser_using_split_text needs a placeholder, so these use the
        // aggregates.
        const pnp::testing::check_parser_using_split_text_config config{
            .exact_splits_to_run = { 1, 2, 3, 7 }
        };
        SECTION("point")
        {
            const auto report = pnp::testing::check_parser_using_split_text<point>(
                config, pnp::to_u8string(point{ .x = 12, .y = -345 })
            );
            INFO(report.value_or(""));
            REQUIRE_FALSE(report.has_value());
        }
        SECTION("line")
        {
            const auto report = pnp::testing::check_parser_using_split_text<line>(
                config,
                pnp::to_u8string(line{ .start = { 1, 22 }, .end = { 333, 4 } })
            );
            INFO(report.value_or(""));
            REQUIRE_FALSE(report.has_value());
        }
        SECTION("a class template")
        {
            const auto report
                = pnp::testing::check_parser_using_split_text<box<point>>(
                    config, pnp::to_u8string(box<point>{.value = { 98, -765 } })
                );
            INFO(report.value_or(""));
            REQUIRE_FALSE(report.has_value());
        }
        SECTION("size, with traits of its own")
        {
            using size_pnp = pnp::object_pnp<size, size_traits>;
            const auto report
                = pnp::testing::check_parser_using_split_text<size, size_pnp>(
                    config,
                    print_with(size_pnp{}, size{ .width = 12, .height = 345 })
                );
            INFO(report.value_or(""));
            REQUIRE_FALSE(report.has_value());
        }
        SECTION("a wrong name, failing the same way every time")
        {
            const auto report = pnp::testing::check_parser_using_split_text<point>(
                config, u8"poin" + pnp::to_u8string(std::tuple{ 1, 2 })
            );
            INFO(report.value_or(""));
            REQUIRE_FALSE(report.has_value());
        }
    }
} // namespace objects_tests