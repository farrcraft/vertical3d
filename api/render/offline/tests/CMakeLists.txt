# The offline library is cpu only, so unlike its realtime sibling every part of it is
# testable without a window or a device.
v3d_add_test(render_offline
	"TestMain.cxx"
	"FrameBufferTest.cxx")

target_link_libraries(v3dtest_render_offline PRIVATE v3dlib_render_offline ${Boost_LIBRARIES})
