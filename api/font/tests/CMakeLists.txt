v3d_add_test(font
	"TestMain.cxx"
	"BidiTest.cxx"
	"ColourTest.cxx"
	"CurvesTest.cxx"
	"CurveStoreTest.cxx"
	"FallbackChainTest.cxx"
	"FieldTest.cxx"
	"GlyphStoreTest.cxx"
	"ParagraphTest.cxx"
	"SegmenterTest.cxx"
	"ShaperTest.cxx"
	"TextureFontCacheTest.cxx"
	"TextureFontTest.cxx"
	"TextureTextBufferTest.cxx"
	"Utf8Test.cxx"
	"VariationTest.cxx")

target_link_libraries(v3dtest_font PRIVATE v3dlib_font v3dlib_image v3dlib_log ${Boost_LIBRARIES} ${PNG_LIBRARIES} JPEG::JPEG Freetype::Freetype ICU::uc harfbuzz::harfbuzz thorvg::thorvg)

# Every case builds a real face, so the shared font has to sit beside the executable.
v3d_add_shared_data(v3dtest_font)

# The faces in fonts/ are fixtures: one for each colour table and a variable font, written by
# fixtures.ts there.
add_custom_command(TARGET v3dtest_font POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/fonts"
		"$<TARGET_FILE_DIR:v3dtest_font>/fonts"
	COMMENT "Copying font test fonts"
	VERBATIM)
