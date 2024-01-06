add_library("v3dlib_engine" 
	"Feature.h"
	"Engine.h" "Engine.cpp"
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_engine PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_engine PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_engine PUBLIC WINVER=0x0A00)
