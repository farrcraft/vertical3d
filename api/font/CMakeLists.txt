v3d_add_api_library(font "TextBuffer.h" "TextBuffer.cxx" "TextureFont.h" "TextureFont.cxx" "TextureFontCache.h" "TextureFontCache.cxx"
	"TextureTextBuffer.h" "TextureTextBuffer.cxx"
	"Utf8.h" "Utf8.cxx"
	"internal/Freetype.h" "internal/Freetype.cxx"
	"internal/GlyphBitmap.h" "internal/GlyphHandle.h" "internal/StrokerHandle.h")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_font PUBLIC v3dlib_log v3dlib_image glm::glm)
# PRIVATE: TextureFont.h hides FreeType behind a forward-declared class. Only TextureFont.cxx and
# the internal/ headers and sources include FreeType.
target_link_libraries(v3dlib_font PRIVATE Freetype::Freetype)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
