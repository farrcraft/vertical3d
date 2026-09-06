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

# Which api libraries to build, per ADR-0033. A consumer names the ones it links and gets
# those and whatever they need; nothing else is added and no package outside that closure is
# looked for, so an app wanting only v3d::image does not need the Vulkan SDK. The manifest
# that expands this is cmake/v3dApiLibraries.cmake.
set(V3D_LIBRARIES "all" CACHE STRING "The api libraries to build, or \"all\"")

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

# The three static analysis gates. /WX follows whether this project is the top level one: a
# consumer that has nested this tree builds it with whatever toolset it happens to have, and a
# newer one inventing a warning is not that consumer's build to stop.
#
# The two analysers are off by default even here, because neither is priced for the edit-build
# loop: each costs several times a plain compile of the tree, and they are a pass over a change
# rather than a step in building one.
option(V3D_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ${PROJECT_IS_TOP_LEVEL})
option(V3D_ANALYZE "Run MSVC's /analyze static analysis with the compiler" OFF)
option(V3D_CLANG_TIDY "Run clang-tidy over every source the compiler builds" OFF)

if(V3D_WARNINGS_AS_ERRORS)
	add_compile_options("/WX")
endif()

# /analyze's findings are the C6xxx and C26xxx numbers, and they reach /WX like any other
# warning. External headers are skipped: CMake gives an imported target's include directories
# to MSVC as /external:I, and /analyze:external- is what stops the analyser walking into them -
# without it a boost or glm header costs more analysis time than the whole tree and reports
# findings in code this repository does not own. /analyze:external- is not the default.
if(V3D_ANALYZE)
	add_compile_options("/analyze" "/analyze:external-")
endif()

# The check list is .clang-tidy at the root of the tree. CMAKE_CXX_CLANG_TIDY is read when a
# target is created, so this has to precede the add_subdirectory calls below; CMake passes
# --driver-mode=cl for an MSVC build itself.
#
# /EHsc is repeated to clang-tidy rather than left to the compile command it is already on.
# CMake writes a system include directory as the joined -external:I<dir>, which clang-cl's
# option table has as separate only; clang-tidy drops it and, silently, every option after it,
# so the /EHsc on the command line never reaches the parser and every source that throws fails
# with "cannot use 'throw' with exceptions disabled". An --extra-arg-before is applied ahead of
# the command, which is the one position that survives.
if(V3D_CLANG_TIDY)
	find_program(V3D_CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
	set(CMAKE_CXX_CLANG_TIDY "${V3D_CLANG_TIDY_EXECUTABLE}" "--quiet" "--extra-arg-before=/EHsc")
	# One switch decides whether a finding stops the build, whichever analyser made it. A
	# clang-tidy finding is otherwise printed and passed over, which is how a check list
	# nobody has to keep clean stops being one.
	if(V3D_WARNINGS_AS_ERRORS)
		list(APPEND CMAKE_CXX_CLANG_TIDY "--warnings-as-errors=*")
	endif()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cmake/v3dApiLibraries.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/v3dDependencies.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/v3dHelpers.cmake")

# The apps name every api library between them, and every tests directory is one library's,
# so either of those means all of them. Loud rather than quiet: a consumer that narrowed the
# selection and left the apps on asked for two things that contradict.
if(V3D_BUILD_APPS OR V3D_BUILD_TESTS)
	if(NOT V3D_LIBRARIES STREQUAL "all")
		message(FATAL_ERROR
			"V3D_LIBRARIES names a subset, but V3D_BUILD_APPS or V3D_BUILD_TESTS is on and "
			"both need the whole api. Set them OFF, or leave V3D_LIBRARIES as \"all\".")
	endif()
endif()

if(V3D_LIBRARIES STREQUAL "all")
	set(V3D_LIBRARIES ${V3D_API_LIBRARIES})
endif()

v3d_api_closure()
v3d_find_packages(${V3D_API_PACKAGES})
list(JOIN V3D_API_BUILD " " v3d_summary)
message(STATUS "vertical3d api: ${v3d_summary}")
list(JOIN V3D_API_PACKAGES " " v3d_summary)
message(STATUS "vertical3d packages: Boost ${v3d_summary}")

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
