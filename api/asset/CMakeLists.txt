v3d_add_api_library(asset
"Asset.h" "Asset.cpp"
"Cache.h" "Cache.cpp"
"kind/Image.h" "kind/Image.cpp"
"kind/Json.h" "kind/Json.cpp"
"kind/JsonFile.h" "kind/JsonFile.cpp"
"Writer.h" "Writer.cpp"
"Loader.h" "Loader.cpp"
"Manager.h" "Manager.cpp"
"Type.h"
"kind/Model.h" "kind/Model.cpp"
"loader/Gltf.h" "loader/Gltf.cpp"
"loader/CgltfImpl.cpp"
"loader/Jpeg.h" "loader/Jpeg.cpp"
"loader/Json.h" "loader/Json.cpp"
"loader/Png.h" "loader/Png.cpp"
"loader/Tga.h" "loader/Tga.cpp"
 "loader/Wav.h" "loader/Wav.cpp"
 "kind/Sound.h" "kind/Sound.cpp"
 "kind/Text.h" "kind/Text.cpp"
 "loader/Text.h" "loader/Text.cpp"
 "kind/Font2D.h" "kind/Font2D.cpp"
"loader/Font2D.h" "loader/Font2D.cpp" "loader/TextureFont.h" "loader/TextureFont.cpp" "kind/TextureFont.h" "kind/TextureFont.cpp")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# v3dlib_audio: the Wav loader calls v3d::audio::AudioClip::load, so an app that links this
# library needs the audio one whether or not it plays a sound.
# v3dlib_font and v3dlib_image: Font2D.h and Image.h name the types those libraries
# define, and the loaders build them.
# v3dlib_type: Model.h names v3d::type::Model, which the gltf loader builds.
target_link_libraries(v3dlib_asset PUBLIC v3dlib_log v3dlib_audio v3dlib_font v3dlib_image v3dlib_type)

# PUBLIC: Manager.h names a boost::filesystem::path, and Json.h, JsonFile.h and Text.h each
# name a boost::json type.
target_link_libraries(v3dlib_asset PUBLIC Boost::filesystem Boost::json)

# PRIVATE: only the gltf loader and the translation unit owning cgltf's implementation
# include the header, and neither api/asset header names a cgltf type.
target_include_directories(v3dlib_asset PRIVATE "${V3D_CGLTF_INCLUDE_DIR}")

# cgltf is a third party header compiled here rather than vendored into the tree, and its
# implementation half is not this repository's to hold to /W4 or to either analyser -
# a local fix is lost the next time the port moves. The pragma in the file drops the
# warning level, /analyze- takes it out of MSVC's analysis and SKIP_LINTING out of
# clang-tidy's.
set_source_files_properties("loader/CgltfImpl.cpp" PROPERTIES
	COMPILE_OPTIONS "/analyze-"
	SKIP_LINTING TRUE)

target_compile_options(v3dlib_asset PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
