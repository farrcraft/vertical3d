# The libraries to build and the order to add them in are both the root's, per ADR-0033:
# V3D_API_BUILD is the closure of what the consumer asked for, alphabetical. Order does not
# matter to CMake here - target_link_libraries names a target that a later subdirectory
# defines and resolves it at generate time - which is what lets this be a plain loop.
foreach(v3d_library IN LISTS V3D_API_BUILD)
	add_subdirectory("${V3D_API_${v3d_library}_PATH}")
endforeach()

# The manifest the root selected from, against the link graph the libraries just declared.
v3d_api_verify_manifest()
