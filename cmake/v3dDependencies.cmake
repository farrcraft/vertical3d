# Every third party package the tree uses, resolved for the api libraries actually being
# built rather than for all of them. v3dApiLibraries.cmake computes which packages those are.
#
# Separate from v3dHelpers.cmake so that including the helper functions does not re-run the
# resolution. A package search that runs twice in one configure can return different results.
#
# glm and EnTT are found explicitly. Their headers are also in the vcpkg include directory,
# but a library carries its own dependencies and must link them rather than rely on that
# directory being on its include path.

# Boost is the only package resolved whatever is selected: v3d_add_api_library links
# Boost::headers into every api library, so no selection avoids it. The components are all
# found in one call, because a second call naming a different component set re-searches
# rather than adding to the first.
#
# This resolves in config mode, not through FindBoost: vcpkg ships boost's own
# BoostConfig.cmake and its wrapper clears Boost_NO_BOOST_CMAKE, so FindBoost delegates and
# returns before it reads any of its own input variables. Boost_USE_STATIC_LIBS is one of
# those and has no effect here. The vcpkg triplet decides static or shared, and x64-windows is
# dynamic.
#
# The components are the compiled boost libraries the tree links, and nothing else. Each one
# is a `boost-*` port the manifest names in its own right rather than a piece of the `boost`
# metapackage. A component named here that the manifest does not install therefore fails the
# configure rather than resolving quietly.
find_package(Boost 1.76.0 REQUIRED COMPONENTS filesystem json program_options unit_test_framework)

# Resolve the named packages.
#
# A macro rather than a function: find_package creates its imported targets in the scope it
# runs in, and a function scope disappears when it returns. CMake 3.24 added
# find_package(... GLOBAL) for this, but the tree requires 3.21. Called from the root so that
# the targets are visible to the apps as well as to api/.
macro(v3d_find_packages)
	foreach(v3d_package IN ITEMS ${ARGN})
		if(v3d_package STREQUAL "cgltf")
			# a single header with no CMake config of its own, so there is no target to link
			# and the header has to be found by hand. api/asset/media puts this on its own
			# include path, because the vcpkg include directory is not on a global include path.
			find_path(V3D_CGLTF_INCLUDE_DIR NAMES "cgltf.h" REQUIRED)
		elseif(v3d_package STREQUAL "JPEG")
			find_package(JPEG REQUIRED)
			# vcpkg's jpeg port is libjpeg-turbo, and its config carries the import libraries
			# that FindJPEG's variables name.
			find_package(libjpeg-turbo CONFIG REQUIRED)
		elseif(v3d_package STREQUAL "SDL3_mixer")
			# api/audio plays sound through SDL3_mixer.
			find_package(SDL3_mixer CONFIG REQUIRED)
		elseif(v3d_package STREQUAL "VulkanMemoryAllocator")
			# memory::Allocator suballocates through VMA. Header only, and it requires the
			# include directory holding vulkan.h, which Vulkan::Vulkan already carries.
			find_package(VulkanMemoryAllocator CONFIG REQUIRED)
		elseif(v3d_package MATCHES "^(glm|EnTT|spdlog)$")
			find_package(${v3d_package} CONFIG REQUIRED)
		else()
			find_package(${v3d_package} REQUIRED)
		endif()
	endforeach()
endmacro()
