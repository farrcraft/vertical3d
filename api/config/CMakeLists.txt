v3d_add_api_library(config
	"Config.h" "Config.cpp"
 "Sounds.h" "Type.h")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# v3dlib_asset: Config.h names an asset::Json, and load() resolves each file it lists
# through the asset manager.
target_link_libraries(v3dlib_config PUBLIC v3dlib_log v3dlib_asset)

target_compile_options(v3dlib_config PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
