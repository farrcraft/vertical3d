add_executable(moya "moya.cxx")

target_link_libraries(moya
		libmoya
		v3dlib_3dtype
		${Boost_LIBRARIES}
		"turbojpeg-static" "jpeg-static"
		"libpng16d" "zlibd")