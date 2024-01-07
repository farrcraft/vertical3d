add_executable(moya "moya.cxx")

target_link_libraries(moya
	PRIVATE
	libmoya
	v3dlib_type
	${Boost_LIBRARIES}
	${PNG_LIBRARIES}
	Boost::program_options
	JPEG::JPEG
)