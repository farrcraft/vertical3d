v3d_add_api_library(config
	"Config.h" "Config.cpp"
	"CameraProfiles.h" "CameraProfiles.cpp"
	"SpriteRegion.h" "SpriteRegion.cpp"
	"SpriteSheet.h" "SpriteSheet.cpp"
	"SpriteSheets.h" "SpriteSheets.cpp"
	"WindowSize.h" "WindowSize.cpp"
	"Type.h")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# v3dlib_asset: Config.h names an asset::Json, and load() resolves each file it lists
# through the asset manager.
# v3dlib_type: CameraProfiles.h holds a table of type::camera::Profile and hands them out by
# value.
# glm: SpriteSheet.h hands a region out as a uv pair, and WindowSize.h a size as an ivec2.
# Boost::json: CameraProfiles.h, SpriteSheets.h and WindowSize.h each read a boost::json::object.
target_link_libraries(v3dlib_config PUBLIC v3dlib_log v3dlib_asset v3dlib_type glm::glm Boost::json)

target_compile_options(v3dlib_config PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
