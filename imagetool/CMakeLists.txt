add_executable(imagetool "imagetool.cxx")

target_link_libraries(imagetool PRIVATE
		v3dlib_log
		v3dlib_image
		${Boost_LIBRARIES}
		#"turbojpeg-static" 
		#"jpeg-static"
		#"libpng16_static" 
		#"zlib"
		${PNG_LIBRARIES}
		JPEG::JPEG
		Boost::log
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(imagetool PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(imagetool PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(imagetool PUBLIC WINVER=0x0A00)
