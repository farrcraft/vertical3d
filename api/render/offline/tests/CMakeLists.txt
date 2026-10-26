# The offline library runs on the CPU only, so every part of it is testable without a
# window or a device.
v3d_add_test(render_offline
	"TestMain.cxx"
	"FrameBufferTest.cxx"
	"FilmTest.cxx"
	"AreaTest.cxx"
	"CameraTest.cxx"
	"MovingTransformTest.cxx"
	"TextureTest.cxx"
	"NoiseTest.cxx"
	"RibLexerTest.cxx"
	"RibDeclarationsTest.cxx"
	"RibReaderTest.cxx"
	"RibWriterTest.cxx"
	"RibArgumentsTest.cxx"
	"SlLexerTest.cxx"
	"SlParserTest.cxx"
	"SlTypesTest.cxx"
	"SlCompilerTest.cxx"
	"SlMachineTest.cxx"
	"SlLibraryTest.cxx"
	"SlDerivativeTest.cxx"
	"SlDisplacementTest.cxx"
	"SlLightingTest.cxx"
	"TraceHierarchyTest.cxx"
	"TraceStencilTest.cxx"
	"SlShaderLibraryTest.cxx"
	"SlImagerTest.cxx"
	"SlGlobalsTest.cxx"
	"SlWalkTest.cxx"
	"TraceSceneTest.cxx"
	"TraceHitShaderTest.cxx"
	"TraceBatchTest.cxx")

target_link_libraries(v3dtest_render_offline PRIVATE v3dlib_render_offline v3dlib_image v3dlib_log v3dlib_type ${Boost_LIBRARIES})

# The suite opens its fixtures by a path relative to the working directory. v3d_add_test
# sets the working directory to the executable's directory.
add_custom_command(TARGET v3dtest_render_offline POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_render_offline>/data"
	COMMENT "Copying offline render test fixtures"
	VERBATIM)
