# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
#set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required (VERSION 3.8)

project ("vertical3d")

# This gets us C++17 and newer language features, e.g. nested namespace support - https://docs.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/compiler-error-c2429?view=msvc-160
#add_compile_options("/std:c++latest")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Need to set this for the COMPONENTS stanza to work next
set(Boost_USE_STATIC_LIBS ON)

# I haven't figured out how to make this work with a relative path
set(CMAKE_MODULE_PATH "Y:/Josh/Code/vertical3d/odyssey/cmake")

list(APPEND CMAKE_PREFIX_PATH "Y:/Josh/Code/vertical3d/vendor/boost/boost_1_79_0")

# This path is used by FindSDL2 module. 
# It will find include and lib folders under this directory.
# This is because find command doesn't include any path for Windows.
# So it can't find anything using default paths.
# SDL2_PATH is already expected in module file. I'm using the same variable.
set(SDL2_PATH "vendor/sdl2/SDL2-2.0.14")

# CMake parses module files by their names. It will use FindSDL2 in this case.
find_package(SDL2 REQUIRED)

# SDL2_INCLUDE_DIR variable set by the FindSDL2 module (if it finds directory).
include_directories(SYSTEM ${SDL2_INCLUDE_DIR})

# CMake already comes with a module for boost - https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost 1.76.0 REQUIRED COMPONENTS log)

include_directories(SYSTEM
	${Boost_INCLUDE_DIRS}
	"vendor/entt-3.10.1/src"
	"vendor/glm-0.9.9.8/glm"
	"vendor/OpenAL 1.1 SDK/include"
	"vendor/freealut-freealut_1_1_0/include"
	"vendor/glew-2.2.0/include"
	"vendor/freetype-2.12.1/include"
	"vendor/libpng-1.6.37"
	"vendor/jpeg-9e")

link_directories(
	${Boost_LIBRARY_DIRS}
	"vendor/libjpeg-turbo-2.1.0"
	"vendor/libpng-1.6.37"
	"vendor/zlib-1.2.11")

# Include sub-projects.
add_subdirectory ("v3dlibs")
add_subdirectory("luxa")
add_subdirectory("stark")
add_subdirectory("talyn")
add_subdirectory("moya")
add_subdirectory("v3dshell")
add_subdirectory("pong")
add_subdirectory("tetris")
add_subdirectory("voxel")
add_subdirectory("odyssey")