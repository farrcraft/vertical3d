v3d_add_api_library(font "TextBuffer.h" "TextBuffer.cxx" "TextureFont.h" "TextureFont.cxx" "TextureFontCache.h" "TextureFontCache.cxx"
	"TextureTextBuffer.h" "TextureTextBuffer.cxx"
	"Utf8.h" "Utf8.cxx"
	"Library.h" "Library.cxx" "Face.h" "Face.cxx" "FaceGlyph.h" "FaceStyle.h" "Family.h" "Family.cxx" "FallbackChain.h" "FallbackChain.cxx"
	"internal/LibraryHandle.h" "internal/LibraryHandle.cxx" "internal/FaceHandle.h" "internal/FaceHandle.cxx"
	"internal/GlyphBitmap.h" "internal/GlyphHandle.h" "internal/StrokerHandle.h")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_font PUBLIC v3dlib_log v3dlib_image glm::glm)
# PRIVATE: Library.h and Face.h hide FreeType behind forward-declared classes. Only the sources
# and the internal/ headers include FreeType.
target_link_libraries(v3dlib_font PRIVATE Freetype::Freetype)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
