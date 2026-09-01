# The frame model is testable without a window or a device - a pass collects draw items and a
# sort key orders them, neither of which touches vulkan. So is the canvas: batching, the
# transform stack and the projection are all cpu side. Everything below the recorder needs a
# device, and waits on ADR-0007.
v3d_add_test(render
	"TestMain.cpp"
	"FrameTest.cpp"
	"SortKeyTest.cpp"
	"RegistryTest.cpp"
	"CanvasTest.cpp")

target_link_libraries(v3dtest_render PRIVATE v3dlib_render v3dlib_font EnTT::EnTT ${SDL3_LIBRARIES} ${Vulkan_LIBRARIES} Freetype::Freetype)
