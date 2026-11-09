#pragma once
#include "pnp/support/user_types/objects.hpp"
#include "pnp/sinks/string.hpp"
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

// ============================================================ types used by
// the tests
//
// The free functions are found by argument-dependent lookup, so they live
// beside the types.

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

    constexpr std::u8string_view
        pnp_name(
            std::type_identity<rectangle>
        )
    {
        return u8"rectangle";
    }

    inline std::tuple<int, int>
        pnp_to_tuple(
            const rectangle& rectangle_arg
        )
    {
        return { rectangle_arg.width(), rectangle_arg.height() };
    }

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

    constexpr std::u8string_view
        pnp_name(
            std::type_identity<point>
        )
    {
        return u8"point";
    }

    inline auto
        pnp_to_tuple(
            const point& point_arg
        )
    {
        return std::tie(point_arg.x, point_arg.y);
    }

    // An object whose fields are objects.
    struct line
    {
        point start;
        point end;

        friend constexpr bool
            operator==(const line&, const line&)
            = default;
    };

    constexpr std::u8string_view
        pnp_name(
            std::type_identity<line>
        )
    {
        return u8"line";
    }

    inline auto
        pnp_to_tuple(
            const line& line_arg
        )
    {
        return std::tie(line_arg.start, line_arg.end);
    }

    // A constructor taking its fields in a different order from how they're
    // written, so it needs pnp_from_tuple.
    struct backwards
    {
    public:
        constexpr backwards(
            const int second_arg,
            const int first_arg
        ) noexcept
            : _first(first_arg)
            , _second(second_arg)
        {
        }

        [[nodiscard]]
        constexpr int
            first() const noexcept
        {
            return _first;
        }

        [[nodiscard]]
        constexpr int
            second() const noexcept
        {
            return _second;
        }

        friend constexpr bool
            operator==(const backwards&, const backwards&)
            = default;
    private:
        int _first;
        int _second;
    };

    constexpr std::u8string_view
        pnp_name(
            std::type_identity<backwards>
        )
    {
        return u8"backwards";
    }

    inline std::tuple<int, int>
        pnp_to_tuple(
            const backwards& backwards_arg
        )
    {
        return { backwards_arg.first(), backwards_arg.second() };
    }

    inline backwards
        pnp_from_tuple(
            std::type_identity<backwards>,
            std::tuple<int, int>&& values_arg
        )
    {
        return backwards(std::get<1>(values_arg), std::get<0>(values_arg));
    }

    // An object that hasn't opted in to the defaults.
    struct not_opted_in
    {
        int value = 0;
    };

    constexpr std::u8string_view
        pnp_name(
            std::type_identity<not_opted_in>
        )
    {
        return u8"not_opted_in";
    }

    inline std::tuple<int>
        pnp_to_tuple(
            const not_opted_in& object_arg
        )
    {
        return { object_arg.value };
    }

    // Has pnp_to_tuple, but no name: not an object.
    struct no_name
    {
        int value = 0;
    };

    inline std::tuple<int>
        pnp_to_tuple(
            const no_name& object_arg
        )
    {
        return { object_arg.value };
    }

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

    // A pnp of its own, built on object_pnp, adding a comment after the object.
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

// The opt-ins. Not for not_opted_in or no_name.
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_pnp<objects_tests::rectangle>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_pnp<objects_tests::point>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_pnp<objects_tests::line>
= true;
template <>
inline constexpr bool PNP_NAMESPACE::PNP_SUPPORT_NAMESPACE::
enable_default_object_pnp<objects_tests::backwards>
= true;

namespace objects_tests
{
    // ============================================================ static
    // properties

    // A name and a tuple make an object; a tuple alone doesn't.
    static_assert(pnp::object_c<rectangle>);
    static_assert(pnp::object_c<point>);
    static_assert(pnp::object_c<not_opted_in>);
    static_assert(not pnp::object_c<no_name>);
    static_assert(not pnp::object_c<int>);

    // The defaults only exist for types that opted in.
    static_assert(pnp::default_printable_c<rectangle>);
    static_assert(pnp::default_parseable_c<rectangle>);
    static_assert(pnp::default_printer<rectangle>::is_specialized);
    static_assert(pnp::default_parser<rectangle>::is_specialized);
    static_assert(not pnp::default_printable_c<not_opted_in>);
    static_assert(not pnp::default_parseable_c<not_opted_in>);

    // Without a placeholder, try_parse isn't available; parse is.
    static_assert(not pnp::parse_constructible_c<rectangle>);
    static_assert(pnp::parse_constructible_c<point>);

    // The values parsed into have references and const removed.
    static_assert(
        std::same_as<pnp::object_values_t<point>, std::tuple<int, int>>
        );
    static_assert(
        std::same_as<pnp::object_values_t<rectangle>, std::tuple<int, int>>
        );

    // Configured by aggregate initialisation.
    static_assert(std::is_aggregate_v<pnp::object_pnp<rectangle>>);

    // An object_pnp is a printer and a parser, whether or not T opted in.
    static_assert(pnp::printer_c<pnp::object_pnp<not_opted_in>, not_opted_in>);
    static_assert(pnp::parser_c<pnp::object_pnp<not_opted_in>, not_opted_in>);

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
                == u8"not_opted_in" + pnp::to_u8string(std::tuple{ 5 })
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
        SECTION("an object built with pnp_from_tuple")
        {
            const backwards original(2, 1);
            REQUIRE(original.first() == 1);
            backwards  value(0, 0);
            const auto outcome = pnp::parse(pnp::to_u8string(original), value);
            REQUIRE_FALSE(outcome.failed());
            CHECK(value == original);
        }
        SECTION("an object whose fields are objects")
        {
            const line original{ .start = { 1, 2 }, .end = { 3, 4 } };
            const auto result = pnp::try_parse<line>(pnp::to_u8string(original));
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
        const std::u8string              fields = pnp::to_u8string(std::tuple{ 3, 4 });
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
        const std::u8string              two_fields = pnp::to_u8string(std::tuple{ 3, 4 });
        const std::u8string              one_field = pnp::to_u8string(std::tuple{ 3 });
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
        std::u8string     text = pnp::to_u8string(
            line{ .start = { 1, 2 }, .end = { 3, 4 } }
        );
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
        "objects: a tuple printer of its own, set by aggregate initialisation",
        "[objects][configure]"
    )
    {
        const pnp::object_pnp<rectangle, angle_tuple_printer> printer{};
        CHECK(print_with(printer, rectangle(3, 4)) == u8"rectangle<3|4>");
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