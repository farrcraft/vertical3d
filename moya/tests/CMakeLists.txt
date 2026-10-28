# Moya is an offline renderer and links nothing from the realtime stack. Its suite needs
# neither a window nor a device.
v3d_add_test(moya
	"TestMain.cxx"
	"FootprintTest.cxx"
	"BucketsTest.cxx"
	"GridShaderTest.cxx"
	"MicroPolygonGridTest.cxx"
	"PolygonClipTest.cxx"
	"PolygonTest.cxx"
	"ProjectedSpaceTest.cxx"
	"RayHiderTest.cxx"
	"Reference.cxx"
	"ReyesReferenceTest.cxx"
	"SurfaceReferenceTest.cxx"
	"TracedReferenceTest.cxx"
	"RenderContextTest.cxx"
	"RIBCameraTest.cxx"
	"RIBGeometryTest.cxx"
	"RIBCurvesTest.cxx"
	"RIBHandlerTest.cxx"
	"RIBOutputTest.cxx"
	"RIBStateTest.cxx"
	"RenderManTest.cxx"
	"RendererTest.cxx"
	"ReyesPrimitiveTest.cxx"
	"SamplesTest.cxx"
	"VertexTest.cxx")

# The suite names the libraries it includes. It compares pictures through api/image and writes
# what it rendered through api/asset, reads RIB through api/render/offline, and builds loggers
# and reads what they wrote through api/log and the spdlog it carries. glm and Boost::filesystem
# come through the api libraries, as a package does.
target_link_libraries(v3dtest_moya PRIVATE v3dlib_moya v3dlib_asset v3dlib_image v3dlib_log v3dlib_render_offline
	v3dlib_type)

# The reference image is resolved against the working directory. v3d_add_test sets that to the
# directory of the executable, so the fixtures are copied beside it.
add_custom_command(TARGET v3dtest_moya POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_moya>/data"
	COMMENT "Copying moya test fixtures"
	VERBATIM)
