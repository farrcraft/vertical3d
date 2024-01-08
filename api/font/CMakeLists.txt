add_library("v3dlib_font" "BitmapFont.h" "BitmapFont.cxx" "BitmapTextBuffer.h" "BitmapTextBuffer.cxx" "Font2D.h" "Font2D.cxx"
	  "TextBuffer.h" "TextBuffer.cxx" "TextureFont.h" "TextureFont.cxx" "TextureFontCache.h" "TextureFontCache.cxx"
	"TextureTextBuffer.h" "TextureTextBuffer.cxx")


# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_font PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_font PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_font PUBLIC WINVER=0x0A00)
