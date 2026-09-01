# The frame model is testable without a window or a device - a pass collects draw items and
# a sort key orders them, neither of which touches vulkan. Everything below the recorder
# does, and waits on ADR-0007.
v3d_add_test(render
	"TestMain.cpp"
	"FrameTest.cpp"
	"SortKeyTest.cpp"
	"RegistryTest.cpp")

target_link_libraries(v3dtest_render PRIVATE v3dlib_render EnTT::EnTT ${SDL3_LIBRARIES} ${Vulkan_LIBRARIES})
