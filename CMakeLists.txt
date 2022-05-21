# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("monorepo")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Need to set this for the COMPONENTS stanza to work next
set(Boost_USE_STATIC_LIBS ON)

list(APPEND CMAKE_PREFIX_PATH "C:/Users/sigsegv/source/repos/Odyssey/vendor/boost/boost_1_79_0")

# CMake already comes with a module for boost - https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost 1.76.0 REQUIRED COMPONENTS log)

include_directories(SYSTEM
	${Boost_INCLUDE_DIRS}
	"vendor/glm-0.9.9.8/glm"
	"vendor/OpenAL 1.1 SDK/include"
	"vendor/freealut-freealut_1_1_0/include"
	"vendor/glew-2.2.0/include"
	"vendor/freetype-2.12.1/include"
	"vendor/libpng-1.6.37"
	"vendor/jpeg-9e")

link_directories(
	${Boost_LIBRARY_DIRS})

# Include sub-projects.
add_subdirectory ("v3dlibs")
