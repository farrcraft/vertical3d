# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required (VERSION 3.8)
cmake_policy(SET CMP0079 NEW)

project ("vertical3d")

# This gets us C++17 and newer language features, e.g. nested namespace support - https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/compiler-error-c2429?view=msvc-160
add_compile_options("/std:c++latest")
#set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("/permissive-")

# Need to set this for the COMPONENTS stanza to work next
set(Boost_USE_STATIC_LIBS ON)

# Don't emit warnings if boost version is newer than the cmake FindBoost definition
set(Boost_NO_WARN_NEW_VERSIONS ON)

find_package(SDL3 REQUIRED)

# SDL3_INCLUDE_DIR variable set by the FindSDL3 module (if it finds directory).
include_directories(SYSTEM ${SDL3_INCLUDE_DIR})

find_package(Vulkan REQUIRED)

# CMake already comes with a module for boost - https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost 1.76.0 REQUIRED COMPONENTS filesystem json locale log program_options unit_test_framework)

find_package(Freetype REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(EnTT)
find_package(OpenGL REQUIRED)
find_package(spdlog CONFIG REQUIRED)

include_directories(SYSTEM
	${Boost_INCLUDE_DIRS}
	${SDL3_INCLUDE_DIRS}
	${Vulkan_INCLUDE_DIRS}
	"vendor/libnoise/include/noise"
	"vendor/soloud/include"
)

link_directories(
	${Boost_LIBRARY_DIRS}
	"vendor/soloud/contrib/Debug"
	"vendor/libnoise/Debug"
)

# Assets shared by more than one app live in the root data/ directory, so there is one
# committed copy rather than one per app. v3d_add_shared_data copies them into an app's
# runtime data directory after it links, alongside whatever that app keeps in <app>/data.
# The engine resolves assets relative to the executable, so the destination has to be
# beside the exe rather than in the source tree.
function(v3d_add_shared_data target)
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CMAKE_SOURCE_DIR}/data"
			"$<TARGET_FILE_DIR:${target}>/data"
		COMMENT "Copying shared data for ${target}"
		VERBATIM)
endfunction()

# An app's own assets are committed under <app>/data. They are copied beside the executable
# after it links for the same reason the shared ones are - the engine resolves every asset
# relative to the exe - and because a build tree that is not refreshed from the source tree
# drifts silently: editing <app>/data then appears to do nothing.
function(v3d_add_app_data target)
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory
			"${CMAKE_CURRENT_SOURCE_DIR}/data"
			"$<TARGET_FILE_DIR:${target}>/data"
		COMMENT "Copying data for ${target}"
		VERBATIM)
endfunction()

# Tests live next to the library they cover, in api/<lib>/tests, and each directory builds
# one Boost.Test binary. v3d_add_test wires up what every one of them needs: the framework
# (whose imported target carries BOOST_TEST_DYN_LINK, since vcpkg builds boost shared),
# /EHsc and /utf-8 - the latter is required wherever spdlog's bundled fmt is compiled - and
# a working directory beside the executable, so a test's data files resolve the same way an
# app's do. Run them with ctest --test-dir out/build/<config>.
enable_testing()

function(v3d_add_test lib)
	set(target "v3dtest_${lib}")
	add_executable(${target} ${ARGN})
	target_link_libraries(${target} PRIVATE Boost::unit_test_framework)
	target_compile_options(${target} PRIVATE /EHsc /utf-8)
	# Boost.Test's CRT leak check reports at exit, before spdlog's global registry is torn
	# down, so any test that builds a Logger reports the same permanent false positive.
	add_test(NAME ${lib} COMMAND ${target} --detect_memory_leaks=0 WORKING_DIRECTORY $<TARGET_FILE_DIR:${target}>)
endfunction()

# Shaders are compiled to SPIR-V at build time and embedded in the library that draws with
# them, rather than shipped as data files: they belong to the engine rather than to any app,
# so no app's data directory is the right place for them. glslc's -mfmt=c writes the module
# out as a C initialiser list, which the source includes into a uint32_t array.
if(NOT Vulkan_GLSLC_EXECUTABLE)
	find_program(Vulkan_GLSLC_EXECUTABLE NAMES glslc HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
endif()
if(NOT Vulkan_GLSLC_EXECUTABLE)
	message(FATAL_ERROR "glslc was not found - it ships with the Vulkan SDK, which VULKAN_SDK should point at")
endif()

function(v3d_add_shader target source)
	get_filename_component(name ${source} NAME)
	set(output "${CMAKE_CURRENT_BINARY_DIR}/shaders/${name}.inc")
	add_custom_command(
		OUTPUT ${output}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
		COMMAND ${Vulkan_GLSLC_EXECUTABLE} --target-env=vulkan1.3 -O -mfmt=c "${CMAKE_CURRENT_SOURCE_DIR}/${source}" -o ${output}
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${source}"
		COMMENT "Compiling ${source} to SPIR-V"
		VERBATIM)
	set_source_files_properties(${output} PROPERTIES HEADER_FILE_ONLY TRUE GENERATED TRUE)
	target_sources(${target} PRIVATE ${output})
	target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endfunction()

# Include sub-projects
add_subdirectory("api")
add_subdirectory("talyn")
add_subdirectory("moya")
add_subdirectory("v3dshell")
add_subdirectory("pong")
add_subdirectory("tetris")
add_subdirectory("voxel")
add_subdirectory("odyssey")
# add_subdirectory("vertical3d")
add_subdirectory("imagetool")