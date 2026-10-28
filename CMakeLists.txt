# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required (VERSION 3.21)
cmake_policy(SET CMP0079 NEW)

project ("vertical3d")

# The root of this repository. Not CMAKE_SOURCE_DIR, which is the consumer's root once
# another project adds this one with add_subdirectory. Everything naming a file in this
# tree goes through this variable instead.
set(V3D_ROOT "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "The root of the vertical3d tree")

# What a consumer of the api libraries does not want built. Both default on for a build of
# this repository and off for one that has nested it.
option(V3D_BUILD_APPS "Build the applications as well as the api libraries" ${PROJECT_IS_TOP_LEVEL})
option(V3D_BUILD_TESTS "Build the Boost.Test suites and register them with ctest" ${PROJECT_IS_TOP_LEVEL})

# Which api libraries to build. A consumer names the ones it links, and gets those and the
# libraries they depend on. Nothing else is added, and no package outside that set is looked
# for, so an app that uses only v3d::image does not need the Vulkan SDK. The manifest that
# expands this list is cmake/v3dApiLibraries.cmake.
set(V3D_LIBRARIES "all" CACHE STRING "The api libraries to build, or \"all\"")

# C++23, which CMake gives MSVC as /std:c++latest. Set the standard here, not the flag. glm
# and EnTT require cxx_std_17, and MSVC reports D9025 when a command line names two standards.
# With the standard set through CMake, the command line names only the newer one.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Every flag is spelled for the compiler that reads it. MSVC's spelling is a file name to Clang
# and GCC, so a flag given to every compiler stops every compiler but MSVC. Each MSVC flag has
# the flag that means the same to Clang and GCC beside it, or a note that they need none.
# clang-cl reads MSVC's spellings, and CMake sets MSVC for it too.
if(MSVC)
	# standard conformance. Clang and GCC conform by default
	add_compile_options("/permissive-")

	# CMake seeds CMAKE_CXX_FLAGS with /W3, and MSVC reports D9025 for a command line that
	# names two warning levels, so the default has to come out before /W4 goes in.
	string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	add_compile_options("/W4")

	# C4062 (an enumerator a switch does not handle) is off at /W4, so turn it on. A switch
	# over an enum lists every enumerator and has no default label. When an enumerator is
	# added, the warning names every switch that has to handle it. A switch with a default
	# label is not reported.
	add_compile_options("/w14062")
else()
	# the warnings /W4 asks MSVC for. -Wall carries -Wswitch, which is C4062
	add_compile_options("-Wall" "-Wextra")
endif()

# The static analysis gates. /WX is on only when this project is the top level one. A
# consumer that nests this tree builds it with its own toolset, and a warning that a newer
# compiler adds must not stop that consumer's build.
#
# The two analysers are off by default even here. Each takes several times as long as a plain
# compile of the tree, so they are run over a finished change rather than on every build.
option(V3D_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ${PROJECT_IS_TOP_LEVEL})
option(V3D_ANALYZE "Run MSVC's /analyze static analysis with the compiler" OFF)
option(V3D_CLANG_TIDY "Run clang-tidy over every source the compiler builds" OFF)

if(V3D_WARNINGS_AS_ERRORS)
	if(MSVC)
		add_compile_options("/WX")
	else()
		add_compile_options("-Werror")
	endif()
endif()

# /analyze's findings are the C6xxx and C26xxx numbers, and they reach /WX like any other
# warning. External headers are skipped: CMake gives an imported target's include directories
# to MSVC as /external:I, and /analyze:external- stops the analyser walking into them. Without
# it, a boost or glm header costs more analysis time than the whole tree, and the analyser
# reports findings in code this repository does not own. /analyze:external- is not the
# default, so it must be stated.
if(V3D_ANALYZE)
	if(NOT MSVC)
		message(FATAL_ERROR "V3D_ANALYZE runs MSVC's /analyze, which only MSVC has. Use V3D_CLANG_TIDY with another compiler.")
	endif()
	add_compile_options("/analyze" "/analyze:external-")
endif()

# The check list is .clang-tidy at the root of the tree. CMAKE_CXX_CLANG_TIDY is read when a
# target is created, so this has to precede the add_subdirectory calls below. CMake passes
# --driver-mode=cl for an MSVC build itself.
#
# /EHsc is repeated to clang-tidy rather than left to the compile command it is already on.
# CMake writes a system include directory as the joined -external:I<dir>, but clang-cl accepts
# only the separated form. clang-tidy silently drops that option and every option after it, so
# the /EHsc on the command line never reaches the parser. Every source that throws then fails
# with "cannot use 'throw' with exceptions disabled". An --extra-arg-before is placed ahead of
# the command, which is the only position that survives.
if(V3D_CLANG_TIDY)
	find_program(V3D_CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
	set(CMAKE_CXX_CLANG_TIDY "${V3D_CLANG_TIDY_EXECUTABLE}" "--quiet")
	# only for MSVC's command lines. Clang and GCC enable exceptions by default, and drop
	# nothing from theirs
	if(MSVC)
		list(APPEND CMAKE_CXX_CLANG_TIDY "--extra-arg-before=/EHsc")
	endif()
	# One switch decides whether a finding stops the build, whichever analyser made it.
	# Without this, clang-tidy prints a finding and the build carries on.
	if(V3D_WARNINGS_AS_ERRORS)
		list(APPEND CMAKE_CXX_CLANG_TIDY "--warnings-as-errors=*")
	endif()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cmake/v3dApiLibraries.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/v3dDependencies.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/v3dHelpers.cmake")

# The apps between them link every api library, directly or through another, so they need the
# whole api. A consumer that narrows the selection and leaves the apps on has asked for two
# things that contradict, and configure fails. The tests do not: each library's suite links
# only that library's closure, so a narrowed selection with the tests on builds the suites of
# the libraries it selected.
if(V3D_BUILD_APPS AND NOT V3D_LIBRARIES STREQUAL "all")
	message(FATAL_ERROR
		"V3D_LIBRARIES names a subset, but V3D_BUILD_APPS is on and the apps need the whole "
		"api. Set it OFF, or leave V3D_LIBRARIES as \"all\".")
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
	add_subdirectory("moya")
	add_subdirectory("demos")
	add_subdirectory("vertical3d")
	add_subdirectory("imagetool")
endif()
