#pragma once

#include "pnp/core/options.hpp"

#include <catch2/catch_test_macros.hpp>
#include <concepts>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace options_tests
{
using PNP_NAMESPACE::current_pnp_options;
using PNP_NAMESPACE::default_pnp_options;
using PNP_NAMESPACE::global_pnp_options;
using PNP_NAMESPACE::pnp_options;
using PNP_NAMESPACE::set_global_pnp_options;
using PNP_NAMESPACE::validate_pnp_options;
using PNP_NAMESPACE::PNP_INTERNAL_NAMESPACE::clamp_fraction;
using PNP_NAMESPACE::PNP_INTERNAL_NAMESPACE::clamp_non_negative_finite;

inline constexpr double nan      = std::numeric_limits<double>::quiet_NaN();
inline constexpr double infinity = std::numeric_limits<double>::infinity();
inline constexpr double largest  = std::numeric_limits<double>::max();
inline constexpr double tiny     = std::numeric_limits<double>::denorm_min();
inline constexpr double just_below_one
    = 1.0 - std::numeric_limits<double>::epsilon() / 2.0;

struct global_options_restorer
{
    pnp_options saved         = global_pnp_options();

    global_options_restorer() = default;
    global_options_restorer(const global_options_restorer&) = delete;
    global_options_restorer&
        operator=(const global_options_restorer&)
        = delete;

    ~global_options_restorer()
    {
        set_global_pnp_options(saved);
    }
};

// ---- Builders: the defaults, with one field changed ----

constexpr pnp_options
    with_threshold(
        const double value_arg
    )
{
    return pnp_options{.parser_input_fraction_of_buffer_threshold = value_arg};
}

constexpr pnp_options
    with_read_fraction(
        const double value_arg
    )
{
    return pnp_options{.buffered_source_minimum_read_fraction = value_arg};
}

constexpr pnp_options
    with_growth_factor(
        const double value_arg
    )
{
    return pnp_options{.buffered_source_growth_factor = value_arg};
}

constexpr pnp_options
    with_compaction(
        const double value_arg
    )
{
    return pnp_options{.buffered_source_compaction_fraction = value_arg};
}

constexpr bool
    is_valid_at_compile_time(
        const pnp_options& options_arg
    )
{
    validate_pnp_options(options_arg);
    return true;
}

// The message validate_pnp_options throws with, or empty if it doesn't
// throw.
inline std::string
    rejection_message(
        const pnp_options& options_arg
    )
{
    try
    {
        validate_pnp_options(options_arg);
    }
    catch (const std::invalid_argument& ex)
    {
        return ex.what();
    }
    return {};
}

inline bool
    message_names(
        const pnp_options&     options_arg,
        const std::string_view field_arg
    )
{
    return rejection_message(options_arg).find(field_arg) != std::string::npos;
}

// ---- Static properties ----

static_assert(std::is_aggregate_v<pnp_options>);
static_assert(std::is_trivially_copyable_v<pnp_options>);

static_assert(noexcept(global_pnp_options()));
static_assert(noexcept(current_pnp_options()));
static_assert(std::same_as<decltype(global_pnp_options()), const pnp_options&>);
static_assert(std::
                  same_as<decltype(current_pnp_options()), const pnp_options&>);

static_assert(&current_pnp_options() == &default_pnp_options);

// ---- Defaults ----

TEST_CASE(
    "default_pnp_options matches the PNP_DEFAULT_ macros",
    "[options][defaults]"
)
{
    STATIC_REQUIRE(
        default_pnp_options.parser_input_context_chars
        == PNP_DEFAULT_PARSER_INPUT_CONTEXT_CHARS
    );
    STATIC_REQUIRE(
        default_pnp_options.parser_input_fraction_of_buffer_threshold
        == PNP_DEFAULT_PARSER_INPUT_BUFFER_THRESHOLD
    );
    STATIC_REQUIRE(
        default_pnp_options.text_source_minimum_release
        == PNP_DEFAULT_TEXT_SOURCE_MINIMUM_RELEASE
    );
    STATIC_REQUIRE(
        default_pnp_options.buffered_source_minimum_block_size
        == PNP_DEFAULT_BUFFERED_SOURCE_MINIMUM_BLOCK_SIZE
    );
    STATIC_REQUIRE(
        default_pnp_options.buffered_source_maximum_reused_block_size
        == PNP_DEFAULT_BUFFERED_SOURCE_MAXIMUM_REUSED_BLOCK_SIZE
    );
    STATIC_REQUIRE(
        default_pnp_options.buffered_source_minimum_read_fraction
        == PNP_DEFAULT_BUFFERED_SOURCE_MINIMUM_READ_FRACTION
    );
    STATIC_REQUIRE(
        default_pnp_options.buffered_source_growth_factor
        == PNP_DEFAULT_BUFFERED_SOURCE_GROWTH_FACTOR
    );
    STATIC_REQUIRE(
        default_pnp_options.buffered_source_compaction_fraction
        == PNP_DEFAULT_BUFFERED_SOURCE_COMPACTION_FRACTION
    );
}

TEST_CASE(
    "pnp_options{} is default_pnp_options",
    "[options][defaults]"
)
{
    constexpr pnp_options empty{};
    STATIC_REQUIRE(
        empty.parser_input_context_chars
        == default_pnp_options.parser_input_context_chars
    );
    STATIC_REQUIRE(
        empty.parser_input_fraction_of_buffer_threshold
        == default_pnp_options.parser_input_fraction_of_buffer_threshold
    );
    STATIC_REQUIRE(
        empty.text_source_minimum_release
        == default_pnp_options.text_source_minimum_release
    );
    STATIC_REQUIRE(
        empty.buffered_source_minimum_block_size
        == default_pnp_options.buffered_source_minimum_block_size
    );
    STATIC_REQUIRE(
        empty.buffered_source_maximum_reused_block_size
        == default_pnp_options.buffered_source_maximum_reused_block_size
    );
    STATIC_REQUIRE(
        empty.buffered_source_minimum_read_fraction
        == default_pnp_options.buffered_source_minimum_read_fraction
    );
    STATIC_REQUIRE(
        empty.buffered_source_growth_factor
        == default_pnp_options.buffered_source_growth_factor
    );
    STATIC_REQUIRE(
        empty.buffered_source_compaction_fraction
        == default_pnp_options.buffered_source_compaction_fraction
    );
}

TEST_CASE(
    "pnp_options keeps defaults for fields not given",
    "[options][defaults]"
)
{
    constexpr pnp_options some{
        .parser_input_context_chars    = 7,
        .text_source_minimum_release   = 100,
        .buffered_source_growth_factor = 2.0,
    };

    STATIC_REQUIRE(some.parser_input_context_chars == 7);
    STATIC_REQUIRE(some.text_source_minimum_release == 100);
    STATIC_REQUIRE(some.buffered_source_growth_factor == 2.0);

    STATIC_REQUIRE(
        some.parser_input_fraction_of_buffer_threshold
        == default_pnp_options.parser_input_fraction_of_buffer_threshold
    );
    STATIC_REQUIRE(
        some.buffered_source_minimum_block_size
        == default_pnp_options.buffered_source_minimum_block_size
    );
    STATIC_REQUIRE(
        some.buffered_source_maximum_reused_block_size
        == default_pnp_options.buffered_source_maximum_reused_block_size
    );
    STATIC_REQUIRE(
        some.buffered_source_minimum_read_fraction
        == default_pnp_options.buffered_source_minimum_read_fraction
    );
    STATIC_REQUIRE(
        some.buffered_source_compaction_fraction
        == default_pnp_options.buffered_source_compaction_fraction
    );
}

// ---- The clamping helpers ----

TEST_CASE(
    "clamp_fraction keeps [0, 1) and maps everything else to 0",
    "[options][clamp]"
)
{
    STATIC_REQUIRE(clamp_fraction(0.0) == 0.0);
    STATIC_REQUIRE(clamp_fraction(0.5) == 0.5);
    STATIC_REQUIRE(clamp_fraction(tiny) == tiny);
    STATIC_REQUIRE(clamp_fraction(just_below_one) == just_below_one);

    STATIC_REQUIRE(clamp_fraction(1.0) == 0.0);
    STATIC_REQUIRE(clamp_fraction(1.5) == 0.0);
    STATIC_REQUIRE(clamp_fraction(-tiny) == 0.0);
    STATIC_REQUIRE(clamp_fraction(-0.5) == 0.0);
    STATIC_REQUIRE(clamp_fraction(infinity) == 0.0);
    STATIC_REQUIRE(clamp_fraction(-infinity) == 0.0);
    STATIC_REQUIRE(clamp_fraction(nan) == 0.0);

    // -0.0 compares equal to 0.0, so it's kept, and still equals 0.0.
    STATIC_REQUIRE(clamp_fraction(-0.0) == 0.0);

    STATIC_REQUIRE(noexcept(clamp_fraction(0.5)));

    // And at run time, where NaN can't be folded away.
    volatile double runtime_nan = nan;
    REQUIRE(clamp_fraction(runtime_nan) == 0.0);
}

TEST_CASE(
    "clamp_non_negative_finite keeps [0, max] and maps everything else to 0",
    "[options][clamp]"
)
{
    STATIC_REQUIRE(clamp_non_negative_finite(0.0) == 0.0);
    STATIC_REQUIRE(clamp_non_negative_finite(tiny) == tiny);
    STATIC_REQUIRE(clamp_non_negative_finite(1.0) == 1.0);
    STATIC_REQUIRE(clamp_non_negative_finite(1.5) == 1.5);
    STATIC_REQUIRE(clamp_non_negative_finite(largest) == largest);

    STATIC_REQUIRE(clamp_non_negative_finite(-tiny) == 0.0);
    STATIC_REQUIRE(clamp_non_negative_finite(-1.0) == 0.0);
    STATIC_REQUIRE(clamp_non_negative_finite(infinity) == 0.0);
    STATIC_REQUIRE(clamp_non_negative_finite(-infinity) == 0.0);
    STATIC_REQUIRE(clamp_non_negative_finite(nan) == 0.0);

    STATIC_REQUIRE(clamp_non_negative_finite(-0.0) == 0.0);

    STATIC_REQUIRE(noexcept(clamp_non_negative_finite(1.0)));

    volatile double runtime_nan = nan;
    REQUIRE(clamp_non_negative_finite(runtime_nan) == 0.0);
}

// ---- validate_pnp_options: compile time ----

static_assert(is_valid_at_compile_time(default_pnp_options));
static_assert(is_valid_at_compile_time(with_threshold(0.0)));
static_assert(is_valid_at_compile_time(with_threshold(just_below_one)));
static_assert(is_valid_at_compile_time(with_read_fraction(0.0)));
static_assert(is_valid_at_compile_time(with_read_fraction(largest)));
static_assert(is_valid_at_compile_time(with_growth_factor(0.0)));
static_assert(is_valid_at_compile_time(with_growth_factor(largest)));
static_assert(is_valid_at_compile_time(with_compaction(0.0)));
static_assert(is_valid_at_compile_time(with_compaction(just_below_one)));

// ---- validate_pnp_options: parser_input_fraction_of_buffer_threshold ----

TEST_CASE(
    "validate_pnp_options: the buffer threshold must be in [0, 1)",
    "[options][validate]"
)
{
    SECTION("accepted")
    {
        for (const double value :
             {0.0, -0.0, tiny, 0.05, 0.0499, 0.5, 0.9501, just_below_one})
        {
            CAPTURE(value);
            REQUIRE_NOTHROW(validate_pnp_options(with_threshold(value)));
        }
    }
    SECTION("rejected")
    {
        for (const double value :
             {1.0, 1.5, -tiny, -0.5, infinity, -infinity, nan})
        {
            CAPTURE(value);
            REQUIRE_THROWS_AS(
                validate_pnp_options(with_threshold(value)),
                std::invalid_argument
            );
        }
    }
    SECTION("named in the message")
    {
        REQUIRE(message_names(
            with_threshold(2.0), "parser_input_fraction_of_buffer_threshold"
        ));
    }
}

// ---- validate_pnp_options: buffered_source_minimum_read_fraction ----

TEST_CASE(
    "validate_pnp_options: the minimum read fraction must be finite and "
    "non-negative",
    "[options][validate]"
)
{
    SECTION("accepted")
    {
        for (const double value : {0.0, -0.0, tiny, 0.25, 1.0, 10.0, largest})
        {
            CAPTURE(value);
            REQUIRE_NOTHROW(validate_pnp_options(with_read_fraction(value)));
        }
    }
    SECTION("rejected")
    {
        for (const double value : {-tiny, -1.0, infinity, -infinity, nan})
        {
            CAPTURE(value);
            REQUIRE_THROWS_AS(
                validate_pnp_options(with_read_fraction(value)),
                std::invalid_argument
            );
        }
    }
    SECTION("named in the message")
    {
        REQUIRE(message_names(
            with_read_fraction(-1.0), "buffered_source_minimum_read_fraction"
        ));
    }
}

// ---- validate_pnp_options: buffered_source_growth_factor ----

TEST_CASE(
    "validate_pnp_options: the growth factor must be finite and non-negative",
    "[options][validate]"
)
{
    SECTION("accepted")
    {
        for (const double value :
             {0.0, -0.0, tiny, 0.5, 1.0, 1.5, 2.0, largest})
        {
            CAPTURE(value);
            REQUIRE_NOTHROW(validate_pnp_options(with_growth_factor(value)));
        }
    }
    SECTION("rejected")
    {
        for (const double value : {-tiny, -1.5, infinity, -infinity, nan})
        {
            CAPTURE(value);
            REQUIRE_THROWS_AS(
                validate_pnp_options(with_growth_factor(value)),
                std::invalid_argument
            );
        }
    }
    SECTION("named in the message")
    {
        REQUIRE(message_names(
            with_growth_factor(nan), "buffered_source_growth_factor"
        ));
    }
}

// ---- validate_pnp_options: buffered_source_compaction_fraction ----

TEST_CASE(
    "validate_pnp_options: the compaction fraction must be in [0, 1)",
    "[options][validate]"
)
{
    SECTION("accepted")
    {
        for (const double value : {0.0, -0.0, tiny, 0.5, 0.9, just_below_one})
        {
            CAPTURE(value);
            REQUIRE_NOTHROW(validate_pnp_options(with_compaction(value)));
        }
    }
    SECTION("rejected")
    {
        for (const double value :
             {1.0, 1.5, -tiny, -0.5, infinity, -infinity, nan})
        {
            CAPTURE(value);
            REQUIRE_THROWS_AS(
                validate_pnp_options(with_compaction(value)),
                std::invalid_argument
            );
        }
    }
    SECTION("named in the message")
    {
        REQUIRE(message_names(
            with_compaction(1.0), "buffered_source_compaction_fraction"
        ));
    }
}

// ---- validate_pnp_options: the size fields ----

TEST_CASE(
    "validate_pnp_options accepts any size field",
    "[options][validate]"
)
{
    constexpr std::size_t max_size = std::numeric_limits<std::size_t>::max();
    for (const std::size_t value : {std::size_t{0}, std::size_t{1}, max_size})
    {
        CAPTURE(value);
        REQUIRE_NOTHROW(validate_pnp_options(
            pnp_options{.parser_input_context_chars = value}
        ));
        REQUIRE_NOTHROW(validate_pnp_options(
            pnp_options{.text_source_minimum_release = value}
        ));
        REQUIRE_NOTHROW(validate_pnp_options(
            pnp_options{.buffered_source_minimum_block_size = value}
        ));
        REQUIRE_NOTHROW(validate_pnp_options(
            pnp_options{.buffered_source_maximum_reused_block_size = value}
        ));
    }
    // The maximum reused size isn't checked against the minimum block size.
    REQUIRE_NOTHROW(validate_pnp_options(pnp_options{
        .buffered_source_minimum_block_size        = 1'000,
        .buffered_source_maximum_reused_block_size = 10,
    }));
}

TEST_CASE(
    "validate_pnp_options: a valid field doesn't hide an invalid one",
    "[options][validate]"
)
{
    // Only the last field checked is wrong.
    REQUIRE(message_names(
        pnp_options{
            .parser_input_fraction_of_buffer_threshold = 0.5,
            .buffered_source_minimum_read_fraction     = 0.5,
            .buffered_source_growth_factor             = 1.5,
            .buffered_source_compaction_fraction       = 1.0,
        },
        "buffered_source_compaction_fraction"
    ));
    // Several wrong: still rejected.
    REQUIRE_THROWS_AS(
        validate_pnp_options(pnp_options{
            .parser_input_fraction_of_buffer_threshold = nan,
            .buffered_source_minimum_read_fraction     = -1.0,
            .buffered_source_growth_factor             = infinity,
            .buffered_source_compaction_fraction       = 2.0,
        }),
        std::invalid_argument
    );
}

// ---- Global options ----

TEST_CASE(
    "global_pnp_options refers to one object",
    "[options][global]"
)
{
    REQUIRE(&global_pnp_options() == &global_pnp_options());
    REQUIRE(&current_pnp_options() == &global_pnp_options());
    REQUIRE(&global_pnp_options() != &default_pnp_options);
}

TEST_CASE(
    "set_global_pnp_options changes every field",
    "[options][global]"
)
{
    const global_options_restorer restorer;

    set_global_pnp_options(pnp_options{
        .parser_input_context_chars                = 42,
        .parser_input_fraction_of_buffer_threshold = 0.25,
        .text_source_minimum_release               = 1'000,
        .buffered_source_minimum_block_size        = 2'000,
        .buffered_source_maximum_reused_block_size = 3'000,
        .buffered_source_minimum_read_fraction     = 0.125,
        .buffered_source_growth_factor             = 2.0,
        .buffered_source_compaction_fraction       = 0.75,
    });

    const pnp_options& global = global_pnp_options();
    REQUIRE(global.parser_input_context_chars == 42);
    REQUIRE(global.parser_input_fraction_of_buffer_threshold == 0.25);
    REQUIRE(global.text_source_minimum_release == 1'000);
    REQUIRE(global.buffered_source_minimum_block_size == 2'000);
    REQUIRE(global.buffered_source_maximum_reused_block_size == 3'000);
    REQUIRE(global.buffered_source_minimum_read_fraction == 0.125);
    REQUIRE(global.buffered_source_growth_factor == 2.0);
    REQUIRE(global.buffered_source_compaction_fraction == 0.75);

    // At run time, current_pnp_options is the global options.
    REQUIRE(&current_pnp_options() == &global);
}

TEST_CASE(
    "set_global_pnp_options leaves the defaults unchanged",
    "[options][global]"
)
{
    const global_options_restorer restorer;

    set_global_pnp_options(pnp_options{
        .parser_input_context_chars          = 3,
        .text_source_minimum_release         = 1,
        .buffered_source_compaction_fraction = 0.0,
    });

    REQUIRE(
        default_pnp_options.parser_input_context_chars
        == PNP_DEFAULT_PARSER_INPUT_CONTEXT_CHARS
    );
    REQUIRE(
        default_pnp_options.text_source_minimum_release
        == PNP_DEFAULT_TEXT_SOURCE_MINIMUM_RELEASE
    );
    REQUIRE(
        default_pnp_options.buffered_source_compaction_fraction
        == PNP_DEFAULT_BUFFERED_SOURCE_COMPACTION_FRACTION
    );
}

TEST_CASE(
    "set_global_pnp_options rejects invalid options, changing nothing",
    "[options][global]"
)
{
    const global_options_restorer restorer;

    constexpr pnp_options         before{
                .parser_input_context_chars                = 11,
                .parser_input_fraction_of_buffer_threshold = 0.3,
                .text_source_minimum_release               = 12,
                .buffered_source_minimum_block_size        = 13,
                .buffered_source_maximum_reused_block_size = 14,
                .buffered_source_minimum_read_fraction     = 0.4,
                .buffered_source_growth_factor             = 1.6,
                .buffered_source_compaction_fraction       = 0.7,
    };
    set_global_pnp_options(before);

    // Each validated field wrong in turn, with every other field changed,
    // so a partial update would show.
    auto changed = [](auto set_bad_field_arg)
    {
        pnp_options options{
            .parser_input_context_chars                = 99,
            .parser_input_fraction_of_buffer_threshold = 0.6,
            .text_source_minimum_release               = 98,
            .buffered_source_minimum_block_size        = 97,
            .buffered_source_maximum_reused_block_size = 96,
            .buffered_source_minimum_read_fraction     = 0.9,
            .buffered_source_growth_factor             = 3.0,
            .buffered_source_compaction_fraction       = 0.1,
        };
        set_bad_field_arg(options);
        return options;
    };
    const pnp_options bad_options[] = {
        changed(
            [](pnp_options& o)
            {
                o.parser_input_fraction_of_buffer_threshold = 1.5;
            }
        ),
        changed(
            [](pnp_options& o)
            {
                o.buffered_source_minimum_read_fraction = -1.0;
            }
        ),
        changed(
            [](pnp_options& o)
            {
                o.buffered_source_growth_factor = infinity;
            }
        ),
        changed(
            [](pnp_options& o)
            {
                o.buffered_source_compaction_fraction = 1.0;
            }
        ),
    };

    for (const pnp_options& bad : bad_options)
    {
        REQUIRE_THROWS_AS(set_global_pnp_options(bad), std::invalid_argument);

        const pnp_options& global = global_pnp_options();
        REQUIRE(
            global.parser_input_context_chars
            == before.parser_input_context_chars
        );
        REQUIRE(
            global.parser_input_fraction_of_buffer_threshold
            == before.parser_input_fraction_of_buffer_threshold
        );
        REQUIRE(
            global.text_source_minimum_release
            == before.text_source_minimum_release
        );
        REQUIRE(
            global.buffered_source_minimum_block_size
            == before.buffered_source_minimum_block_size
        );
        REQUIRE(
            global.buffered_source_maximum_reused_block_size
            == before.buffered_source_maximum_reused_block_size
        );
        REQUIRE(
            global.buffered_source_minimum_read_fraction
            == before.buffered_source_minimum_read_fraction
        );
        REQUIRE(
            global.buffered_source_growth_factor
            == before.buffered_source_growth_factor
        );
        REQUIRE(
            global.buffered_source_compaction_fraction
            == before.buffered_source_compaction_fraction
        );
    }
}

TEST_CASE(
    "global_pnp_options references reflect later sets, copies don't",
    "[options][global]"
)
{
    const global_options_restorer restorer;

    set_global_pnp_options(pnp_options{
        .parser_input_context_chars = 5, .buffered_source_growth_factor = 2.0
    });

    const pnp_options& reference = global_pnp_options();
    const pnp_options  snapshot  = global_pnp_options();

    set_global_pnp_options(pnp_options{
        .parser_input_context_chars = 6, .buffered_source_growth_factor = 3.0
    });

    REQUIRE(reference.parser_input_context_chars == 6);
    REQUIRE(reference.buffered_source_growth_factor == 3.0);
    REQUIRE(snapshot.parser_input_context_chars == 5);
    REQUIRE(snapshot.buffered_source_growth_factor == 2.0);
}

TEST_CASE(
    "current_pnp_options uses the defaults at compile time",
    "[options][current]"
)
{
    const global_options_restorer restorer;

    set_global_pnp_options(pnp_options{
        .parser_input_context_chars
        = PNP_DEFAULT_PARSER_INPUT_CONTEXT_CHARS + 1,
        .parser_input_fraction_of_buffer_threshold = 0.75,
        .text_source_minimum_release
        = PNP_DEFAULT_TEXT_SOURCE_MINIMUM_RELEASE + 1,
        .buffered_source_compaction_fraction = 0.25,
    });

    constexpr std::size_t compile_time_context
        = current_pnp_options().parser_input_context_chars;
    constexpr double compile_time_threshold
        = current_pnp_options().parser_input_fraction_of_buffer_threshold;
    constexpr std::size_t compile_time_release
        = current_pnp_options().text_source_minimum_release;
    constexpr double compile_time_compaction
        = current_pnp_options().buffered_source_compaction_fraction;

    REQUIRE(compile_time_context == PNP_DEFAULT_PARSER_INPUT_CONTEXT_CHARS);
    REQUIRE(
        compile_time_threshold == PNP_DEFAULT_PARSER_INPUT_BUFFER_THRESHOLD
    );
    REQUIRE(compile_time_release == PNP_DEFAULT_TEXT_SOURCE_MINIMUM_RELEASE);
    REQUIRE(
        compile_time_compaction
        == PNP_DEFAULT_BUFFERED_SOURCE_COMPACTION_FRACTION
    );

    REQUIRE(
        current_pnp_options().parser_input_context_chars
        == PNP_DEFAULT_PARSER_INPUT_CONTEXT_CHARS + 1
    );
    REQUIRE(
        current_pnp_options().parser_input_fraction_of_buffer_threshold == 0.75
    );
    REQUIRE(
        current_pnp_options().text_source_minimum_release
        == PNP_DEFAULT_TEXT_SOURCE_MINIMUM_RELEASE + 1
    );
    REQUIRE(current_pnp_options().buffered_source_compaction_fraction == 0.25);
}

TEST_CASE(
    "the restorer puts the global options back",
    "[options][global]"
)
{
    const pnp_options before = global_pnp_options();
    {
        const global_options_restorer restorer;
        set_global_pnp_options(pnp_options{
            .parser_input_context_chars    = 1'234,
            .buffered_source_growth_factor = 4.0,
        });
        REQUIRE(global_pnp_options().parser_input_context_chars == 1'234);
    }

    REQUIRE(
        global_pnp_options().parser_input_context_chars
        == before.parser_input_context_chars
    );
    REQUIRE(
        global_pnp_options().parser_input_fraction_of_buffer_threshold
        == before.parser_input_fraction_of_buffer_threshold
    );
    REQUIRE(
        global_pnp_options().buffered_source_growth_factor
        == before.buffered_source_growth_factor
    );
}
} // namespace options_tests