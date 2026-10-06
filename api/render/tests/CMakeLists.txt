# Cases that need no window and no device. A pass collects draw items and a sort key orders them
# without touching vulkan. The three canvases are cpu side too: batching, the transform stack, the
# projection and the line geometry. So are the choice of swapchain format, the channel order of a
# captured frame, and the frame count after which a released resource is destroyed. Everything
# else below the recorder needs a device, and is in render_device below.
v3d_add_test(render
	"TestMain.cpp"
	"FrameTest.cpp"
	"SortKeyTest.cpp"
	"RegistryTest.cpp"
	"RetirementTest.cpp"
	"CanvasTest.cpp"
	"LineCanvasTest.cpp"
	"WorldCanvasTest.cpp"
	"DepthOrderTest.cpp"
	"SpriteTest.cpp"
	"ParticleTest.cpp"
	"SwapchainFormatTest.cpp"
	"RecorderCheckTest.cpp"
	"LitSettingsTest.cpp"
	"ShadowTest.cpp"
	"GradeTest.cpp"
	"CaptureConvertTest.cpp"
	"ResultTest.cpp")

target_link_libraries(v3dtest_render PRIVATE v3dlib_render v3dlib_ecs v3dlib_font v3dlib_image v3dlib_type EnTT::EnTT ${SDL3_LIBRARIES} ${Vulkan_LIBRARIES} Freetype::Freetype)

# A second binary, because these cases need a Vulkan device and the first must run on a machine
# with none. CI provides a software implementation. Where there is no device at all, the binary
# exits with 77, which ctest reads as skipped rather than failed. device/TestMain.cpp explains why
# the skip happens before the test framework starts.
v3d_add_test(render_device
	"device/TestMain.cpp"
	"device/Headless.cpp"
	"device/Reference.cpp"
	"device/OffscreenFrameTest.cpp"
	"device/WorldDepthTest.cpp"
	"device/WorldBlendTest.cpp"
	"device/EffectsTest.cpp"
	"device/ReleaseTest.cpp"
	"device/DescriptorPoolTest.cpp"
	"device/SceneSetTest.cpp"
	"device/DepthTargetTest.cpp"
	"device/MeshRegistryTest.cpp"
	"device/LitSceneTest.cpp"
	"device/SkinTest.cpp"
	"device/RenderTargetTest.cpp"
	"device/PostTest.cpp"
	"device/PipelineBuilderTest.cpp"
	"device/TimingsTest.cpp")

# A committed picture is resolved against the working directory, which v3d_add_test points at
# the executable, so the references have to sit beside it. moya copies its own the same way.
add_custom_command(TARGET v3dtest_render_device POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/device/data"
		"$<TARGET_FILE_DIR:v3dtest_render_device>/data"
	COMMENT "Copying render device references"
	VERBATIM)

# the case that compiles a depth only pipeline needs a vertex stage of its own, because the
# renderers' shaders belong to v3dlib_render
v3d_add_shader(v3dtest_render_device "shaders/depth.vert")

# a pipeline that declares a scene set at set 2 and reads nothing else, so the case that binds
# one can show the set reached the draw
v3d_add_shader(v3dtest_render_device "shaders/scene.vert")
v3d_add_shader(v3dtest_render_device "shaders/scene.frag")

# a full-screen pass that copies by position, so the identity case is exact on any implementation
v3d_add_shader(v3dtest_render_device "shaders/copy.frag")

# the mesh registry's cases load models, and the asset suite already keeps the fixtures
target_compile_definitions(v3dtest_render_device PRIVATE V3D_ASSET_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/../../asset/tests/data")

target_link_libraries(v3dtest_render_device PRIVATE v3dlib_render v3dlib_ecs v3dlib_image v3dlib_asset v3dlib_asset_media v3dlib_type Boost::filesystem EnTT::EnTT ${SDL3_LIBRARIES} ${Vulkan_LIBRARIES})
set_tests_properties(render_device PROPERTIES SKIP_RETURN_CODE 77)
