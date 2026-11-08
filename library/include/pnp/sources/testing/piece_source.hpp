#pragma once
// pnp namespaces.
#include "pnp/internal/macros.hpp"
// Base class.
#include "pnp/sources/buffered_text_source.hpp"
// parser concepts.
#include "pnp/core/concepts.hpp"
// free functions for base tests.
#include "pnp/core/parse/free_functions.hpp"

// std::constructible_from.
#include <concepts>
// std::size_t.
#include <cstddef>
// std::uint64_t.
#include <cstdint>
// std::uniform_random_bit_generator. Only the concept is used: no
// distributions, as they aren't portable between standard libraries.
#include <random>
// The space read_piece() writes to.
#include <span>
// std::invalid_argument.
#include <stdexcept>
// The text, and std::char_traits.
#include <string>
// std::forward.
#include <utility>

PNP_NS_BEGIN
PNP_TESTING_NS_BEGIN

/*!
 * @brief A text source for testing parsers. It hands its text over a few code
 * units at a time.
 *
 * Real input, especially from streams, can arrive in any kind of configuration;
 * sometimes there is no input, sometimes bytes arrive one at a time, sometimes
 * there is huge amounts of text before nothing for a while.
 *
 * A parser in pnp should be robust and be able to deal with such eventualities.
 * piece_text_source is a source that splits text on purpose, so a parsers
 * behaviour can be checked.
 *
 * The constructor takes a full string as input, keesp a copy of the text, and
 * splits it according to its configuration. The configuration can either allow
 * a fixed number of code units to be given each time read_piece() is called, or
 * a random number of code units (between two user-defined bounds) is given.
 *
 * A fixed size of 1 splits the text between every code unit, which is the most
 * thorough check of where splits can fail. Random sizes can give mixed
 * patterns. The sizes come straight from the generator, with no statistical
 * distribution. The generator is designed so the user can seed it as they
 * desire - allowing them to re-generate the same piece sizes.
 *
 * For the most thorough check, also pass a minimum release of 0, a small
 * minimum block size, and construct the parser_input with a
 * fraction_of_buffer_threshold of 0 and context_chars of 0. Text is then
 * released as early as possible, and moved often, so a parser that keeps a
 * plain view of text it no longer holds through an input_text or input_mark
 * should fail quickly.
 *
 * By default it behaves like input that may wait for more, such as std::cin,
 * so nothing is read ahead of what the parser asks for. Pass
 * reads_without_waiting_arg as true to have it behave like a file instead.
 */
struct piece_source : public buffered_text_source
{
public:
    template <typename S>
    requires std::constructible_from<std::u8string, S&&>
    explicit piece_source(
        S&&               text_arg,
        const std::size_t piece_size_arg            = 1,
        const bool        reads_without_waiting_arg = false,
        const std::size_t minimum_release_arg
        = current_pnp_options().text_source_minimum_release,
        const std::size_t minimum_block_size_arg
        = current_pnp_options().buffered_source_minimum_block_size,
        const std::size_t maximum_reused_block_size_arg
        = current_pnp_options().buffered_source_maximum_reused_block_size,
        const double minimum_read_fraction_arg
        = current_pnp_options().buffered_source_minimum_read_fraction,
        const double growth_factor_arg
        = current_pnp_options().buffered_source_growth_factor,
        const double compaction_fraction_arg
        = current_pnp_options().buffered_source_compaction_fraction
    );
    template <typename S, typename G>
    requires std::constructible_from<std::u8string, S&&>
             && std::uniform_random_bit_generator<G>
    explicit piece_source(
        S&&               text_arg,
        G&                generator_arg,
        const std::size_t minimum_piece_size_arg,
        const std::size_t maximum_piece_size_arg,
        const bool        reads_without_waiting_arg = false,
        const std::size_t minimum_release_arg
        = current_pnp_options().text_source_minimum_release,
        const std::size_t minimum_block_size_arg
        = current_pnp_options().buffered_source_minimum_block_size,
        const std::size_t maximum_reused_block_size_arg
        = current_pnp_options().buffered_source_maximum_reused_block_size,
        const double minimum_read_fraction_arg
        = current_pnp_options().buffered_source_minimum_read_fraction,
        const double growth_factor_arg
        = current_pnp_options().buffered_source_growth_factor,
        const double compaction_fraction_arg
        = current_pnp_options().buffered_source_compaction_fraction
    );
    [[nodiscard]]
    std::size_t
        minimum_piece_size() const noexcept;
    [[nodiscard]]
    std::size_t
        maximum_piece_size() const noexcept;
private:
    // Use type erasure as, while its slower (and not constexpr), this is just
    // for testing. speed really isn't an issue here.
    struct random_bits
    {
    public:
        virtual ~random_bits() = default;
        virtual std::uint64_t
            operator()()
            = 0;
    };

    template <typename G>
    requires std::uniform_random_bit_generator<G>
    struct random_bits_from final : public random_bits
    {
    public:
        explicit random_bits_from(G& generator_arg) noexcept;
        std::uint64_t
            operator()() override;
    private:
        G* _generator;
    };

    read_result
        read_piece(const std::span<char8_t> into_arg) override;
    std::size_t
                                 next_piece_size();
    std::u8string                _text;
    std::size_t                  _next = 0;
    std::size_t                  _minimum_piece_size;
    std::size_t                  _maximum_piece_size;
    std::unique_ptr<random_bits> _random;
};

struct check_parser_using_split_text_config
{
    //! The exact splits on the input to run.
    std::vector<std::size_t> exact_splits_to_run = {1, 3, 5, 7};
    //! Random seed.
    std::uint64_t rng_seed                       = 0;
    //! Number of random values to generate
    std::size_t n_random_values                  = 20;
    std::size_t minimum_random_piece_size        = 0;
    //! Maximum random piece size. Should be atleast 1 (or no progresw will be
    //! made).
    std::size_t maximum_random_piece_size        = 10;
    //! Whether to stress out the memory.
    bool stress_memory                           = false;
};

/*!
 * @brief A funciton to test a parser, which can be configured to simulate
 * different types of buffering and text delay using piece_source.
 *
 * A parser can work on a whole string, but fail if not designed properly when
 * its input arrives a piece at a time - for example, from std::cin, a file, or
 * a network connection. This function parses str_arg once as a whole, then
 * again through a piece_source for each configuration in config_arg, and
 * compares each result with the first. It stops at the first configuration
 * whose result differs.
 *
 * The configurations tried are:
 * - Each fixed piece size in config_arg.exact_splits_to_run.
 * - Random piece sizes, from config_arg.minimum_random_piece_size to
 *   config_arg.maximum_random_piece_size, for each of config_arg.random_seeds
 *   seeds, starting at config_arg.first_seed. The sizes come from a
 *   std::mt19937_64, with no standard distribution, so the same seed splits
 *   the text the same way on every platform.
 * Each is tried with reads_without_waiting both true and false. If
 * config_arg.stress_memory is true, the source also releases and moves text
 * as often as it can, which catches a parser keeping a plain view of text it
 * no longer holds through an input_text.
 *
 * Results match if they have the same outcome state, position, parser error
 * messages and error, the same context before and at the position, the same
 * line and column, and values equal by equal_to_arg. The value is compared
 * even when the parse failed, as a parser should change it in the same way
 * however its text arrived. The context after the position isn't compared, as
 * a source that waits for input can only show what has arrived.
 *
 * An exception thrown while parsing a configuration is caught, and reported as
 * that configuration's failure.
 *
 * Each parse uses the parser_input settings from current_pnp_options().
 *
 * @tparam T The type parsed.
 * @tparam Parser The type of parser_arg. Defaults to default_parser<T>.
 * @tparam Eq The type of equal_to_arg. Defaults to std::equal_to<T>.
 * @param config_arg Which configurations to try.
 * @param str_arg The text to parse.
 * @param parser_arg The parser to check. Defaults to a default constructed
 * Parser.
 * @param equal_to_arg Compares two parsed values, returning true if they're
 * equal. Defaults to a default constructed Eq.
 * @return Empty if every configuration gave the same result as the whole
 * text. Otherwise, a description of the first configuration that didn't: the
 * piece_source call that reproduces it, what differed, and both results.
 * @throws std::invalid_argument If config_arg holds a fixed piece size of 0,
 * or a random range whose maximum is 0 or below its minimum.
 */
template <
    typename T,
    typename Parser = default_parser<T>,
    typename Eq     = std::equal_to<T>>
requires parser_c<Parser, T> && parse_constructible_c<T>
         && std::predicate<const Eq&, const T&, const T&>
std::optional<std::string>
    check_parser_using_split_text(
        const check_parser_using_split_text_config& config_arg,
        const std::u8string_view                    str_arg,
        const Parser&                               parser_arg   = Parser{},
        const Eq&                                   equal_to_arg = Eq{}
    );
PNP_TESTING_NS_END
PNP_NS_END

PNP_NS_BEGIN

PNP_TESTING_NS_BEGIN
template <typename S>
requires std::constructible_from<std::u8string, S&&>
inline piece_source::piece_source(
    S&&               text_arg,
    const std::size_t piece_size_arg,
    const bool        reads_without_waiting_arg,
    const std::size_t minimum_release_arg,
    const std::size_t minimum_block_size_arg,
    const std::size_t maximum_reused_block_size_arg,
    const double      minimum_read_fraction_arg,
    const double      growth_factor_arg,
    const double      compaction_fraction_arg
)
    : buffered_text_source(
          reads_without_waiting_arg,
          minimum_release_arg,
          minimum_block_size_arg,
          maximum_reused_block_size_arg,
          minimum_read_fraction_arg,
          growth_factor_arg,
          compaction_fraction_arg
      )
    , _text(std::forward<S>(text_arg))
    , _minimum_piece_size(piece_size_arg)
    , _maximum_piece_size(piece_size_arg)
{
    if (piece_size_arg == 0)
    {
        throw std::invalid_argument(
            "piece_source: a fixed piece size must be at least 1."
        );
    }
}

template <typename S, typename G>
requires std::constructible_from<std::u8string, S&&>
             && std::uniform_random_bit_generator<G>
inline piece_source::piece_source(
    S&&               text_arg,
    G&                generator_arg,
    const std::size_t minimum_piece_size_arg,
    const std::size_t maximum_piece_size_arg,
    const bool        reads_without_waiting_arg,
    const std::size_t minimum_release_arg,
    const std::size_t minimum_block_size_arg,
    const std::size_t maximum_reused_block_size_arg,
    const double      minimum_read_fraction_arg,
    const double      growth_factor_arg,
    const double      compaction_fraction_arg
)
    : buffered_text_source(
          reads_without_waiting_arg,
          minimum_release_arg,
          minimum_block_size_arg,
          maximum_reused_block_size_arg,
          minimum_read_fraction_arg,
          growth_factor_arg,
          compaction_fraction_arg
      )
    , _text(std::forward<S>(text_arg))
    , _minimum_piece_size(minimum_piece_size_arg)
    , _maximum_piece_size(maximum_piece_size_arg)
    , _random(std::make_unique<random_bits_from<G>>(generator_arg))
{
    if (maximum_piece_size_arg == 0
        || maximum_piece_size_arg < minimum_piece_size_arg)
    {
        throw std::invalid_argument(
            "piece_source: the largest piece size must be at least 1, and at "
            "least the smallest."
        );
    }
}

inline std::size_t
    piece_source::minimum_piece_size() const noexcept
{
    return _minimum_piece_size;
}

inline std::size_t
    piece_source::maximum_piece_size() const noexcept
{
    return _maximum_piece_size;
}

template <typename G>
requires std::uniform_random_bit_generator<G>
inline piece_source::random_bits_from<G>::random_bits_from(
    G& generator_arg
) noexcept
    : _generator(&generator_arg)
{}

template <typename G>
requires std::uniform_random_bit_generator<G>
inline std::uint64_t
    piece_source::random_bits_from<G>::operator()()
{
    return static_cast<std::uint64_t>((*_generator)() - G::min());
}

inline buffered_text_source::read_result
    piece_source::read_piece(
        const std::span<char8_t> into_arg
    )
{
    using namespace std;
    if (_next >= _text.size())
    {
        return read_result{.code_units_written = 0, .more_content = false};
    }
    const size_t wanted = next_piece_size();
    // The smallest of the size wanted, what's left, and the room given.
    const size_t left   = _text.size() - _next;
    const size_t piece  = wanted < left ? wanted : left;
    const size_t count  = piece < into_arg.size() ? piece : into_arg.size();
    if (count != 0)
    {
        char_traits<char8_t>::copy(
            into_arg.data(), _text.data() + _next, count
        );
    }
    _next += count;
    return read_result{
        .code_units_written = count,
        .more_content       = _next < _text.size(),
    };
}

inline std::size_t
    piece_source::next_piece_size()
{
    if (_random == nullptr)
    {
        return _minimum_piece_size;
    }
    // Can't overflow: the largest piece size is at least 1, so the range is
    // at most the largest piece size.
    const std::uint64_t range
        = static_cast<std::uint64_t>(_maximum_piece_size - _minimum_piece_size)
          + 1;
    return _minimum_piece_size + static_cast<std::size_t>((*_random)() % range);
}

template <typename T, typename Parser, typename Eq>
requires parser_c<Parser, T> && parse_constructible_c<T>
         && std::predicate<const Eq&, const T&, const T&>
std::optional<std::string>
    check_parser_using_split_text(
        const check_parser_using_split_text_config& config_arg,
        const std::u8string_view                    str_arg,
        const Parser&                               parser_arg,
        const Eq&                                   equal_to_arg
    )
{
    using namespace std;
    using namespace UNICODE_BRIDGE_NAMESPACE;
    const parse_outcome_with_value<T> expected_result
        = pnp::try_parse<T>(str_arg, parser_arg);
    auto bool_to_str = [](const bool bool_arg)
    {
        return bool_arg ? "true" : "false";
    };
    auto parse_outcome_with_val_to_str =
        [&](const parse_outcome_with_value<T>& parse_outcome_arg) -> std::string
    {
        const parse_outcome& local_outcome = parse_outcome_arg.outcome;
        string rv = "parse_outcome_with_value<T>(outcome = (position = ";
        rv.append(std::to_string(local_outcome.position));
        rv.append(", messages = {");
        for (size_t idx = 0; auto& msg : local_outcome.parser_error_messages)
        {
            rv.append(idx == 0 ? "" : ", ");
            idx++;
            rv.append(cast_string_view<char>(msg));
        }
        rv.append("}, context = {before_pos_text = ");
        const auto& local_context = local_outcome.parsers_error_context;
        rv.append(cast_string_view<char>(local_context.before_pos_text));
        rv.append(", before_text_cut_off = ");
        rv.append(bool_to_str(local_context.before_text_cut_off));
        rv.append(", current_pos_text = ");
        rv.append(cast_string_view<char>(local_context.current_pos_text));
        rv.append(", after_pos_text = ");
        rv.append(cast_string_view<char>(local_context.after_pos_text));
        rv.append(", after_text_cut_off = ");
        rv.append(bool_to_str(local_context.after_text_cut_off));
        rv.append(", location = {line = ");
        const text_location& local_location = local_context.location;
        rv.append(std::to_string(local_location.line));
        rv.append(", column = ");
        rv.append(std::to_string(local_location.column));
        rv.append("}, parse_outcome_state = ");
        rv.append(std::to_string(
            std::to_underlying(local_outcome.parse_outcome_state)
        ));
        rv.append(", error = \"");
        rv.append(cast_string_view<char>(
            local_outcome.error.to_message(local_outcome.position)
        ));
        rv.append("\"}))");
        return rv;
    };
    optional<string> res_var;
    string           configuration_string;
    auto             run_piece_source = [&](piece_source&& piece_source_arg)
    {
        vector<string> errors;
        try
        {
            const parse_outcome_with_value<T> next_result
                = try_parse<T>(piece_source_arg, parser_arg);
            if (not equal_to_arg(next_result.value, expected_result.value))
            {
                errors.push_back("piece_source parsed value not equal to "
                                 "natively parsed result");
            }
            const parse_outcome& next_outcome     = next_result.outcome;
            const parse_outcome& expected_outcome = expected_result.outcome;
            if (next_outcome.position != expected_outcome.position)
            {
                errors.push_back(
                    "piece_source final input position not identical to "
                    "natively parsed final input position "
                );
            }
            if (next_outcome.parser_error_messages
                != expected_outcome.parser_error_messages)
            {
                errors.push_back("piece_source error messages not identical to "
                                 "natively parsed result error messages");
            }
            const parse_error_context& local_next_context
                = next_outcome.parsers_error_context;
            const parse_error_context& local_expected_context
                = expected_outcome.parsers_error_context;
            if (local_next_context.before_pos_text
                != local_expected_context.before_pos_text)
            {
                errors.push_back(
                    "piece_source parse_error_context before_pos_text not "
                    "identical to natively parsed parse_error_context "
                    "before_pos_text"
                );
            }
            if (local_next_context.before_text_cut_off
                != local_expected_context.before_text_cut_off)
            {
                errors.push_back(
                    "piece_source parse_error_context before_text_cut_off not "
                    "identical to natively parsed parse_error_context "
                    "before_text_cut_off"
                );
            }
            if (local_next_context.current_pos_text
                != local_expected_context.current_pos_text)
            {
                errors.push_back(
                    "piece_source parse_error_context current_pos_text not "
                    "identical to natively parsed parse_error_context "
                    "current_pos_text"
                );
            }
            if (local_next_context.location.line
                != local_expected_context.location.line)
            {
                errors.push_back(
                    "piece_source parse_error_context location line not "
                    "identical to natively parsed location line "
                );
            }
            if (local_next_context.location.column
                != local_expected_context.location.column)
            {
                errors.push_back(
                    "piece_source parse_error_context location column not "
                    "identical to natively parsed location column "
                );
            }
            if (next_outcome.parse_outcome_state
                != expected_outcome.parse_outcome_state)
            {
                errors.push_back(
                    "piece_source parse_outcome_state not "
                    "identical to natively parsed parse_outcome_state"
                );
            }
            if (next_outcome.error.to_message(next_outcome.position)
                != expected_outcome.error.to_message(expected_outcome.position))
            {
                errors.push_back("piece_source error message not "
                                 "identical to natively parsed error message");
            }
            if (not errors.empty())
            {
                res_var = "Errors found when running " + configuration_string
                          + ". expected output = "
                          + parse_outcome_with_val_to_str(expected_result)
                          + ", actual output = "
                          + parse_outcome_with_val_to_str(next_result)
                          + ". Errors are as follows: ";
                for (auto& error : errors)
                {
                    res_var.value().append(error);
                    res_var.value().append("; ");
                }
            }
        }
        catch (const std::exception& exception_arg)
        {
            res_var = "Errors found when running " + configuration_string
                      + ". expected output = "
                      + parse_outcome_with_val_to_str(expected_result)
                      + ", piece source threw an exception with the following "
                        "text \""
                      + string(exception_arg.what()) + "\"";
        }
    };
    for (auto& exact_split : config_arg.exact_splits_to_run)
    {
        for (auto& truth_value : {true, false})
        {
            configuration_string = "piece_source("
                                   + string(cast_string_view<char>(str_arg))
                                   + ", " + std::to_string(exact_split) + ", "
                                   + bool_to_str(truth_value) + ")";
            run_piece_source(piece_source(str_arg, exact_split, truth_value));
            if (res_var.has_value())
            {
                return res_var;
            }
        }
    }
    const pnp_options& options = current_pnp_options();
    const size_t       minimum_release
        = config_arg.stress_memory ? 0 : options.text_source_minimum_release;
    const size_t minimum_block_size
        = config_arg.stress_memory ? 1
                                   : options.buffered_source_minimum_block_size;
    for (size_t idx = 0; idx < config_arg.n_random_values; ++idx)
    {
        const uint64_t seed = config_arg.rng_seed + idx;
        for (const bool truth_value : {true, false})
        {
            mt19937_64 generator(seed);
            configuration_string
                = "piece_source(text, std::mt19937_64(" + std::to_string(seed)
                  + "), " + std::to_string(config_arg.minimum_random_piece_size)
                  + ", " + std::to_string(config_arg.maximum_random_piece_size)
                  + ", " + bool_to_str(truth_value) + ", "
                  + std::to_string(minimum_release) + ", "
                  + std::to_string(minimum_block_size) + ")";
            run_piece_source(piece_source(
                str_arg,
                generator,
                config_arg.minimum_random_piece_size,
                config_arg.maximum_random_piece_size,
                truth_value,
                minimum_release,
                minimum_block_size,
                options.buffered_source_maximum_reused_block_size,
                options.buffered_source_minimum_read_fraction,
                options.buffered_source_growth_factor,
                options.buffered_source_compaction_fraction
            ));
            if (res_var.has_value())
            {
                return res_var;
            }
        }
    }
    return res_var;
}

PNP_TESTING_NS_END
PNP_NS_END