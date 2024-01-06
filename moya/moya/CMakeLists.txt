add_executable(moya "moya.cxx")

target_link_libraries(moya
		PRIVATE
		libmoya
		v3dlib_3dtype
		${Boost_LIBRARIES}
#		"turbojpeg-static" 
#		"jpeg-static"
#		"libpng16_static" 
#		"zlib"
	${PNG_LIBRARIES}
	Boost::program_options
	JPEG::JPEG
)