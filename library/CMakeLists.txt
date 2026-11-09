# Guard against rebuilding by accident.
if(NOT DEFINED PNP_LIBRARY_INCLUDED)
  set(PNP_LIBRARY_INCLUDED TRUE)
  # Set library name so that, if we change it, we only have to change this line
  set(PNP_LIBRARY_NAME print_and_parse)

  # Set the required libs for the library.
  set(PNP_LIBRARY_REQUIRED_LIBS
    unicode_bridge::unicode_bridge
  )

  # Set the header files variable. This is a list of all the header files.
  set(PNP_INTERNAL_HEADERS
  # Contains everything the library exposes. Most will use this.
    include/pnp.hpp
  # Contains the core functionality. Basically everything but the default printer/parser specializations.
    include/pnp/core.hpp
  # All the core functionality of print_and_parse.
  # The concepts
    include/pnp/core/concepts.hpp
  # This file contains the global options and functionality to change the global options.
    include/pnp/core/options.hpp
  # Exports all the parser logic.
    include/pnp/core/parse.hpp
  # Contains the default parser object.
    include/pnp/core/parse/default_parser.hpp
  # Contains the free functions used to parse text data.
    include/pnp/core/parse/free_functions.hpp
  # Contains parse_outcome, the object for describing the outcome of parsing text.
    include/pnp/core/parse/parse_outcome.hpp
  # The core data structure used for parsing.
    include/pnp/core/parse/parser_input.hpp
  # To ensure parsers save points are maintained, parser_run_key is used to enforce not being able to call an arbitrary parser without some context.
    include/pnp/core/parse/parser_run_key.hpp
  # # placeholder value, for structures without a default constructor, but should still be parsable.
    include/pnp/core/parse/placeholder.hpp
  # The data structure holds the controlling function that gathers more string input for the parser.
    include/pnp/core/parse/text_source.hpp
  # Contains the result objects used for the free functions
    include/pnp/core/parse/parser_result.hpp
    include/pnp/core/parse/function_parser.hpp
    include/pnp/core/parse/runtime_parser_container.hpp
  # Exports all the print logic.
    include/pnp/core/print.hpp
  # Default printer object, no specialisations.
    include/pnp/core/print/default_printer.hpp
  # The printer object created using a function.
    include/pnp/core/print/function_printer.hpp
  # Used as a runtime generic container - can be changed through the runtime.
    include/pnp/core/print/runtime_printer_container.hpp
  # Contains all the print-based free functions
    include/pnp/core/print/free_functions.hpp
  # Printer exception type for when the printer goes wrong.
    include/pnp/core/print/print_exception.hpp
  # Objects used to abstract over printing.
    include/pnp/core/print/text_sink.hpp
  # Contains all the parser sources that are given by the library.
    include/pnp/sources.hpp
  # Exports a string source modelled on an istream.
    include/pnp/sources/buffered_text_source.hpp
    include/pnp/sources/istream_text_source.hpp
    include/pnp/sources/testing.hpp
    include/pnp/sources/testing/piece_source.hpp
    include/pnp/sources/value_reader.hpp
  # Exports a string source modelled on a string.
    include/pnp/sources/complete_text_source.hpp
    include/pnp/sources/string_source.hpp
    include/pnp/sources/string_view_source.hpp
  # Contains all the string sink objects given by the library.
    include/pnp/sinks.hpp
  # Exports a sink modelled on an ostream.
    include/pnp/sinks/ostream.hpp
  # Exports a sink modelled on a string.
    include/pnp/sinks/string.hpp
  # Writers are objects that can read multiple values, with an understanding of how those values are split.
   # include/pnp/readers.hpp
   # include/pnp/readers/delimiter_writer.hpp
  # The parser object created using a function.
  # Internal folder. Stuff in here shouldn't be needed by anyone except us.
    include/pnp/internal/macros.hpp
    include/pnp/support/internal/core_language.hpp
    include/pnp/support/internal/core_language/bool.hpp
    include/pnp/support/internal/core_language/integers.hpp
    include/pnp/support/internal/core_language/chars.hpp
    include/pnp/support/internal/core_language/floats.hpp
    include/pnp/support/internal/core_language/c_array.hpp
    include/pnp/support/internal/core_language/nullptr.hpp
    include/pnp/support/internal/core_language/pointer.hpp
    include/pnp/support/internal/core_language/char_traits.hpp
    include/pnp/support/internal/pnp_options_base.hpp
    include/pnp/support/internal/fixed_width_pnp.hpp
    include/pnp/support/internal/token_or_value.hpp
    include/pnp/support/internal/sequences.hpp
    include/pnp/support/internal/std/string.hpp
    include/pnp/support/internal/std/string_view.hpp
    include/pnp/support/internal/std/maps.hpp
    include/pnp/support/internal/std/ranges.hpp
    include/pnp/support/internal/utility.hpp
    include/pnp/support/internal/tuple_like.hpp
    include/pnp/support/internal/alternatives.hpp
    include/pnp/support/internal/std/span.hpp
    include/pnp/support/internal/std/mdspan.hpp
    include/pnp/support/internal/std/smart_pointers.hpp
    include/pnp/support/internal/std/optional.hpp
    include/pnp/support/internal/std/expected.hpp
    include/pnp/support/internal/std/variant.hpp
    include/pnp/support/internal/std/bitset.hpp
    include/pnp/support/internal/std/compare.hpp
    include/pnp/support/internal/std/complex.hpp
    include/pnp/support/internal/std/path.hpp
    include/pnp/support/internal/std/valarray.hpp
    include/pnp/support/internal/std/flat_maps.hpp
    include/pnp/support/internal/std/inplace_vector.hpp
    include/pnp/support/internal/std/system_error.hpp
    include/pnp/support/internal/std/source_location.hpp
    include/pnp/support/internal/std/stacktrace.hpp
    include/pnp/support/internal/std/reference_wrapper.hpp
    include/pnp/support.hpp
    include/pnp/support/core_language.hpp
    include/pnp/support/core_language/bool.hpp
    include/pnp/support/core_language/chars.hpp
    include/pnp/support/core_language/integers.hpp
    include/pnp/support/core_language/floats.hpp
    include/pnp/support/core_language/nullptr.hpp
    include/pnp/support/core_language/pointer.hpp
    include/pnp/support/core_language/c_array.hpp
    include/pnp/support/core_language/char_array.hpp
    include/pnp/support/user_types.hpp
    include/pnp/support/user_types/objects.hpp
    include/pnp/support/user_types/enum.hpp
    include/pnp/support/std.hpp
    include/pnp/support/std/byte.hpp
    include/pnp/support/std/array.hpp
    include/pnp/support/std/deque.hpp
    include/pnp/support/std/forward_list.hpp
    include/pnp/support/std/list.hpp
    include/pnp/support/std/valarray.hpp
    include/pnp/support/std/vector.hpp
    include/pnp/support/std/string.hpp
    include/pnp/support/std/string_view.hpp
    include/pnp/support/std/pair.hpp
    include/pnp/support/std/tuple.hpp
    include/pnp/support/std/priority_queue.hpp
    include/pnp/support/std/queue.hpp
    include/pnp/support/std/stack.hpp
    include/pnp/support/std/views.hpp
    include/pnp/support/std/span.hpp
    include/pnp/support/std/mdspan.hpp
    include/pnp/support/std/map.hpp
    include/pnp/support/std/initializer_list.hpp
    include/pnp/support/std/flat_map.hpp
    include/pnp/support/std/flat_multimap.hpp
    include/pnp/support/std/flat_set.hpp
    include/pnp/support/std/flat_multiset.hpp
    include/pnp/support/std/inplace_vector.hpp
    include/pnp/support/std/error_code.hpp
    include/pnp/support/std/error_condition.hpp
    include/pnp/support/std/source_location.hpp
    include/pnp/support/std/stacktrace.hpp
    include/pnp/support/std/multimap.hpp
    include/pnp/support/std/unordered_map.hpp
    include/pnp/support/std/unordered_multimap.hpp
    include/pnp/support/std/set.hpp
    include/pnp/support/std/multiset.hpp
    include/pnp/support/std/unordered_set.hpp
    include/pnp/support/std/unordered_multiset.hpp
    include/pnp/support/std/optional.hpp
    include/pnp/support/std/variant.hpp
    include/pnp/support/std/expected.hpp
    include/pnp/support/std/bitset.hpp
    include/pnp/support/std/compare.hpp
    include/pnp/support/std/complex.hpp
    include/pnp/support/std/reference_wrapper.hpp
    include/pnp/support/std/weak_ptr.hpp
    include/pnp/support/std/shared_ptr.hpp
    include/pnp/support/std/unique_ptr.hpp
    include/pnp/support/std/path.hpp
  )

  # Set the cpp files variable. This is a list of all the cpp files.
  set(PNP_INTERNAL_SOURCES
    src/pnp.cpp
  )

  # Set the external header files variable. This is a list of all the external header files. They are libraries not built by print_and_parse.
  set(PNP_EXTERNAL_HEADER_FILES
  )

  # Set the external cpp files variable. This is a list of all the external cpp files. Not made by print_and_parse.
  set(PNP_EXTERNAL_SOURCE_FILES
  )

  # Sets how the IDE organises the files.
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. FILES
    ${PNP_INTERNAL_SOURCES}
    ${PNP_INTERNAL_HEADERS}
    ${PNP_EXTERNAL_SOURCE_FILES}
    ${PNP_EXTERNAL_HEADER_FILES}
  )

  # Adds the library itself.
  add_library(${PNP_LIBRARY_NAME})

  # Sets the sources of the library.
  target_sources(${PNP_LIBRARY_NAME}
    PRIVATE
      ${PNP_INTERNAL_SOURCES}
      ${PNP_EXTERNAL_SOURCE_FILES}
    PUBLIC
      FILE_SET public_headers TYPE HEADERS
      BASE_DIRS include
      FILES ${PNP_INTERNAL_HEADERS}
    PRIVATE
      FILE_SET private_external_headers TYPE HEADERS
      BASE_DIRS include
      FILES ${PNP_EXTERNAL_HEADER_FILES}
  )

  set_common_compile_options(${PNP_LIBRARY_NAME})

  # Add the include directories to the target.
  target_include_directories(${PNP_LIBRARY_NAME}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
  )

  # Link the PNP_LIBRARY_REQUIRED_LIBS to the PNP_LIBRARY_NAME
  target_link_libraries(${PNP_LIBRARY_NAME} PUBLIC ${PNP_LIBRARY_REQUIRED_LIBS})

  # Required for gcc toolchain and the Clang toolchain under linux.
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    target_link_libraries(${PNP_LIBRARY_NAME} PUBLIC stdc++_libbacktrace)
  endif()
endif()
