# A windowed application outside the vertical3d tree; see docs/api/UsingTheApi.md. CI
# configures it to check that each api target carries its own include path and dependencies.
# Inside the tree that cannot be checked, because the root's global settings and the apps that
# name every library always supply them.
#
# It is a root project of its own rather than a subdirectory of vertical3d, so
# CMAKE_SOURCE_DIR differs from V3D_ROOT.
#
# Its vcpkg.json and vcpkg-configuration.json are copies of the tree's, as a real consumer
# repository carries. CI does not install from them - it points the configure at the
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

# The three libraries this app links. V3D_LIBRARIES selects which api libraries are built: the
# named ones and everything they depend on, and only their packages are looked for. Naming them
# rather than leaving V3D_LIBRARIES at "all" makes CI check that closure: an api library that
# does not declare something it links fails this configure and no other.
set(V3D_LIBRARIES engine log render)
get_filename_component(V3D_TREE "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_subdirectory("${V3D_TREE}" v3d)

add_executable(v3dstarter
	"src/AppEngine.h" "src/AppEngine.cxx"
	"src/main.cxx")

# A CMake function is global once defined, so the tree's data helpers are callable here. This
# one copies data/ beside the executable, because the engine resolves every asset relative to
# the exe. v3d_add_shared_data(v3dstarter) would add the tree's own data/ - the fonts and themes
# - which this app does not draw and so does not ask for.
v3d_add_app_data(v3dstarter)

# Three targets and no third party package of its own. boost::filesystem is compiled by these
# sources - asset/Manager.h names a path - but it arrives through v3d::asset, so nothing here
# has to name it. A consumer that does name a package itself has to find_package it in its own
# scope.
target_link_libraries(v3dstarter PRIVATE
	v3d::engine
	v3d::log
	v3d::render)
