v3d_add_api_library(asset_media
	"Loaders.h" "Loaders.cpp"
	"kind/Image.h" "kind/Image.cpp"
	"kind/Model.h" "kind/Model.cpp"
	"loader/CgltfImpl.cpp"
	"loader/Gltf.h" "loader/Gltf.cpp"
	"loader/Image.h" "loader/Image.cpp")

# PUBLIC: the kinds derive from asset::Asset and the loaders from asset::Loader, and each kind
# names the payload it carries - an image::Image or a type::Model.
target_link_libraries(v3dlib_asset_media PUBLIC v3dlib_log v3dlib_asset v3dlib_image v3dlib_type)

# PRIVATE: the gltf loader builds its transforms with glm, and no header here includes it.
target_link_libraries(v3dlib_asset_media PRIVATE glm::glm)

# PRIVATE: only the gltf loader and the translation unit owning cgltf's implementation
# include the header, and no header here names a cgltf type.
target_include_directories(v3dlib_asset_media PRIVATE "${V3D_CGLTF_INCLUDE_DIR}")

# cgltf is a third party header compiled here rather than vendored into the tree. Its
# implementation half is not held to /W4 or to either analyser, because a local fix would be
# lost the next time the port updates. The pragma in the file drops the
# warning level, /analyze- takes it out of MSVC's analysis and SKIP_LINTING out of
# clang-tidy's.
set_source_files_properties("loader/CgltfImpl.cpp" PROPERTIES
	COMPILE_OPTIONS "/analyze-"
	SKIP_LINTING TRUE)

target_compile_options(v3dlib_asset_media PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
