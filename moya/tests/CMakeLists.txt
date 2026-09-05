# Moya is an offline renderer and takes nothing from the realtime stack, so its suite needs
# neither a window nor a device - only the type library its geometry is built on.
v3d_add_test(moya
	"TestMain.cxx"
	"FrameBufferTest.cxx"
	"FrustumTest.cxx"
	"MicroPolygonGridTest.cxx"
	"PlaneTest.cxx"
	"PolygonTest.cxx"
	"ReferenceTest.cxx"
	"RenderContextTest.cxx"
	"RendererTest.cxx"
	"ReyesPrimitiveTest.cxx"
	"VertexTest.cxx")

# the suite renders through image::Factory, which is where the png and jpeg codecs come in
target_link_libraries(v3dtest_moya PRIVATE v3dlib_moya v3dlib_type ${Boost_LIBRARIES} ${PNG_LIBRARIES} JPEG::JPEG)

# The reference image is resolved against the working directory, which v3d_add_test points at
# the executable, so the fixtures have to sit beside it.
add_custom_command(TARGET v3dtest_moya POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_moya>/data"
	COMMENT "Copying moya test fixtures"
	VERBATIM)
