add_library("v3dlib_asset"
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

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_asset PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_asset PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_asset PUBLIC WINVER=0x0A00)

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# v3dlib_audio: the Wav loader calls v3d::audio::AudioClip::load, so an app that links this
# library needs the audio one whether or not it plays a sound.
target_link_libraries(v3dlib_asset PUBLIC v3dlib_log v3dlib_audio)

target_compile_options(v3dlib_asset PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

add_subdirectory("tests")
