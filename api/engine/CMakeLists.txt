add_library("v3dlib_engine" 
	"Feature.h"
	"Engine.h" "Engine.cpp"
)

target_compile_definitions(v3dlib_engine PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_engine PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_engine PUBLIC WINVER=0x0A00)
