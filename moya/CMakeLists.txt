add_subdirectory("libmoya")
if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()

add_executable(moya "moya.cxx")

target_link_libraries(moya
	PRIVATE
	v3dlib_moya
	v3dlib_log
	v3dlib_render_offline
	v3dlib_version
	Boost::program_options
)

# The driver resolves --file against the working directory. The demo scenes are copied into
# data/moya/ beside the executable, so they are reachable from the build tree.
v3d_add_app_data(moya)
v3d_add_app_icon(moya "icon.svg")
v3d_install_app(moya)
