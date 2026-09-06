v3d_add_api_library(asset
"Asset.h" "Asset.cpp"
"Cache.h" "Cache.cpp"
"Image.h" "Image.cpp"
"Json.h" "Json.cpp"
"JsonFile.h" "JsonFile.cpp"
"Loader.h" "Loader.cpp"
"Manager.h" "Manager.cpp"
"Type.h"
"loader/Jpeg.h" "loader/Jpeg.cpp"
"loader/Json.h" "loader/Json.cpp"
"loader/Png.h" "loader/Png.cpp"
"loader/Tga.h" "loader/Tga.cpp"
 "loader/Wav.h" "loader/Wav.cpp"
 "Sound.h" "Sound.cpp"
 "Text.h" "Text.cpp"
 "loader/Text.h" "loader/Text.cpp"
 "Font2D.h" "Font2D.cpp"
"loader/Font2D.h" "loader/Font2D.cpp" "loader/TextureFont.h" "loader/TextureFont.cpp" "TextureFont.h" "TextureFont.cpp")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# v3dlib_audio: the Wav loader calls v3d::audio::AudioClip::load, so an app that links this
# library needs the audio one whether or not it plays a sound.
# v3dlib_font and v3dlib_image: Font2D.h and Image.h name the types those libraries
# define, and the loaders build them.
target_link_libraries(v3dlib_asset PUBLIC v3dlib_log v3dlib_audio v3dlib_font v3dlib_image)

# PUBLIC: Manager.h names a boost::filesystem::path, and Json.h, JsonFile.h and Text.h each
# name a boost::json type.
target_link_libraries(v3dlib_asset PUBLIC Boost::filesystem Boost::json)

target_compile_options(v3dlib_asset PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
