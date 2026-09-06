# Every third party package the tree uses, resolved for the api libraries actually being
# built rather than for all of them. Which packages those are is what
# v3dApiLibraries.cmake computes; see ADR-0033.
#
# Separate from v3dHelpers.cmake so that including the helper functions does not re-run the
# resolution: the two are needed independently, and a package search that runs twice in one
# configure is a search that can disagree with itself.
#
# glm and EnTT are named here even though nothing calls find_package for them historically.
# They resolved by accident: Boost_INCLUDE_DIRS is the vcpkg installed include directory,
# which also holds every header-only port, so the root's global include_directories made
# them reachable. A library that carries its own dependencies cannot rely on that.

# Boost is the one package resolved whatever is selected: v3d_add_api_library links
# Boost::headers into every api library, so no selection avoids it. The components are all
# found in one call because that is what FindBoost does - a second call naming a different
# component set re-searches rather than adding to the first.

# Must precede find_package(Boost) - the COMPONENTS stanza reads it.
set(Boost_USE_STATIC_LIBS ON)

# Don't emit warnings if boost version is newer than the cmake FindBoost definition
set(Boost_NO_WARN_NEW_VERSIONS ON)

# CMake already comes with a module for boost - https://cmake.org/cmake/help/latest/module/FindBoost.html
find_package(Boost 1.76.0 REQUIRED COMPONENTS filesystem json locale log program_options unit_test_framework)

# Resolve the named packages.
#
# A macro rather than a function, and deliberately: find_package creates its imported targets
# in the scope it runs in, and a function scope disappears when it returns. CMake 3.24 added
# find_package(... GLOBAL) for exactly this, but the tree requires 3.21. Called from the root
# so that the targets are visible to the apps as well as to api/.
macro(v3d_find_packages)
	foreach(v3d_package IN ITEMS ${ARGN})
		if(v3d_package STREQUAL "cgltf")
			# a single header with no CMake config of its own, so there is no target to link
			# and the header has to be found by hand. api/asset puts this on its own include
			# path rather than the tree relying on the vcpkg include directory being globally
			# reachable, which is what ADR-0027 took away.
			find_path(V3D_CGLTF_INCLUDE_DIR NAMES "cgltf.h" REQUIRED)
		elseif(v3d_package STREQUAL "JPEG")
			find_package(JPEG REQUIRED)
			# vcpkg's jpeg port is libjpeg-turbo, and its config is what carries the import
			# libraries FindJPEG's variables name.
			find_package(libjpeg-turbo CONFIG REQUIRED)
		elseif(v3d_package STREQUAL "SDL3_mixer")
			# The mixer is what api/audio is built on, per ADR-0021.
			find_package(SDL3_mixer CONFIG REQUIRED)
		elseif(v3d_package MATCHES "^(glm|EnTT|spdlog)$")
			find_package(${v3d_package} CONFIG REQUIRED)
		else()
			find_package(${v3d_package} REQUIRED)
		endif()
	endforeach()
endmacro()
