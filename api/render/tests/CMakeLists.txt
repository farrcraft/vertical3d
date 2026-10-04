# The frame model is testable without a window or a device - a pass collects draw items and a
# sort key orders them, neither of which touches vulkan. So are all three canvases: batching, the
# transform stack, the projection and the line geometry are all cpu side, and so is which format a
# chain is built with, and so is the channel order a captured frame is written in, and so is when a
# released resource is destroyed, given a count of frames. Everything else below the recorder needs
# a device, and waits on ADR-0007.
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
	"CaptureConvertTest.cpp")

target_link_libraries(v3dtest_render PRIVATE v3dlib_render v3dlib_font v3dlib_image v3dlib_type EnTT::EnTT ${SDL3_LIBRARIES} ${Vulkan_LIBRARIES} Freetype::Freetype)

# A second binary rather than more cases in the first, because these need a gpu and that one
# must keep running on a runner with none. ADR-0007 is what puts a software implementation
# there; until it does, this is skipped rather than failed, and 77 is the exit code ctest is
# told to read that way - see device/TestMain.cpp for why it is not simply skipped from inside.
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
	"device/PipelineBuilderTest.cpp")

# A committed picture is resolved against the working directory, which v3d_add_test points at
# the executable, so the references have to sit beside it - the same copy moya and talyn make
# for theirs. What may be in one is ADR-0054.
add_custom_command(TARGET v3dtest_render_device POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/device/data"
		"$<TARGET_FILE_DIR:v3dtest_render_device>/data"
	COMMENT "Copying render device references"
	VERBATIM)

# a depth only pipeline is the one shape the builder could not express, so the case that
# compiles one needs a vertex stage of its own - the renderers' shaders are v3dlib_render's
v3d_add_shader(v3dtest_render_device "shaders/depth.vert")

# a pipeline declaring a scene at set 2 and reading nothing else, which is how the case that
# binds one shows the set arrived - ADR-0064
v3d_add_shader(v3dtest_render_device "shaders/scene.vert")
v3d_add_shader(v3dtest_render_device "shaders/scene.frag")

# a full-screen pass that copies by position, which is how the identity case is exact anywhere
v3d_add_shader(v3dtest_render_device "shaders/copy.frag")

# the mesh registry's cases load models, and the asset suite already keeps the fixtures
target_compile_definitions(v3dtest_render_device PRIVATE V3D_ASSET_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/../../asset/tests/data")

target_link_libraries(v3dtest_render_device PRIVATE v3dlib_render v3dlib_image v3dlib_asset v3dlib_type Boost::filesystem EnTT::EnTT ${SDL3_LIBRARIES} ${Vulkan_LIBRARIES})
set_tests_properties(render_device PROPERTIES SKIP_RETURN_CODE 77)
