add_executable(imagetool "imagetool.cxx")

# Boost::program_options is named here rather than reached through a library: imagetool
# parses its own command line. The winapi version workaround arrives with v3dlib_image.
target_link_libraries(imagetool PRIVATE
		v3dlib_log
		v3dlib_image
		Boost::program_options
)

target_compile_options(imagetool PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")
