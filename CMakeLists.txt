# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required (VERSION 3.21)
cmake_policy(SET CMP0079 NEW)

project ("vertical3d")

# The root of this repository. Not CMAKE_SOURCE_DIR, which is the consumer's root once
# another project reaches this one with add_subdirectory - see ADR-0027. Everything naming
# a file in this tree goes through this variable instead.
set(V3D_ROOT "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "The root of the vertical3d tree")

# What a consumer of the api libraries does not want built. Both default on for a build of
# this repository and off for one that has nested it.
option(V3D_BUILD_APPS "Build the applications as well as the api libraries" ${PROJECT_IS_TOP_LEVEL})
option(V3D_BUILD_TESTS "Build the Boost.Test suites and register them with ctest" ${PROJECT_IS_TOP_LEVEL})

# This gets us C++17 and newer language features, e.g. nested namespace support - https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/compiler-error-c2429?view=msvc-160
# Stated as a standard rather than as /std:c++latest directly: an imported target that
# requires cxx_std_17 - glm and EnTT both do - adds /std:c++17 to the command line, and MSVC
# reports D9025 for a command line naming two. CMake maps cxx_std_23 to /std:c++latest here,
# so the flag is unchanged and the requirement is satisfied rather than contradicted.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("/permissive-")

# CMake seeds CMAKE_CXX_FLAGS with /W3, and MSVC reports D9025 for a command line that names
# two warning levels, so the default has to come out before /W4 goes in.
string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
add_compile_options("/W4")

include("${CMAKE_CURRENT_LIST_DIR}/cmake/v3dDependencies.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/v3dHelpers.cmake")

if(V3D_BUILD_TESTS)
	enable_testing()
endif()

# Include sub-projects
add_subdirectory("api")

if(V3D_BUILD_APPS)
	add_subdirectory("talyn")
	add_subdirectory("moya")
	add_subdirectory("v3dshell")
	add_subdirectory("pong")
	add_subdirectory("tetris")
	add_subdirectory("voxel")
	add_subdirectory("odyssey")
	add_subdirectory("vertical3d")
	add_subdirectory("imagetool")
endif()
