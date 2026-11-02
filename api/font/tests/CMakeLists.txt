v3d_add_test(font
	"TestMain.cxx"
	"BidiTest.cxx"
	"CurvesTest.cxx"
	"CurveStoreTest.cxx"
	"FallbackChainTest.cxx"
	"GlyphStoreTest.cxx"
	"ParagraphTest.cxx"
	"SegmenterTest.cxx"
	"ShaperTest.cxx"
	"TextureFontCacheTest.cxx"
	"TextureFontTest.cxx"
	"TextureTextBufferTest.cxx"
	"Utf8Test.cxx")

target_link_libraries(v3dtest_font PRIVATE v3dlib_font v3dlib_image v3dlib_log ${Boost_LIBRARIES} ${PNG_LIBRARIES} JPEG::JPEG Freetype::Freetype ICU::uc)

# Every case builds a real face, so the shared font has to sit beside the executable.
v3d_add_shared_data(v3dtest_font)
