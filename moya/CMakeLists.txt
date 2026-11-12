add_subdirectory("libmoya")
add_subdirectory("tools/rgbtable")
add_subdirectory("bench")
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

# moya --hits traces through the CPU device, which loads its libraries from beside moya.
v3d_moya_cpu_libraries(moya)

# moya reads materials against MaterialX's standard libraries, which it finds beside itself.
v3d_moya_materialx_libraries(moya)

# The icon is drawn by imagetool's appicon, and a release installs every app together, so a
# build of moya without the other apps has neither.
if(V3D_BUILD_APPS)
	v3d_add_app_icon(moya "icon.svg")
	v3d_install_app(moya "moya")
	foreach(isa "sse42" "avx2" "avx512")
		install(TARGETS "moya_cpu_${isa}" RUNTIME DESTINATION "${V3D_INSTALL_APP_DIR}" LIBRARY DESTINATION "${V3D_INSTALL_APP_DIR}")
	endforeach()
	install(DIRECTORY "${CMAKE_BINARY_DIR}/materialx/" DESTINATION "${V3D_INSTALL_APP_DIR}/materialx")
endif()
