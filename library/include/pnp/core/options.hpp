#pragma once
#include "pnp/internal/macros.hpp"

#include <cstddef>
#include <limits>
#include <stdexcept>

// Default values for pnp_options. Should match across the whole program - e.g.
// through using target_compile_definitions in CMake.
#ifndef PNP_DEFAULT_PARSER_INPUT_CONTEXT_CHARS
    #define PNP_DEFAULT_PARSER_INPUT_CONTEXT_CHARS 20
#endif
#ifndef PNP_DEFAULT_PARSER_INPUT_BUFFER_THRESHOLD
    #define PNP_DEFAULT_PARSER_INPUT_BUFFER_THRESHOLD 0.5
#endif
#ifndef PNP_DEFAULT_TEXT_SOURCE_MINIMUM_RELEASE
    #define PNP_DEFAULT_TEXT_SOURCE_MINIMUM_RELEASE 4'096
#endif
#ifndef PNP_DEFAULT_BUFFERED_SOURCE_MINIMUM_BLOCK_SIZE
    #define PNP_DEFAULT_BUFFERED_SOURCE_MINIMUM_BLOCK_SIZE 4'096
#endif
#ifndef PNP_DEFAULT_BUFFERED_SOURCE_MAXIMUM_REUSED_BLOCK_SIZE
    #define PNP_DEFAULT_BUFFERED_SOURCE_MAXIMUM_REUSED_BLOCK_SIZE 65'536
#endif
#ifndef PNP_DEFAULT_BUFFERED_SOURCE_MINIMUM_READ_FRACTION
    #define PNP_DEFAULT_BUFFERED_SOURCE_MINIMUM_READ_FRACTION 0.25
#endif
#ifndef PNP_DEFAULT_BUFFERED_SOURCE_GROWTH_FACTOR
    #define PNP_DEFAULT_BUFFERED_SOURCE_GROWTH_FACTOR 1.5
#endif
#ifndef PNP_DEFAULT_BUFFERED_SOURCE_COMPACTION_FRACTION
    #define PNP_DEFAULT_BUFFERED_SOURCE_COMPACTION_FRACTION 0.5
#endif
PNP_NS_BEGIN

/*!
 * @brief Global options for pnp.
 */
struct pnp_options
{
    // parser_input settings.
    //! Default for parser_input::context_chars().
    std::size_t parser_input_context_chars{
        PNP_DEFAULT_PARSER_INPUT_CONTEXT_CHARS
    };
    //! Default for parser_input::fraction_of_buffer_threshold().
    double parser_input_fraction_of_buffer_threshold{
        PNP_DEFAULT_PARSER_INPUT_BUFFER_THRESHOLD
    };
    // text_source settings.
    //! Default for text_source::minimum_release().
    std::size_t text_source_minimum_release{
        PNP_DEFAULT_TEXT_SOURCE_MINIMUM_RELEASE
    };
    // buffered_text_source settings.
    //! Default for buffered_text_source::minimum_block_size().
    std::size_t buffered_source_minimum_block_size{
        PNP_DEFAULT_BUFFERED_SOURCE_MINIMUM_BLOCK_SIZE
    };
    //! Default for buffered_text_source::maximum_reused_block_size().
    std::size_t buffered_source_maximum_reused_block_size{
        PNP_DEFAULT_BUFFERED_SOURCE_MAXIMUM_REUSED_BLOCK_SIZE
    };
    //! Used in the calculation of the variable returned from
    //! buffered_text_source::minimum_read_size(). Must be a finite number of at
    //! least 0.
    double buffered_source_minimum_read_fraction{
        PNP_DEFAULT_BUFFERED_SOURCE_MINIMUM_READ_FRACTION
    };
    //! Default for buffered_text_source::growth_factor(). Must be a finite
    //! number of at least 0.
    double buffered_source_growth_factor{
        PNP_DEFAULT_BUFFERED_SOURCE_GROWTH_FACTOR
    };
    //! Default for buffered_text_source::compaction_fraction(). Must be in
    //! [0, 1).
    double buffered_source_compaction_fraction{
        PNP_DEFAULT_BUFFERED_SOURCE_COMPACTION_FRACTION
    };
};

/*!
 * @brief Default pnp_options variable.
 */
inline constexpr pnp_options default_pnp_options{};

/*!
 * @brief Validates pnp_options argument.
 *
 * @param options_arg The pnp_options to validate.
 * @throws std::invalid_argument if parser_input_fraction_of_buffer_threshold
 * or buffered_source_compaction_fraction is outside [0, 1), including NaN.
 * @throws std::invalid_argument if buffered_source_minimum_read_fraction or
 * buffered_source_growth_factor is negative, infinite or NaN.
 */
constexpr void
    validate_pnp_options(const pnp_options& options_arg);

PNP_INTERNAL_NS_BEGIN

/*!
 * @brief Normalises a double to ensure it is non-negative and finite.
 *
 * If it is not non-negative and finite, it is set to 0.0.
 *
 * @param value_arg The value to check.
 * @return Either the original value or the normalised value.
 */
constexpr double
    clamp_non_negative_finite(const double value_arg) noexcept;

/*!
 * @brief Normalises a double to ensure it is at least 0, and less than 1.
 *
 * If it is outside of these bounds, it is set to 0.
 *
 * @param value_arg The value to normalise.
 * @return Either 0 or the original value.
 */
constexpr double
    clamp_fraction(const double value_arg) noexcept;

/*!
 * @brief The program-wide options returned by global_pnp_options(), initialised
 * to default_pnp_options.
 */
inline constinit pnp_options global_options = default_pnp_options;

PNP_INTERNAL_NS_END

/*!
 * @brief Returns the program-wide pnp_options.
 *
 * The reference refers to the global options, so it reflects later calls to
 * set_global_pnp_options. Copy it to keep a snapshot.
 *
 * @return The const reference to the global pnp_options.
 */
[[nodiscard]]
const pnp_options&
    global_pnp_options() noexcept;
/*!
 * @brief Sets the global pnp_options.
 *
 * There is no thread safety in the pnp_options, or its accessor. Must be called
 * either before any parsing starts, or while no other thread is using pnp.
 *
 * @param options_arg The pnp_options to set the global pnp_options to.
 */
void
    set_global_pnp_options(const pnp_options& options_arg);

/*!
 * @brief Returns the options parsing should use. Uses the default pnp_options
 * during constant evaluation, and the global pnp_options at all other times.
 *
 * @return A const reference to either the global or default pnp_options.
 */
[[nodiscard]]
constexpr const pnp_options&
    current_pnp_options() noexcept;

PNP_NS_END

PNP_NS_BEGIN

constexpr void
    validate_pnp_options(
        const pnp_options& options_arg
    )
{
    using namespace PNP_INTERNAL_NAMESPACE;
    if (auto clamped_val
        = clamp_fraction(options_arg.parser_input_fraction_of_buffer_threshold);
        clamped_val != options_arg.parser_input_fraction_of_buffer_threshold)
    {
        throw std::invalid_argument(
            "pnp_options: parser_input_fraction_of_buffer_threshold "
            "must be at least 0.0 and below 1.0."
        );
    }
    if (auto clamped_val = clamp_non_negative_finite(
            options_arg.buffered_source_minimum_read_fraction
        );
        clamped_val != options_arg.buffered_source_minimum_read_fraction)
    {
        throw std::invalid_argument(
            "pnp_options: buffered_source_minimum_read_fraction "
            "must be a finite number of at least 0.0."
        );
    }
    if (auto clamped_val
        = clamp_non_negative_finite(options_arg.buffered_source_growth_factor);
        clamped_val != options_arg.buffered_source_growth_factor)
    {
        throw std::invalid_argument(
            "pnp_options: buffered_source_growth_factor "
            "must be a finite number of at least 0.0."
        );
    }
    if (auto clamped_val
        = clamp_fraction(options_arg.buffered_source_compaction_fraction);
        clamped_val != options_arg.buffered_source_compaction_fraction)
    {
        throw std::invalid_argument(
            "pnp_options: buffered_source_compaction_fraction "
            "must be at least 0.0 and below 1.0."
        );
    }
}

PNP_INTERNAL_NS_BEGIN
constexpr double
    clamp_non_negative_finite(
        const double value_arg
    ) noexcept
{
    using namespace std;
    return value_arg >= 0.0 && value_arg <= numeric_limits<double>::max()
               ? value_arg
               : 0.0;
}

constexpr double
    clamp_fraction(
        const double value_arg
    ) noexcept
{
    // Anything outside [0, 1) goes to 0.0.
    return value_arg >= 0.0 && value_arg < 1.0 ? value_arg : 0.0;
}

PNP_INTERNAL_NS_END
static_assert(
    (validate_pnp_options(default_pnp_options), true),
    "The PNP_DEFAULT_* macros produce invalid default pnp_options."
);

inline const pnp_options&
    global_pnp_options() noexcept
{
    return PNP_INTERNAL_NAMESPACE::global_options;
}

inline void
    set_global_pnp_options(
        const pnp_options& options_arg
    )
{
    validate_pnp_options(options_arg);
    PNP_INTERNAL_NAMESPACE::global_options = options_arg;
}

constexpr const pnp_options&
    current_pnp_options() noexcept
{
    if consteval
    {
        return default_pnp_options;
    }
    else
    {
        return global_pnp_options();
    }
}

PNP_NS_END