v3d_add_api_library(font "TextBuffer.h" "TextBuffer.cxx" "TextureFont.h" "TextureFont.cxx" "TextureFontCache.h" "TextureFontCache.cxx"
	"TextureTextBuffer.h" "TextureTextBuffer.cxx"
	"Utf8.h" "Utf8.cxx"
	"Library.h" "Library.cxx" "Face.h" "Face.cxx" "FaceGlyph.h" "FaceMetrics.h" "FaceStyle.h" "Family.h" "Family.cxx" "FallbackChain.h" "FallbackChain.cxx"
	"Feature.h" "ShapedGlyph.h" "ShapedRun.h" "Shaper.h" "Shaper.cxx"
	"Segmenter.h" "Segmenter.cxx" "Bidi.h" "Bidi.cxx"
	"GlyphStore.h" "GlyphStore.cxx" "TextStyle.h" "TextStyle.cxx" "ShapingCache.h" "ShapingCache.cxx" "Paragraph.h" "Paragraph.cxx"
	"internal/LibraryHandle.h" "internal/LibraryHandle.cxx" "internal/FaceHandle.h" "internal/FaceHandle.cxx"
	"internal/BreakHandle.h" "internal/BreakHandle.cxx" "internal/BidiHandle.h" "internal/BidiHandle.cxx" "internal/Icu.h"
	"internal/Measure.h" "internal/Measure.cxx" "internal/MetricsCache.h" "internal/MetricsCache.cxx" "internal/Span.h"
	"internal/GlyphBitmap.h" "internal/GlyphHandle.h" "internal/StrokerHandle.h")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_font PUBLIC v3dlib_log v3dlib_image glm::glm)
# PRIVATE: Library.h, Face.h, Segmenter.h and Bidi.h hide FreeType and ICU behind
# forward-declared classes. Only the sources and the internal/ headers include them.
target_link_libraries(v3dlib_font PRIVATE Freetype::Freetype harfbuzz::harfbuzz ICU::uc ICU::data)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
