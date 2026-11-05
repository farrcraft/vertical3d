v3d_add_api_library(font
	"face/Library.h" "face/Library.cxx"
	"face/Face.h" "face/Face.cxx"
	"face/Glyph.h"
	"face/Metrics.h"
	"face/Style.h"
	"face/Family.h" "face/Family.cxx"
	"face/FallbackChain.h" "face/FallbackChain.cxx"
	"face/Variation.h"
	"text/Utf8.h" "text/Utf8.cxx"
	"text/Segmenter.h" "text/Segmenter.cxx"
	"text/Bidi.h" "text/Bidi.cxx"
	"text/Feature.h"
	"text/ShapedGlyph.h"
	"text/ShapedRun.h"
	"text/Shaper.h" "text/Shaper.cxx"
	"text/ShapingCache.h" "text/ShapingCache.cxx"
	"text/Style.h" "text/Style.cxx"
	"text/Paragraph.h" "text/Paragraph.cxx"
	"text/internal/BidiHandle.h" "text/internal/BidiHandle.cxx"
	"text/internal/BreakHandle.h" "text/internal/BreakHandle.cxx"
	"text/internal/Icu.h"
	"text/internal/Measure.h" "text/internal/Measure.cxx"
	"text/internal/MetricsCache.h" "text/internal/MetricsCache.cxx"
	"text/internal/Span.h"
	"glyph/Store.h" "glyph/Store.cxx"
	"glyph/Rasterize.h" "glyph/Rasterize.cxx"
	"glyph/Field.h" "glyph/Field.cxx"
	"glyph/Curves.h" "glyph/Curves.cxx"
	"glyph/CurveStore.h" "glyph/CurveStore.cxx"
	"glyph/ColourGlyph.h" "glyph/ColourGlyph.cxx"
	"glyph/Paint.h" "glyph/Paint.cxx"
	"glyph/internal/PaintGrid.h" "glyph/internal/PaintGrid.cxx"
	"glyph/internal/PaintWalk.h" "glyph/internal/PaintWalk.cxx"
	"glyph/internal/SlotBitmap.h" "glyph/internal/SlotBitmap.cxx"
	"texture/Buffer.h" "texture/Buffer.cxx"
	"texture/Font.h" "texture/Font.cxx"
	"texture/FontCache.h" "texture/FontCache.cxx"
	"texture/TextBuffer.h" "texture/TextBuffer.cxx"
	"texture/internal/GlyphBitmap.h"
	"texture/internal/GlyphHandle.h"
	"internal/LibraryHandle.h" "internal/LibraryHandle.cxx"
	"internal/FaceHandle.h" "internal/FaceHandle.cxx"
	"internal/Composite.h" "internal/Composite.cxx"
	"internal/OutlineHandle.h"
	"internal/StrokerHandle.h"
	"internal/OutlineReader.h" "internal/OutlineReader.cxx"
	"internal/CurveBuilder.h" "internal/CurveBuilder.cxx"
	"internal/SvgReader.h" "internal/SvgReader.cxx"
	"internal/SvgDocuments.h" "internal/SvgDocuments.cxx")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_font PUBLIC v3dlib_log v3dlib_image glm::glm)
# PRIVATE: face/Library.h, face/Face.h, text/Segmenter.h and text/Bidi.h hide FreeType and ICU
# behind forward-declared classes. Only the sources and the internal/ headers include
# them, and ThorVG.
target_link_libraries(v3dlib_font PRIVATE Freetype::Freetype harfbuzz::harfbuzz ICU::uc ICU::data thorvg::thorvg)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
