# The document a sound config is, and what a clip does with a file it cannot read. Playing one
# needs the device initialize() opens, so nothing here calls it.
v3d_add_test(audio
	"TestMain.cpp"
	"AudioClipTest.cpp"
	"EngineTest.cpp")

# SDL3: the mixer carries SDL3 with it, whether or not a device is ever opened.
target_link_libraries(v3dtest_audio PRIVATE
	v3dlib_audio
	v3dlib_asset
	v3dlib_event
	v3dlib_image
	v3dlib_font
	${Boost_LIBRARIES}
	${SDL3_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG
	Freetype::Freetype)
