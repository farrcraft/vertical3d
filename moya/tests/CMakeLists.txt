# Moya is an offline renderer and takes nothing from the realtime stack, so its suite needs
# neither a window nor a device.
v3d_add_test(moya
	"TestMain.cxx"
	"FootprintTest.cxx"
	"FrameBufferTest.cxx"
	"GridShaderTest.cxx"
	"MicroPolygonGridTest.cxx"
	"PolygonClipTest.cxx"
	"PolygonTest.cxx"
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
# Boost::filesystem is named because the suite creates its output directory itself, and none of
# the libraries it links carries it.
target_link_libraries(v3dtest_moya PRIVATE v3dlib_moya v3dlib_image v3dlib_log v3dlib_render_offline v3dlib_type
	Boost::filesystem)

# The reference image is resolved against the working directory, which v3d_add_test points at
# the executable, so the fixtures have to sit beside it.
add_custom_command(TARGET v3dtest_moya POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_moya>/data"
	COMMENT "Copying moya test fixtures"
	VERBATIM)
