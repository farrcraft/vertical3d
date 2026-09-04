add_library("v3dlib_config"
	"Config.h" "Config.cpp"
 "Sounds.h" "Type.h")

target_compile_definitions(v3dlib_config PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_config PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_config PUBLIC WINVER=0x0A00)

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_config PUBLIC v3dlib_log)

target_compile_options(v3dlib_config PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

add_subdirectory("tests")
