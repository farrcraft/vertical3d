add_executable(imagetool "imagetool.cxx")

target_link_libraries(imagetool
		v3dlib_image
		${Boost_LIBRARIES}
		"turbojpeg-static" "jpeg-static"
		"libpng16d" "zlibd")