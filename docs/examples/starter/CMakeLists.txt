# A windowed application outside the vertical3d tree, built the way
# [docs/NewProject.md](../../NewProject.md) describes. It exists to be configured by CI: nothing
# inside the tree can tell a target that carries its own include path and its own dependencies
# from one picking them up off the root's global or off an app that names all thirteen
# libraries, because in this repository both are always there.
#
# It is a root project of its own rather than a subdirectory of vertical3d, which is what makes
# CMAKE_SOURCE_DIR differ from V3D_ROOT.
#
# Its vcpkg.json and vcpkg-configuration.json are copies of the tree's, which is what a real
# consumer repository carries. CI does not install from them - it points the configure at the
# install the ctest job already made - so the copies here document the requirement rather than
# doubling the CI cost of it.

cmake_minimum_required(VERSION 3.21)

project("v3dstarter" CXX)

# What a consumer sets for itself, and all of it. The language standard and /permissive- are
# deliberately not in the api's interface properties; everything else a v3d:: target needs -
# its include root, /EHsc, /utf-8, the boost winapi version workaround, and every third party
# dependency its headers name - arrives through the target.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options("/permissive-")

# An application in its own repository reaches the tree through a submodule or FetchContent.
# The path is relative here because the example travels with the tree it consumes.
set(V3D_BUILD_APPS OFF)
set(V3D_BUILD_TESTS OFF)
get_filename_component(V3D_TREE "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_subdirectory("${V3D_TREE}" v3d)

# A package this project names itself has to be found in this scope. find_package creates
# imported targets in the directory that calls it and below, so nothing vertical3d resolved is
# visible here. After the add_subdirectory, so that Boost_USE_STATIC_LIBS is already what the
# api was resolved against.
find_package(Boost REQUIRED COMPONENTS filesystem)

add_executable(v3dstarter
	"src/AppEngine.h" "src/AppEngine.cxx"
	"src/main.cxx")

# A CMake function is global once defined, so the tree's data helpers are callable here. This
# one copies data/ beside the executable, because the engine resolves every asset relative to
# the exe. v3d_add_shared_data(v3dstarter) would add the tree's own data/ - the fonts and themes
# - which this app does not draw and so does not ask for.
v3d_add_app_data(v3dstarter)

target_link_libraries(v3dstarter PRIVATE
	v3d::engine
	v3d::log
	v3d::render
	Boost::filesystem)
