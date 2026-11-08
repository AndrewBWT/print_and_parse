#pragma once
#include "pnp/core/parse/parser_result.hpp" // basic_parser_return_value
#include "pnp/internal/macros.hpp"

#include <concepts>
#include <utility>

PNP_NS_BEGIN

struct parser_input;

/*!
 * @brief A parser must have a particular signature (see
 * check_parser_takes_key), and this object is one of the arguments to that
 * function. Instances of parser_run_key can only be created by parser_input.
 *
 * parser_run_key is used to ensure that parsers
 * can only be called using a parser_input (the parser must take the
 * parser_run_key by value). This ensures that sub-parsers, for example in
 * sequence parsers, have to use the current parser_input.
 */
struct parser_run_key
{
public:
    // Keys shouldn't be able to be copied or moved, so these are all deleted.
    // Otherwise they could be given to child parsers - which we don't want.
    parser_run_key(const parser_run_key&) = delete;
    parser_run_key(parser_run_key&&)      = delete;
    parser_run_key&
        operator=(const parser_run_key&)
        = delete;
    parser_run_key&
        operator=(parser_run_key&&)
        = delete;
private:
    friend struct parser_input;
    friend struct check_parser_takes_key;
    constexpr parser_run_key() noexcept = default;
};

/*!
 * @brief A struct that provides a way of checking that a parser has the
 * correct interface.
 *
 * check_parser_takes_key is a friend to parser_run_key, so that it can check
 * whether a parser has the correct function signature.
 *
 * That is, it has a function called run_parser that takes a parser_input, a
 * reference to a T and a parser_run_key (taken by value), and returns a
 * basic_parser_return_value object.
 */
struct check_parser_takes_key
{
    //! Whether Parser's run_parser takes a run_key.
    template <typename Parser, typename T>
    static constexpr bool takes_key =
        requires (const Parser & parser_arg, parser_input & input_arg, T & value_arg) {
            {
                parser_arg.run_parser(input_arg, value_arg, parser_run_key())
            } -> std::same_as<basic_parser_return_value>;
    }
    // Rejects a const lvalue reference (it could be passed to a child parser - which we don't want).
    and not requires (
        const Parser& parser_arg,
        parser_input& input_arg,
        T& value_arg,
        const parser_run_key& key_arg
        ) {
        parser_arg.run_parser(input_arg, value_arg, key_arg);
    }
    // Rejects an rvalue reference (it could be moved on to a child parser, which we don't want)
    and not requires (
        const Parser& parser_arg,
        parser_input& input_arg,
        T& value_arg,
        parser_run_key&& key_arg
        ) {
        parser_arg.run_parser(input_arg, value_arg, std::move(key_arg));
    };
};

PNP_NS_END