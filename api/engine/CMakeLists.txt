add_library("v3dlib_engine" 
	"Feature.h"
	"Engine.h" "Engine.cpp"
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_engine PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_engine PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_engine PUBLIC WINVER=0x0A00)

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_engine PUBLIC v3dlib_log)

target_compile_options(v3dlib_engine PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

add_subdirectory("tests")
