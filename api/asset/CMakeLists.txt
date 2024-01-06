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
 "loader/Wav.h" "loader/Wav.cpp" 
 "Sound.h" "Sound.cpp" 
 "Text.h" "Text.cpp"
 "loader/Text.h" "loader/Text.cpp"
 "Shader.h" "Shader.cpp" 
 "loader/Shader.h" "loader/Shader.cpp" 
 "ShaderProgram.h" "ShaderProgram.cpp" 
 "loader/ShaderProgram.h" "loader/ShaderProgram.cpp" 
 "Font2D.h" "Font2D.cpp"
"loader/Font2D.h" "loader/Font2D.cpp" "loader/TextureFont.h" "loader/TextureFont.cpp" "TextureFont.h" "TextureFont.cpp")

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_asset PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_asset PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_asset PUBLIC WINVER=0x0A00)