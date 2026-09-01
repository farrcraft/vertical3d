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
find_package(Boost 1.76.0 REQUIRED COMPONENTS json locale log program_options)

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