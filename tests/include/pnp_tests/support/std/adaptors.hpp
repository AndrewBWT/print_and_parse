#pragma once
#include "pnp/support.hpp"
#include "pnp_tests_internal.hpp"

#include <deque>
#include <functional>
#include <list>
#include <queue>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

namespace pnp_tests
{
// A priority_queue's elements, highest priority first, by popping a copy.
// priority_queue has no ==, and its printed order is the heap's, so this is how
// two are compared.
template <typename Priority_Queue>
std::vector<typename Priority_Queue::value_type>
    drain(
        Priority_Queue queue_arg
    )
{
    std::vector<typename Priority_Queue::value_type> result;
    while (not queue_arg.empty())
    {
        result.push_back(queue_arg.top());
        queue_arg.pop();
    }
    return result;
}

template <typename Adaptor>
Adaptor
    make_adaptor(
        std::initializer_list<typename Adaptor::value_type> values_arg
    )
{
    Adaptor adaptor;
    for (const auto& value : values_arg)
    {
        adaptor.push(value);
    }
    return adaptor;
}

// Bits as 1 and 0: fixed width 1.
inline pnp::support::bool_pnp<>
    adaptor_test_digit_bits()
{
    return pnp::support::bool_pnp<>({.true_value = u8"1", .false_value = u8"0"}
    );
}

// Bytes as two upper-case hex digits with no prefix: fixed width 2.
using adaptor_test_hex_byte = pnp::support::
    fixed_width_pnp<std::uint8_t, pnp::support::fixed_width_pnp_unsigned_hex>;

inline adaptor_test_hex_byte
    adaptor_test_hex_bytes()
{
    return adaptor_test_hex_byte(
        {.prefix = u8"", .upper = true, .fixed_size_output = true}
    );
}

// An adaptor over a container of Element, printed and parsed by Element_Pnp.
template <typename Adaptor, typename Element_Pnp>
using adaptor_test_pnp = pnp::support::generic_adaptor_pnp<
    Adaptor,
    pnp::support::
        generic_sequence_pnp<typename Adaptor::container_type, Element_Pnp>>;
} // namespace pnp_tests

using namespace pnp_tests;

TEST_CASE(
    "adaptors: std::stack prints bottom to top",
    "[adaptors]"
)
{
    const auto stack = make_adaptor<std::stack<int>>({1, 2, 3});
    const auto text  = pnp::to_u8string(stack);
    CHECK(text == u8"[1,2,3]");

    const auto parsed = pnp::try_parse<std::stack<int>>(text);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == stack);
    CHECK(parsed.value.top() == 3);

    // Empty, and a different underlying container.
    CHECK(pnp::to_u8string(std::stack<int>{}) == u8"[]");
    const auto vector_stack
        = make_adaptor<std::stack<int, std::vector<int>>>({4, 5});
    CHECK(pnp::to_u8string(vector_stack) == u8"[4,5]");
    const auto parsed_vector_stack
        = pnp::try_parse<std::stack<int, std::vector<int>>>(u8"[4,5]");
    REQUIRE(succeeded(parsed_vector_stack));
    CHECK(parsed_vector_stack.value == vector_stack);
}

TEST_CASE(
    "adaptors: std::queue prints front to back",
    "[adaptors]"
)
{
    const auto queue = make_adaptor<std::queue<std::string>>({"a", "b,c"});
    const auto text  = pnp::to_u8string(queue);
    CHECK(text == u8"[\"a\",\"b,c\"]");

    const auto parsed = pnp::try_parse<std::queue<std::string>>(text);
    REQUIRE(succeeded(parsed));
    CHECK(parsed.value == queue);
    CHECK(parsed.value.front() == "a");

    // A list-backed queue.
    const auto parsed_list_queue
        = pnp::try_parse<std::queue<int, std::list<int>>>(u8"[1,2,3]");
    REQUIRE(succeeded(parsed_list_queue));
    CHECK(parsed_list_queue.value.front() == 1);
    CHECK(parsed_list_queue.value.back() == 3);
}

TEST_CASE(
    "adaptors: std::priority_queue keeps its priority order",
    "[adaptors]"
)
{
    // Parsing rebuilds the heap from any order.
    const auto parsed
        = pnp::try_parse<std::priority_queue<int>>(u8"[3,1,4,1,5]");
    REQUIRE(succeeded(parsed));
    CHECK(drain(parsed.value) == std::vector<int>{5, 4, 3, 1, 1});

    // A round trip keeps the same priority order, whatever the printed heap
    // order.
    const auto queue = make_adaptor<std::priority_queue<int>>({2, 7, 1, 8});
    const auto reparsed
        = pnp::try_parse<std::priority_queue<int>>(pnp::to_u8string(queue));
    REQUIRE(succeeded(reparsed));
    CHECK(drain(reparsed.value) == drain(queue));

    // A min-heap, through its comparator.
    using min_queue
        = std::priority_queue<int, std::vector<int>, std::greater<int>>;
    const auto min_parsed = pnp::try_parse<min_queue>(u8"[3,1,2]");
    REQUIRE(succeeded(min_parsed));
    CHECK(drain(min_parsed.value) == std::vector<int>{1, 2, 3});

    CHECK(pnp::to_u8string(std::priority_queue<int>{}) == u8"[]");
}

TEST_CASE(
    "adaptors: errors and configuration come from the container",
    "[adaptors]"
)
{
    // The container's own parse errors.
    const auto no_open = pnp::try_parse<std::stack<int>>(u8"1,2]");
    CHECK(not succeeded(no_open));
    CHECK(first_message(no_open) == u8"Expected opening delimiter \"[\".");
    CHECK(not succeeded(pnp::try_parse<std::queue<int>>(u8"[1,x]")));
    CHECK(not succeeded(pnp::try_parse<std::priority_queue<int>>(u8"[1,2")));

    // Configured through the container part.
    using stack_printer = pnp::default_printer<std::stack<int>>;
    const stack_printer spaced(
        {.container
         = stack_printer::container_part_type({.separator_spacing = u8" "})}
    );
    CHECK(
        pnp::to_u8string(spaced, make_adaptor<std::stack<int>>({1, 2, 3}))
        == u8"[1, 2, 3]"
    );

    using queue_parser = pnp::default_parser<std::queue<int>>;
    const queue_parser lenient(
        {.container
         = queue_parser::container_part_type({.skip_whitespace = true})}
    );
    const auto relaxed
        = pnp::try_parse<std::queue<int>>(u8"[ 1 , 2 ]", lenient);
    REQUIRE(succeeded(relaxed));
    CHECK(relaxed.value.front() == 1);
}

TEST_CASE(
    "adaptors: fixed-width elements can drop the separator",
    "[adaptors][fixed_width]"
)
{
    // A stack of bits: [1011], bottom to top.
    using bits_stack = std::stack<bool>;
    using bits_stack_pnp
        = adaptor_test_pnp<bits_stack, pnp::support::bool_pnp<>>;
    const bits_stack_pnp stack_pnp(
        {.container = bits_stack_pnp::container_part_type(
             {.element = adaptor_test_digit_bits(), .separator = u8""}
         )}
    );

    const auto stack = make_adaptor<bits_stack>({true, false, true, true});
    CHECK(pnp::to_u8string(stack_pnp, stack) == u8"[1011]");
    const auto parsed_stack = pnp::try_parse<bits_stack>(u8"[1011]", stack_pnp);
    REQUIRE(succeeded(parsed_stack));
    CHECK(parsed_stack.value == stack);
    CHECK(parsed_stack.value.top() == true);

    // A queue of bytes: [FFFFDDEC45], front to back.
    using bytes_queue = std::queue<std::uint8_t>;
    using bytes_queue_pnp
        = adaptor_test_pnp<bytes_queue, adaptor_test_hex_byte>;
    const bytes_queue_pnp queue_pnp(
        {.container = bytes_queue_pnp::container_part_type(
             {.element = adaptor_test_hex_bytes(), .separator = u8""}
         )}
    );

    const auto queue
        = make_adaptor<bytes_queue>({0xFF, 0xFF, 0xDD, 0xEC, 0x45});
    CHECK(pnp::to_u8string(queue_pnp, queue) == u8"[FFFFDDEC45]");
    const auto parsed_queue
        = pnp::try_parse<bytes_queue>(u8"[FFFFDDEC45]", queue_pnp);
    REQUIRE(succeeded(parsed_queue));
    CHECK(parsed_queue.value == queue);
    CHECK(parsed_queue.value.front() == 0xFF);

    // A priority queue of bytes: the heap is rebuilt from any order.
    using bytes_heap     = std::priority_queue<std::uint8_t>;
    using bytes_heap_pnp = adaptor_test_pnp<bytes_heap, adaptor_test_hex_byte>;
    const bytes_heap_pnp heap_pnp(
        {.container = bytes_heap_pnp::container_part_type(
             {.element = adaptor_test_hex_bytes(), .separator = u8""}
         )}
    );

    const auto parsed_heap
        = pnp::try_parse<bytes_heap>(u8"[0105FF7F]", heap_pnp);
    REQUIRE(succeeded(parsed_heap));
    CHECK(
        drain(parsed_heap.value)
        == std::vector<std::uint8_t>{0xFF, 0x7F, 0x05, 0x01}
    );

    // Empty adaptors still print their delimiters.
    CHECK(pnp::to_u8string(stack_pnp, bits_stack{}) == u8"[]");
}

TEST_CASE(
    "adaptors: the container's rules decide what may be empty",
    "[adaptors][fixed_width]"
)
{
    using bits_stack_pnp
        = adaptor_test_pnp<std::stack<bool>, pnp::support::bool_pnp<>>;
    using container_pnp = bits_stack_pnp::container_part_type;

    // A container's length varies, so its delimiters stay, even with
    // fixed-width elements.
    CHECK_THROWS_AS(
        container_pnp(
            {.open_delimiter  = u8"",
             .element         = adaptor_test_digit_bits(),
             .separator       = u8"",
             .close_delimiter = u8""}
        ),
        std::invalid_argument
    );

    // Elements that aren't fixed width need a separator.
    using int_stack_pnp = pnp::default_printer<std::stack<int>>;
    CHECK_THROWS_AS(
        int_stack_pnp::container_part_type({.separator = u8""}),
        std::invalid_argument
    );

    // A stack's width is its container's, and a container whose length varies
    // has none.
    const bits_stack_pnp no_separator(
        {.container = container_pnp(
             {.element = adaptor_test_digit_bits(), .separator = u8""}
         )}
    );
    CHECK(not no_separator.fixed_output_width().has_value());
    CHECK(not pnp::default_printer<std::queue<int>>{}
                  .fixed_output_width()
                  .has_value());
}

// --- Defaults
// ----------------------------------------------------------------------
static_assert(pnp::default_printable_c<std::stack<int>>);
static_assert(pnp::default_parseable_c<std::stack<int>>);
static_assert(pnp::default_printable_c<std::queue<int>>);
static_assert(pnp::default_parseable_c<std::queue<int>>);
static_assert(pnp::default_printable_c<std::priority_queue<int>>);
static_assert(pnp::default_parseable_c<std::priority_queue<int>>);
static_assert(pnp::default_parseable_c<std::stack<int, std::vector<int>>>);
static_assert(pnp::default_parseable_c<std::queue<int, std::list<int>>>);

// --- The exact-type guard ----------------------------------------------------
static_assert(not pnp::printer_c<
              pnp::default_printer<std::stack<int>>,
              std::queue<int>>);
static_assert(
    not pnp::printer_c<pnp::default_printer<std::stack<int>>, std::deque<int>>,
    "not its container"
);
static_assert(not pnp::printer_c<
              pnp::default_printer<std::queue<int>>,
              std::priority_queue<int>>);