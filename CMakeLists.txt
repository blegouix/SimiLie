# SPDX-License-Identifier: GPL-3.0

cmake_minimum_required(VERSION 3.20)
project(SimiLie C CXX)

set_property(GLOBAL PROPERTY GLOBAL_DEPENDS_NO_CYCLES ON)

## We use C++17 all around!
set(CMAKE_CXX_STANDARD 20 CACHE INTERNAL "The C++ standard whose features are requested to build this project.")

# Set default DDC options when included
option(DDC_BUILD_BENCHMARKS       "Build DDC benchmarks." OFF)
option(DDC_BUILD_DOCUMENTATION    "Build DDC documentation/website" OFF)
option(DDC_BUILD_EXAMPLES         "Build DDC examples" OFF)
option(DDC_BUILD_TESTS            "Build DDC tests if BUILD_TESTING is enabled" OFF)

# Our dependencies

## Use CTest for running tests
include(CTest)

## if tests are enabled, use googletest from `vendor/`
### we use it to write unit tests
if("${BUILD_TESTING}")
  set(SIMILIE_GTest_DEPENDENCY_POLICY "AUTO" CACHE STRING "Policy to find the `GTest` package. Options: ${SIMILIE_DEPENDENCY_POLICIES}")
  set_property(CACHE SIMILIE_GTest_DEPENDENCY_POLICY PROPERTY STRINGS ${SIMILIE_DEPENDENCY_POLICIES})

  if("${SIMILIE_GTest_DEPENDENCY_POLICY}" STREQUAL "AUTO")
    find_package(GTest "1.15" QUIET)
    if(NOT "${GTest_FOUND}")
      add_subdirectory("vendor/googletest/" "googletest") # SYSTEM)
    endif()
  elseif("${SIMILIE_GTest_DEPENDENCY_POLICY}" STREQUAL "EMBEDDED")
    add_subdirectory("vendor/googletest/" "googletest") # SYSTEM)
  elseif("${SIMILIE_GTest_DEPENDENCY_POLICY}" STREQUAL "INSTALLED")
    find_package(GTest "1.15" REQUIRED)
  endif()
endif()

## Use the discrete domain computation library (ddc) from `vendor/`
add_subdirectory("vendor/ddc/" "ddc") # SYSTEM)


###############################################################################################
#                              Build libraries and executables
###############################################################################################
# Our project code

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)
