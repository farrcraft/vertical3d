add_library("v3dlib_config"
	"Config.h" "Config.cpp"
	"BindingContext.h" "BindingContext.cpp"
 "Sounds.h" "Type.h")

target_compile_definitions(v3dlib_config PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_config PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_config PUBLIC WINVER=0x0A00)
