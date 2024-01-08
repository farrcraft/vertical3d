add_executable(moya "moya.cxx")

target_link_libraries(moya
	PRIVATE
	v3dlib_moya
	v3dlib_type
	${Boost_LIBRARIES}
	${PNG_LIBRARIES}
	Boost::program_options
	JPEG::JPEG
)