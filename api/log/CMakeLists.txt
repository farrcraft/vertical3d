add_library("v3dlib_log"
	"Logger.h" "Logger.cpp"
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_log PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_log PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_log PUBLIC WINVER=0x0A00)

target_compile_options(v3dlib_log PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")
