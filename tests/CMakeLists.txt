# Guard against rebuilding by accident.
if(NOT DEFINED PNP_TESTS_INCLUDED)
  set(PNP_TESTS_INCLUDED TRUE)
  # Set library name so that, if we change it, we only have to change this line
  # Set the name of the example executable once
  set(PNP_TESTS_EXECUTABLE_NAME print_and_parse_tests)

  # The set of all test files. The idea behind these is they mirror the files in the library.
  # However we may change this as we move forward.

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1  # pin to a specific release
  )
  # Set CMake to modern. This should stop silly comparison errors being created.
  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  FetchContent_MakeAvailable(Catch2)
  FetchContent_Declare(
      fmt
      GIT_REPOSITORY https://github.com/fmtlib/fmt.git
      GIT_TAG 11.2.0
  )
  FetchContent_MakeAvailable(fmt)
  
  # These are the libs required for the testing suite.
  set(PNP_TESTS_REQUIRED_LIBS
    print_and_parse
    Catch2::Catch2WithMain
    fmt::fmt
  )

  #There are some additional entries at the end which are not mirrored by files in the library.
  set(PNP_TESTS_FILES
    include/pnp_tests_internal.hpp
    include/pnp_tests/core.hpp
    include/pnp_tests/core/parse.hpp
    include/pnp_tests/core/parse/default_parser.hpp
    include/pnp_tests/core/parse/free_functions.hpp
    include/pnp_tests/core/parse/function_parser.hpp
    include/pnp_tests/core/parse/parse_outcome.hpp
    include/pnp_tests/core/parse/parser_input.hpp
    include/pnp_tests/core/parse/parser_result.hpp
    include/pnp_tests/core/parse/placeholder.hpp
    include/pnp_tests/core/parse/runtime_parser_container.hpp
    include/pnp_tests/core/print.hpp
    include/pnp_tests/core/print/default_printer.hpp
    include/pnp_tests/core/print/free_functions.hpp
    include/pnp_tests/core/print/function_printer.hpp
    include/pnp_tests/core/print/print_exception.hpp
    include/pnp_tests/core/print/runtime_printer_container.hpp
    include/pnp_tests/core/print/text_sink.hpp
    include/pnp_tests/core/concepts.hpp
    include/pnp_tests/core/options.hpp
    include/pnp_tests/sinks.hpp
    include/pnp_tests/sinks/ostream.hpp
    include/pnp_tests/sinks/string.hpp
    include/pnp_tests/sources.hpp
    include/pnp_tests/sources/buffered_text_source.hpp
    include/pnp_tests/sources/string_sources.hpp
    include/pnp_tests/sources/value_reader.hpp
    include/pnp_tests/sources/istream_text_source.hpp
    include/pnp_tests/sources/testing.hpp
    include/pnp_tests/sources/testing/piece_source.hpp
    include/pnp_tests.hpp
    include/pnp_tests/support.hpp
    include/pnp_tests/support/core_language.hpp
    include/pnp_tests/support/core_language/bool.hpp
    include/pnp_tests/support/core_language/integers.hpp
    include/pnp_tests/support/core_language/chars.hpp
    include/pnp_tests/support/core_language/floats.hpp
    include/pnp_tests/support/core_language/c_array.hpp
    include/pnp_tests/support/core_language/nullptr.hpp
    include/pnp_tests/support/core_language/pointer.hpp
    include/pnp_tests/support/std.hpp
    include/pnp_tests/support/std/string.hpp
    include/pnp_tests/support/std/string_view.hpp
    include/pnp_tests/support/std/byte.hpp
    include/pnp_tests/support/std/pair_and_tuple.hpp
    include/pnp_tests/support/std/maps.hpp
    include/pnp_tests/support/std/array.hpp
    include/pnp_tests/support/std/lists.hpp
    include/pnp_tests/support/std/sets.hpp
    include/pnp_tests/support/std/adaptors.hpp
    include/pnp_tests/support/std/views.hpp
    include/pnp_tests/support/std/spans.hpp
    include/pnp_tests/support/std/smart_pointers.hpp
    include/pnp_tests/support/std/optional.hpp
    include/pnp_tests/support/std/expected.hpp
    include/pnp_tests/support/std/variant.hpp
    include/pnp_tests/support/std/bitset.hpp
    include/pnp_tests/support/std/compare.hpp
    include/pnp_tests/support/std/complex.hpp
    include/pnp_tests/support/std/path.hpp
    include/pnp_tests/support/std/valarray.hpp
    include/pnp_tests/support/std/reference_wrapper.hpp
    include/pnp_tests/support/std/initializer_list.hpp
    include/pnp_tests/support/std/flat_maps.hpp
    include/pnp_tests/support/std/flat_sets.hpp
    include/pnp_tests/support/std/inplace_vector.hpp
    include/pnp_tests/support/std/system_error.hpp
    include/pnp_tests/support/std/source_location.hpp
    include/pnp_tests/support/std/stacktrace.hpp
    include/pnp_tests/support/user_types.hpp
    include/pnp_tests/support/user_types/enum.hpp
    include/pnp_tests/support/user_types/objects.hpp
    include/pnp_tests/support/internal.hpp
    include/pnp_tests/support/internal/fixed_width_pnp.hpp
    src/main.cpp
  )
  
  # Sets how the IDE organises the files.
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. FILES
    ${PNP_TESTS_FILES}
  )

  # Add the set of files to executable.
  add_executable(${PNP_TESTS_EXECUTABLE_NAME} ${PNP_TESTS_FILES})
  
  set_common_compile_options(${PNP_TESTS_EXECUTABLE_NAME})
  
  # Add the include directory to the target.
  target_include_directories(${PNP_TESTS_EXECUTABLE_NAME} PRIVATE include)
  # Link the REQUIRED_LIBS to the LIBRARY.
  target_link_libraries(${PNP_TESTS_EXECUTABLE_NAME} PUBLIC ${PNP_TESTS_REQUIRED_LIBS})
  # Ensure that the IDE sees the files in the file structure they are organised in.
  
endif()