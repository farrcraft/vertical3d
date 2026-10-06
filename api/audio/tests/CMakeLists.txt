# The document a sound config is, and what a clip does with a file it cannot read. Playing one
# needs the device initialize() opens, and no test depends on one opening. Only the dispatcher
# test calls initialize(), and it needs SDL's audio subsystem to start but no device to open.
v3d_add_test(audio
	"TestMain.cpp"
	"AudioClipTest.cpp"
	"EngineTest.cpp"
	"LoaderTest.cpp")

# SDL3: the mixer carries SDL3 with it, whether or not a device is ever opened.
target_link_libraries(v3dtest_audio PRIVATE
	v3dlib_audio
	v3dlib_asset
	${SDL3_LIBRARIES})
