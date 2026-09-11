# The offline library is cpu only, so unlike its realtime sibling every part of it is
# testable without a window or a device.
v3d_add_test(render_offline
	"TestMain.cxx"
	"FrameBufferTest.cxx"
	"RibLexerTest.cxx"
	"RibDeclarationsTest.cxx"
	"RibReaderTest.cxx"
	"RibArgumentsTest.cxx"
	"SlLexerTest.cxx"
	"SlParserTest.cxx"
	"SlTypesTest.cxx"
	"SlCompilerTest.cxx"
	"SlMachineTest.cxx"
	"SlLibraryTest.cxx"
	"SlLightingTest.cxx"
	"SlShaderLibraryTest.cxx"
	"SlImagerTest.cxx")

target_link_libraries(v3dtest_render_offline PRIVATE v3dlib_render_offline ${Boost_LIBRARIES})

# The suite opens its fixtures by a relative path against the working directory, which
# v3d_add_test points at the executable.
add_custom_command(TARGET v3dtest_render_offline POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_render_offline>/data"
	COMMENT "Copying offline render test fixtures"
	VERBATIM)
