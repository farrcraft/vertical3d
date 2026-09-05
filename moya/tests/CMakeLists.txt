# Moya is an offline renderer and takes nothing from the realtime stack, so its suite needs
# neither a window nor a device - only the type library its geometry is built on.
v3d_add_test(moya
	"TestMain.cxx"
	"FrameBufferTest.cxx"
	"FrustumTest.cxx"
	"MicroPolygonGridTest.cxx"
	"PlaneTest.cxx"
	"PolygonTest.cxx"
	"RenderContextTest.cxx"
	"RendererTest.cxx"
	"ReyesPrimitiveTest.cxx"
	"VertexTest.cxx")

target_link_libraries(v3dtest_moya PRIVATE v3dlib_moya v3dlib_type ${Boost_LIBRARIES})
