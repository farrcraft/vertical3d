v3d_add_test(font
	"TestMain.cxx"
	"Font2DTest.cxx"
	"TextureFontCacheTest.cxx"
	"TextureTextBufferTest.cxx")

target_link_libraries(v3dtest_font PRIVATE v3dlib_font v3dlib_image ${Boost_LIBRARIES} ${PNG_LIBRARIES} JPEG::JPEG Freetype::Freetype)

# Every case builds a real face, so the shared font has to sit beside the executable.
v3d_add_shared_data(v3dtest_font)
