# Cases that need no window and no device. They test CPU-side code, such as sorting draw items,
# canvas geometry, choosing a swapchain format and decoding a model. Decoding reads files but
# makes no device call, so it is here too. A case that needs a device goes in render_device below.
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
	"SwapchainPresentTest.cpp"
	"RecorderCheckTest.cpp"
	"ClipTest.cpp"
	"LitSettingsTest.cpp"
	"ShadowTest.cpp"
	"GradeTest.cpp"
	"CaptureConvertTest.cpp"
	"QuadFormatTest.cpp"
	"ResultTest.cpp"
	"DecodedModelTest.cpp")

target_link_libraries(v3dtest_render PRIVATE v3dlib_render v3dlib_ecs v3dlib_font v3dlib_image v3dlib_type v3dlib_asset v3dlib_asset_media v3dlib_log Boost::filesystem EnTT::EnTT ${SDL3_LIBRARIES} ${Vulkan_LIBRARIES} Freetype::Freetype)

# a model is decoded from a copy of the asset suite's fixtures
target_compile_definitions(v3dtest_render PRIVATE V3D_ASSET_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/../../asset/tests/data")

# A second binary, because these cases need a Vulkan device and the first must run on a machine
# with none. CI provides a software implementation. Where there is no device at all, the binary
# exits with 77, which ctest reports as skipped, not failed. device/TestMain.cpp says why the
# binary exits before the test framework starts.
v3d_add_test(render_device
	"device/TestMain.cpp"
	"device/Headless.cpp"
	"device/Reference.cpp"
	"device/Shapes.cpp"
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
	"device/LitCrowdTest.cpp"
	"device/RenderTargetTest.cpp"
	"device/PostTest.cpp"
	"device/PipelineBuilderTest.cpp"
	"device/TimingsTest.cpp"
	"device/TextureSamplerTest.cpp"
	"device/DepthWriteTest.cpp"
	"device/DeviceDescriptionTest.cpp"
	"device/AllocatorTest.cpp")

# A committed reference image is opened relative to the working directory. v3d_add_test sets
# that to the executable's directory, so the references are copied there.
add_custom_command(TARGET v3dtest_render_device POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/device/data"
		"$<TARGET_FILE_DIR:v3dtest_render_device>/data"
	COMMENT "Copying render device references"
	VERBATIM)

# the case that compiles a depth only pipeline needs a vertex stage of its own, because the
# renderers' shaders belong to v3dlib_render
v3d_add_shader(v3dtest_render_device "shaders/depth.vert")

# a second vertex stage that differs from depth.vert only in its code, so the pipeline cache case
# has two pipelines that differ in one field
v3d_add_shader(v3dtest_render_device "shaders/flat.vert")

# a pipeline that declares a scene set at set 2 and reads nothing else, so the case that binds
# one can show the set reached the draw
v3d_add_shader(v3dtest_render_device "shaders/scene.vert")
v3d_add_shader(v3dtest_render_device "shaders/scene.frag")

# a full-screen pass that copies by position, so the identity case is exact on any implementation
v3d_add_shader(v3dtest_render_device "shaders/copy.frag")

# writes the texture bound at set 1, so a case can show which material a draw was recorded with
v3d_add_shader(v3dtest_render_device "shaders/albedo.frag")

# the mesh registry's cases load models, and the asset suite already keeps the fixtures
target_compile_definitions(v3dtest_render_device PRIVATE V3D_ASSET_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/../../asset/tests/data")

target_link_libraries(v3dtest_render_device PRIVATE v3dlib_render v3dlib_ecs v3dlib_image v3dlib_asset v3dlib_asset_media v3dlib_log v3dlib_type Boost::filesystem EnTT::EnTT ${SDL3_LIBRARIES} ${Vulkan_LIBRARIES})
set_tests_properties(render_device PROPERTIES SKIP_RETURN_CODE 77)

# A third binary, because these cases present to a window, which needs a display as well as a
# device. Where no window can be made, or nothing can present to it, the binary exits with 77.
v3d_add_test(render_window
	"window/TestMain.cpp"
	"window/Windowed.cpp"
	"window/EngineCaptureTest.cpp"
	"window/EngineAllocationsTest.cpp"
	"window/FullscreenTest.cpp")

target_link_libraries(v3dtest_render_window PRIVATE v3dlib_render v3dlib_image v3dlib_log Boost::filesystem ${SDL3_LIBRARIES} ${Vulkan_LIBRARIES})
set_tests_properties(render_window PROPERTIES SKIP_RETURN_CODE 77)
