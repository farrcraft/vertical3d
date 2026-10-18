# A windowed application outside the vertical3d tree, built against it as a consumer would. CI
# configures it to check that each api target carries its own include path and dependencies.
# Inside the tree that cannot be checked, because the root's global settings and the apps that
# name every library always supply them.
#
# It is a root project of its own rather than a subdirectory of vertical3d, so
# CMAKE_SOURCE_DIR differs from V3D_ROOT.
#
# Its vcpkg.json and vcpkg-configuration.json are copies of the tree's, as a consumer
# repository would carry. CI does not install from them. It points the configure at the vcpkg
# install the ctest job has already made, so the copies only document what a consumer needs.

cmake_minimum_required(VERSION 3.21)

project("v3dstarter" CXX)

# The only settings a consumer makes for itself. The api's interface properties leave out the
# language standard and /permissive-. Everything else a v3d:: target needs comes through the
# target: its include root, /EHsc, /utf-8, and every third party dependency its headers name.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options("/permissive-")

# An application in its own repository reaches the tree through a submodule or FetchContent.
# The path is relative here because the example travels with the tree it consumes.
set(V3D_BUILD_APPS OFF)
set(V3D_BUILD_TESTS OFF)

# The libraries this app links. V3D_LIBRARIES selects which api libraries are built: the named
# ones and everything they depend on. Only their packages are looked for. Because the list is
# not "all", CI checks that closure here: an api library that links something it does not
# declare fails this configure.
set(V3D_LIBRARIES engine log render)
get_filename_component(V3D_TREE "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_subdirectory("${V3D_TREE}" v3d)

add_executable(v3dstarter
	"src/AppEngine.h" "src/AppEngine.cxx"
	"src/main.cxx")

# A CMake function is global once defined, so the tree's data helpers are callable here. This
# one copies data/ beside the executable, because the engine resolves every asset relative to
# the exe. v3d_add_shared_data(v3dstarter) would also copy the tree's own data/, which holds
# the shared font. This app draws no text, so it does not call it.
v3d_add_app_data(v3dstarter)

# The app names v3d:: targets and no third party package. These sources compile against
# boost::filesystem, because asset/Manager.h names a path. It comes through v3d::asset, so
# nothing here names it. A consumer that names a package itself has to call find_package for it
# in its own scope.
target_link_libraries(v3dstarter PRIVATE
	v3d::engine
	v3d::log
	v3d::render)
