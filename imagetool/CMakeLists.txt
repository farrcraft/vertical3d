add_executable(imagetool "imagetool.cxx")

target_link_libraries(imagetool PRIVATE
		v3dlib_log
		v3dlib_image
		${Boost_LIBRARIES}
		${PNG_LIBRARIES}
		JPEG::JPEG
		fmt::fmt
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(imagetool PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(imagetool PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(imagetool PUBLIC WINVER=0x0A00)

target_compile_options(imagetool PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")
