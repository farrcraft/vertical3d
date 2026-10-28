v3d_add_api_library(asset_media
	"Loaders.h" "Loaders.cpp"
	"SpriteRegion.h" "SpriteRegion.cpp"
	"SpriteSheet.h" "SpriteSheet.cpp"
	"SpriteSheets.h" "SpriteSheets.cpp"
	"kind/Image.h" "kind/Image.cpp"
	"kind/Model.h" "kind/Model.cpp"
	"loader/Gltf.h" "loader/Gltf.cpp"
	"loader/Image.h" "loader/Image.cpp")

# PUBLIC: the kinds derive from asset::Asset and the loaders from asset::Loader, and each kind
# names the payload it carries - an image::Image or a model::Model. glm, because SpriteSheet.h
# hands out a region as a uv pair.
target_link_libraries(v3dlib_asset_media PUBLIC v3dlib_log v3dlib_asset v3dlib_image v3dlib_model glm::glm)

# PRIVATE: the gltf loader builds the skeleton and the clips, and no header here names them
# apart from the model that holds them.
target_link_libraries(v3dlib_asset_media PRIVATE v3dlib_animation)

# PRIVATE: only the gltf loader includes the header, and no header here names a cgltf type.
# cgltf's implementation is compiled once for every program, in interchange, which this
# library links for it.
target_include_directories(v3dlib_asset_media PRIVATE "${V3D_CGLTF_INCLUDE_DIR}")
target_link_libraries(v3dlib_asset_media PRIVATE v3dlib_interchange)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
