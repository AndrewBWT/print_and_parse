function(set_common_compile_options TARGET)
  target_compile_features(${TARGET} PUBLIC cxx_std_23)
  
  target_compile_options(${TARGET} PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/bigobj>"
    "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
  )
  if(MSVC)
#    target_compile_options(${TARGET} PRIVATE /W4 /permissive- /bigobj /utf-8)
  else()
#    target_compile_options(${TARGET} PRIVATE -Wall -Wextra -pedantic)
  endif()
  set_target_properties(${TARGET} PROPERTIES CXX_EXTENSIONS OFF)
endfunction()

# Current CMake version. We have not tested with other versions.
cmake_minimum_required(VERSION 3.28)

# Set up the project information
project(
  print_and_parse
  VERSION 			0.0.1
  DESCRIPTION 		"An library for easy printing and parsing."
  LANGUAGES 		CXX
)

# Set up the variables TOP_LEVEL_PROJECT and RUNNING_TESTS

# PNP_TOP_LEVEL_PROJECT informs the user if this is the top level project.
# If it is the top level project, we assume that it is being built on its own.
set(PNP_TOP_LEVEL_PROJECT FALSE)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(PNP_TOP_LEVEL_PROJECT TRUE)
endif()

# We include this here so that the variable BUILD_TESTING is now defined as TRUE (due to its inclusion by CTest).
# Remember, the user can set BUILD_TESTING themselves if they so wish, include(CTest) won't override that value.
if(PNP_TOP_LEVEL_PROJECT)
  include(CTest)
endif()

# Ensures that BUILD_TESTING is defined as FALSE. If LSPS_TOP_LEVEL_PROJECT == FALSE, it would be an undefined value.
if(NOT DEFINED BUILD_TESTING)
  set(BUILD_TESTING OFF)
endif()

# Sets up the PNP_RUNNING_TESTS variable.
set(PNP_RUNNING_TESTS FALSE)
if(PNP_TOP_LEVEL_PROJECT AND BUILD_TESTING)
  set(PNP_RUNNING_TESTS TRUE)
endif()

# Set up the options for the user.
option(PNP_ENABLE_DOCS "Build print_and_parse documentation" ${PNP_TOP_LEVEL_PROJECT})
option(PNP_ENABLE_TESTS "Build print_and_parse tests" ${PNP_RUNNING_TESTS})
option(PNP_ENABLE_EXAMPLES "Build print_and_parse examples" ${PNP_TOP_LEVEL_PROJECT})

if(NOT TARGET unicode_bridge::unicode_bridge)
  include(FetchContent)
  if(PNP_TOP_LEVEL_PROJECT)
    FetchContent_Declare(
      unicode_bridge
      GIT_REPOSITORY https://github.com/AndrewBWT/unicode_bridge.git
      GIT_TAG        main
    )
    FetchContent_MakeAvailable(unicode_bridge)
  else()
    FetchContent_GetProperties(unicode_bridge)
    if(NOT unicode_bridge_POPULATED)
      message(FATAL_ERROR "Required target unicode_bridge::unicode_bridge not found.
Please make sure:
  - unicode_bridge::unicode_bridge is available (e.g., via find_package(unicode_bridge CONFIG REQUIRED)),
  - or build this project as a top-level project.")
    endif()
  endif()
endif()

# Show the user the status of the core variables.
message(STATUS "print_and_parse - Top level project: ${PNP_TOP_LEVEL_PROJECT}")
message(STATUS "print_and_parse - Building tests: ${PNP_ENABLE_TESTS}")
message(STATUS "print_and_parse - Building docs: ${PNP_ENABLE_DOCS}")
message(STATUS "print_and_parse - Building examples: ${PNP_ENABLE_EXAMPLES}")
  
# Add the library folder.
add_subdirectory(library)

# Enable documentation if required.
#if(PNP_ENABLE_DOCS)
#  find_package(Doxygen)
#  if(Doxygen_FOUND)
#    add_subdirectory(docs)
#  else()
#    message(STATUS "Doxygen not found, skipping documentation creation.")
#  endif()
#endif()

# Enable tests if required
if(PNP_ENABLE_TESTS)
  add_subdirectory(tests)
endif()

# Enable examples if required
if(PNP_ENABLE_EXAMPLES)
#  add_subdirectory(examples)
endif()
