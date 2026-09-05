add_executable(moya "moya.cxx")

target_link_libraries(moya
	PRIVATE
	v3dlib_moya
	v3dlib_type
	v3dlib_render_offline
	${Boost_LIBRARIES}
	${PNG_LIBRARIES}
	Boost::program_options
	JPEG::JPEG
)

# The driver resolves --file against the working directory, so the demo scene has to sit
# beside the executable to be reachable from the build tree.
v3d_add_app_data(moya)
