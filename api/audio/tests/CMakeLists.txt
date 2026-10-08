# The document a sound config is, and what a clip does with a file it cannot read. Playing one
# needs the device initialize() opens, and no test depends on one opening. Only the dispatcher
# test calls initialize(), and it needs SDL's audio subsystem to start but no device to open.
v3d_add_test(audio
	"TestMain.cpp"
	"AudioClipTest.cpp"
	"EngineTest.cpp"
	"LoaderTest.cpp"
	"EventsTest.cpp")

# SDL3: the mixer carries SDL3 with it, whether or not a device is ever opened.
target_link_libraries(v3dtest_audio PRIVATE
	v3dlib_audio
	v3dlib_asset
	v3dlib_event
	v3dlib_log
	${SDL3_LIBRARIES})

# With the FMOD backend built, a case plays the SDK's own example banks. FMOD's two dlls are copied
# beside the executable, because nothing else puts them on its path. There is no such case in CI,
# which builds without the SDK.
if(V3D_FMOD_ROOT)
	target_sources(v3dtest_audio PRIVATE "FmodEventsTest.cpp")
	target_compile_definitions(v3dtest_audio PRIVATE
		V3D_FMOD_EXAMPLE_BANKS="${V3D_FMOD_ROOT}/api/studio/examples/media")
	add_custom_command(TARGET v3dtest_audio POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"$<TARGET_FILE:v3d_fmod_core>" "$<TARGET_FILE:v3d_fmod_studio>"
			"$<TARGET_FILE_DIR:v3dtest_audio>"
		COMMENT "Copying the FMOD dlls beside the audio tests"
		VERBATIM)
endif()
