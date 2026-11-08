#pragma once

#include "pnp/core/concepts.hpp"
#include "pnp/core/parse/default_parser.hpp"
#include "pnp/core/parse/parser_input.hpp"
#include "pnp/core/parse/parser_result.hpp"
#include "pnp/core/parse/parser_run_key.hpp"
#include "pnp/core/print/default_printer.hpp"
#include "pnp/core/print/text_sink.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace concepts_tests
{
    using PNP_NAMESPACE::basic_parser_return_value;
    using PNP_NAMESPACE::parser_input;
    using PNP_NAMESPACE::parser_run_key;
    using PNP_NAMESPACE::text_sink;

    // ---- Printers ----

    struct valid_printer
    {
        void run_printer(const int& value_arg, text_sink& sink_arg) const;
    };

    struct by_value_printer
    {
        void run_printer(int value_arg, text_sink& sink_arg) const;
    };

    struct const_sink_printer
    {
        void run_printer(const int& value_arg, const text_sink& sink_arg) const;
    };

    struct generic_printer
    {
        template <typename U>
        void run_printer(const U& value_arg, text_sink& sink_arg) const;
    };

    struct returns_value_printer
    {
        int run_printer(const int& value_arg, text_sink& sink_arg) const;
    };

    struct non_const_printer
    {
        void run_printer(const int& value_arg, text_sink& sink_arg);
    };

    struct mutable_value_printer
    {
        void run_printer(int& value_arg, text_sink& sink_arg) const;
    };

    struct swapped_arguments_printer
    {
        void run_printer(text_sink& sink_arg, const int& value_arg) const;
    };

    struct missing_sink_printer
    {
        void run_printer(const int& value_arg) const;
    };

    struct string_printer
    {
        void run_printer(const std::u8string& value_arg, text_sink& sink_arg) const;
    };

    struct no_run_printer
    {
    };

    // ---- Parsers ----

    struct valid_parser
    {
        basic_parser_return_value
            run_parser(parser_input& input_arg, int& value_arg, parser_run_key key_arg)
            const;
    };

    struct const_ref_key_parser
    {
        basic_parser_return_value
            run_parser(
                parser_input& input_arg,
                int& value_arg,
                const parser_run_key& key_arg
            ) const;
    };

    struct rvalue_key_parser
    {
        basic_parser_return_value
            run_parser(parser_input& input_arg, int& value_arg, parser_run_key&& key_arg)
            const;
    };

    struct no_key_parser
    {
        basic_parser_return_value
            run_parser(parser_input& input_arg, int& value_arg) const;
    };

    struct string_parser
    {
        basic_parser_return_value
            run_parser(
                parser_input& input_arg,
                std::u8string& value_arg,
                parser_run_key key_arg
            ) const;
    };

    struct no_run_parser
    {
    };

    // ---- Types for the default concepts ----

    struct printer_only_type
    {
    };

    struct parser_only_type
    {
    };

    struct both_type
    {
    };

    struct neither_type
    {
    };

    struct flag_false_type
    {
    };

    struct not_default_initialisable_type
    {
    };

    struct missing_flag_type
    {
    };

    struct flag_but_not_a_printer_type
    {
    };
}

PNP_NS_BEGIN

// printer_only_type: printer, no parser.
template <>
struct default_printer<concepts_tests::printer_only_type>
{
    static constexpr bool is_specialized = true;
    void run_printer(const concepts_tests::printer_only_type&, text_sink&) const;
};

// parser_only_type: parser, no printer.
template <>
struct default_parser<concepts_tests::parser_only_type>
{
    static constexpr bool is_specialized = true;
    basic_parser_return_value
        run_parser(parser_input&, concepts_tests::parser_only_type&, parser_run_key)
        const;
};

// both_type: printer and parser.
template <>
struct default_printer<concepts_tests::both_type>
{
    static constexpr bool is_specialized = true;
    void run_printer(const concepts_tests::both_type&, text_sink&) const;
};

template <>
struct default_parser<concepts_tests::both_type>
{
    static constexpr bool is_specialized = true;
    basic_parser_return_value
        run_parser(parser_input&, concepts_tests::both_type&, parser_run_key) const;
};

// flag_false_type: valid printer and parser, but is_specialized is false.
template <>
struct default_printer<concepts_tests::flag_false_type>
{
    static constexpr bool is_specialized = false;
    void run_printer(const concepts_tests::flag_false_type&, text_sink&) const;
};

template <>
struct default_parser<concepts_tests::flag_false_type>
{
    static constexpr bool is_specialized = false;
    basic_parser_return_value
        run_parser(parser_input&, concepts_tests::flag_false_type&, parser_run_key)
        const;
};

// not_default_initialisable_type: valid, but needs a constructor argument.
template <>
struct default_printer<concepts_tests::not_default_initialisable_type>
{
    static constexpr bool is_specialized = true;
    explicit default_printer(int);
    void run_printer(const concepts_tests::not_default_initialisable_type&, text_sink&)
        const;
};

template <>
struct default_parser<concepts_tests::not_default_initialisable_type>
{
    static constexpr bool is_specialized = true;
    explicit default_parser(int);
    basic_parser_return_value
        run_parser(
            parser_input&,
            concepts_tests::not_default_initialisable_type&,
            parser_run_key
        ) const;
};

// missing_flag_type: valid printer and parser, no is_specialized member.
template <>
struct default_printer<concepts_tests::missing_flag_type>
{
    void run_printer(const concepts_tests::missing_flag_type&, text_sink&) const;
};

template <>
struct default_parser<concepts_tests::missing_flag_type>
{
    basic_parser_return_value
        run_parser(parser_input&, concepts_tests::missing_flag_type&, parser_run_key)
        const;
};

// flag_but_not_a_printer_type: is_specialized is true, but the functions are
// wrong.
template <>
struct default_printer<concepts_tests::flag_but_not_a_printer_type>
{
    static constexpr bool is_specialized = true;
    int run_printer(const concepts_tests::flag_but_not_a_printer_type&, text_sink&)
        const;
};

template <>
struct default_parser<concepts_tests::flag_but_not_a_printer_type>
{
    static constexpr bool is_specialized = true;
    basic_parser_return_value
        run_parser(parser_input&, concepts_tests::flag_but_not_a_printer_type&)
        const;
};

PNP_NS_END

namespace concepts_tests
{
    using PNP_NAMESPACE::default_parseable_c;
    using PNP_NAMESPACE::default_printable_and_parseable_c;
    using PNP_NAMESPACE::default_printable_c;
    using PNP_NAMESPACE::parser_c;
    using PNP_NAMESPACE::printer_c;

    TEST_CASE("printer_c accepts valid printers", "[concepts][printer_c]")
    {
        STATIC_REQUIRE(printer_c<valid_printer, int>);
        STATIC_REQUIRE(printer_c<by_value_printer, int>);
        STATIC_REQUIRE(printer_c<const_sink_printer, int>);
        STATIC_REQUIRE(printer_c<generic_printer, int>);
        STATIC_REQUIRE(printer_c<generic_printer, std::u8string>);
        STATIC_REQUIRE(printer_c<string_printer, std::u8string>);
    }

    TEST_CASE("printer_c rejects invalid printers", "[concepts][printer_c]")
    {
        STATIC_REQUIRE(not printer_c<returns_value_printer, int>);
        STATIC_REQUIRE(not printer_c<non_const_printer, int>);
        STATIC_REQUIRE(not printer_c<mutable_value_printer, int>);
        STATIC_REQUIRE(not printer_c<swapped_arguments_printer, int>);
        STATIC_REQUIRE(not printer_c<missing_sink_printer, int>);
        STATIC_REQUIRE(not printer_c<string_printer, int>);
        STATIC_REQUIRE(not printer_c<no_run_printer, int>);
        STATIC_REQUIRE(not printer_c<int, int>);
    }

    TEST_CASE("parser_c accepts valid parsers", "[concepts][parser_c]")
    {
        STATIC_REQUIRE(parser_c<valid_parser, int>);
        STATIC_REQUIRE(parser_c<string_parser, std::u8string>);
    }

    TEST_CASE("parser_c rejects invalid parsers", "[concepts][parser_c]")
    {
        STATIC_REQUIRE(not parser_c<const_ref_key_parser, int>);
        STATIC_REQUIRE(not parser_c<rvalue_key_parser, int>);
        STATIC_REQUIRE(not parser_c<no_key_parser, int>);
        STATIC_REQUIRE(not parser_c<string_parser, int>);
        STATIC_REQUIRE(not parser_c<no_run_parser, int>);
        STATIC_REQUIRE(not parser_c<int, int>);
    }

    TEST_CASE("default concepts with a printer only", "[concepts][default]")
    {
        STATIC_REQUIRE(default_printable_c<printer_only_type>);
        STATIC_REQUIRE(not default_parseable_c<printer_only_type>);
        STATIC_REQUIRE(not default_printable_and_parseable_c<printer_only_type>);
    }

    TEST_CASE("default concepts with a parser only", "[concepts][default]")
    {
        STATIC_REQUIRE(not default_printable_c<parser_only_type>);
        STATIC_REQUIRE(default_parseable_c<parser_only_type>);
        STATIC_REQUIRE(not default_printable_and_parseable_c<parser_only_type>);
    }

    TEST_CASE("default concepts with both", "[concepts][default]")
    {
        STATIC_REQUIRE(default_printable_c<both_type>);
        STATIC_REQUIRE(default_parseable_c<both_type>);
        STATIC_REQUIRE(default_printable_and_parseable_c<both_type>);
    }

    TEST_CASE("default concepts with neither", "[concepts][default]")
    {
        STATIC_REQUIRE(not default_printable_c<neither_type>);
        STATIC_REQUIRE(not default_parseable_c<neither_type>);
        STATIC_REQUIRE(not default_printable_and_parseable_c<neither_type>);
    }

    TEST_CASE("default concepts require is_specialized to be true", "[concepts][default]")
    {
        STATIC_REQUIRE(printer_c<PNP_NAMESPACE::default_printer<flag_false_type>, flag_false_type>);
        STATIC_REQUIRE(parser_c<PNP_NAMESPACE::default_parser<flag_false_type>, flag_false_type>);

        STATIC_REQUIRE(not default_printable_c<flag_false_type>);
        STATIC_REQUIRE(not default_parseable_c<flag_false_type>);
        STATIC_REQUIRE(not default_printable_and_parseable_c<flag_false_type>);
    }

    TEST_CASE("default concepts require is_specialized to exist", "[concepts][default]")
    {
        STATIC_REQUIRE(printer_c<PNP_NAMESPACE::default_printer<missing_flag_type>, missing_flag_type>);
        STATIC_REQUIRE(parser_c<PNP_NAMESPACE::default_parser<missing_flag_type>, missing_flag_type>);

        STATIC_REQUIRE(not default_printable_c<missing_flag_type>);
        STATIC_REQUIRE(not default_parseable_c<missing_flag_type>);
    }

    TEST_CASE("default concepts require default initialisation", "[concepts][default]")
    {
        STATIC_REQUIRE(printer_c<
            PNP_NAMESPACE::default_printer<not_default_initialisable_type>,
            not_default_initialisable_type
        >);
        STATIC_REQUIRE(parser_c<
            PNP_NAMESPACE::default_parser<not_default_initialisable_type>,
            not_default_initialisable_type
        >);

        STATIC_REQUIRE(not default_printable_c<not_default_initialisable_type>);
        STATIC_REQUIRE(not default_parseable_c<not_default_initialisable_type>);
    }

    TEST_CASE("default concepts require a valid printer or parser", "[concepts][default]")
    {
        STATIC_REQUIRE(not default_printable_c<flag_but_not_a_printer_type>);
        STATIC_REQUIRE(not default_parseable_c<flag_but_not_a_printer_type>);
    }
}