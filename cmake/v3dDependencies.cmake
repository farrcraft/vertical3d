# Every third party package the tree uses, resolved once.
#
# Separate from v3dHelpers.cmake so that including the helper functions does not re-run the
# resolution: the two are needed independently, and a package search that runs twice in one
# configure is a search that can disagree with itself.
#
# glm and EnTT are named here even though nothing calls find_package for them historically.
# They resolved by accident: Boost_INCLUDE_DIRS is the vcpkg installed include directory,
# which also holds every header-only port, so the root's global include_directories made
# them reachable. A library that carries its own dependencies cannot rely on that.

# Must precede find_package(Boost) - the COMPONENTS stanza reads it.
set(Boost_USE_STATIC_LIBS ON)

# Don't emit warnings if boost version is newer than the cmake FindBoost definition
set(Boost_NO_WARN_NEW_VERSIONS ON)

# CMake already comes with a module for boost - https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost 1.76.0 REQUIRED COMPONENTS filesystem json locale log program_options unit_test_framework)

find_package(SDL3 REQUIRED)

# The mixer is what api/audio is built on, per ADR-0021.
find_package(SDL3_mixer CONFIG REQUIRED)

find_package(Vulkan REQUIRED)

find_package(Freetype REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# cgltf is a single header with no CMake config of its own, so there is no target to link
# and the header has to be found by hand. api/asset puts this on its own include path
# rather than the tree relying on the vcpkg include directory being globally reachable,
# which is what ADR-0027 took away.
find_path(V3D_CGLTF_INCLUDE_DIR NAMES "cgltf.h" REQUIRED)
