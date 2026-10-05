# The root decides which libraries to build. V3D_API_BUILD is the closure of what the
# consumer asked for, in alphabetical order. The order of add_subdirectory does not matter:
# target_link_libraries can name a target that a later subdirectory defines, because CMake
# resolves it at generate time.
foreach(v3d_library IN LISTS V3D_API_BUILD)
	add_subdirectory("${V3D_API_${v3d_library}_PATH}")
endforeach()

# The manifest the root selected from, against the link graph the libraries just declared.
v3d_api_verify_manifest()

# Check each library's PUBLIC links against what its headers include.
v3d_api_verify_visibility("${CMAKE_CURRENT_SOURCE_DIR}")
