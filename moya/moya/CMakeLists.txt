add_executable(moya "moya.cxx")

target_link_libraries(moya
		${Boost_LIBRARIES}
		"turbojpeg-static" "jpeg-static"
		"libpng16d" "zlibd")