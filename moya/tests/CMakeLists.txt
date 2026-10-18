# Moya is an offline renderer and links nothing from the realtime stack. Its suite needs
# neither a window nor a device.
v3d_add_test(moya
	"TestMain.cxx"
	"FootprintTest.cxx"
	"FrameBufferTest.cxx"
	"GridShaderTest.cxx"
	"MicroPolygonGridTest.cxx"
	"PolygonClipTest.cxx"
	"PolygonTest.cxx"
	"ProjectedSpaceTest.cxx"
	"RayHiderTest.cxx"
	"ReferenceTest.cxx"
	"RenderContextTest.cxx"
	"RIBHandlerTest.cxx"
	"RenderManTest.cxx"
	"RendererTest.cxx"
	"ReyesPrimitiveTest.cxx"
	"VertexTest.cxx")

# The suite names the libraries it includes. It compares pictures through api/image, reads RIB
# through api/render/offline, and builds loggers and reads what they wrote through api/log and
# the spdlog it carries. glm comes through the api libraries, as a package does.
# The suite creates its output directory itself through Boost::filesystem, and none of the
# libraries it links carries that.
target_link_libraries(v3dtest_moya PRIVATE v3dlib_moya v3dlib_image v3dlib_log v3dlib_render_offline v3dlib_type
	Boost::filesystem)

# The reference image is resolved against the working directory. v3d_add_test sets that to the
# directory of the executable, so the fixtures are copied beside it.
add_custom_command(TARGET v3dtest_moya POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_moya>/data"
	COMMENT "Copying moya test fixtures"
	VERBATIM)
