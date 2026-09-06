v3d_add_api_library(font "BitmapFont.h" "BitmapFont.cxx" "BitmapTextBuffer.h" "BitmapTextBuffer.cxx" "Font2D.h" "Font2D.cxx"
	  "TextBuffer.h" "TextBuffer.cxx" "TextureFont.h" "TextureFont.cxx" "TextureFontCache.h" "TextureFontCache.cxx"
	"TextureTextBuffer.h" "TextureTextBuffer.cxx")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# Freetype: TextureFont.h names FT_Library and FT_Face.
target_link_libraries(v3dlib_font PUBLIC v3dlib_log v3dlib_image Freetype::Freetype glm::glm)

# PRIVATE: the atlas is bounded with a type::Bound2D, which no font header names.
target_link_libraries(v3dlib_font PRIVATE v3dlib_type)

target_compile_options(v3dlib_font PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
