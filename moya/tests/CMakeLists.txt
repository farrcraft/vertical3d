# Moya is an offline renderer and links nothing from the realtime stack. Its suite needs
# neither a window nor a device.
v3d_add_test(moya
	"TestMain.cxx"
	"FootprintTest.cxx"
	"FoundationTest.cxx"
	"AssemblerTest.cxx"
	"BenchTest.cxx"
	"BsdfTest.cxx"
	"BucketsTest.cxx"
	"BuilderTest.cxx"
	"ColourTest.cxx"
	"ConeTest.cxx"
	"CheckpointTest.cxx"
	"CommandLineTest.cxx"
	"CompositingReferenceTest.cxx"
	"CompareTest.cxx"
	"CryptomatteTest.cxx"
	"ConductorTest.cxx"
	"DielectricTest.cxx"
	"DisplacementTest.cxx"
	"FuzzTest.cxx"
	"GeometricNodesTest.cxx"
	"LayeringTest.cxx"
	"ThinFilmTest.cxx"
	"CurvesTest.cxx"
	"DeepTest.cxx"
	"DenoiseTest.cxx"
	"DenoiserTest.cxx"
	"DeviceTest.cxx"
	"ExrTest.cxx"
	"FilmCombineTest.cxx"
	"DenoiseFeaturesTest.cxx"
	"EventsTest.cxx"
	"FirstHitTest.cxx"
	"FilmTest.cxx"
	"GridShaderTest.cxx"
	"HitTest.cxx"
	"HitViewTest.cxx"
	"InstancesTest.cxx"
	"LightSamplingTest.cxx"
	"LightPathsTest.cxx"
	"LightsTest.cxx"
	"LpeTest.cxx"
	"MaterialCompileTest.cxx"
	"MaterialLibraryTest.cxx"
	"MotionTest.cxx"
	"NormalMapTest.cxx"
	"OpacityTest.cxx"
	"OutputsTest.cxx"
	"PathsReferenceTest.cxx"
	"PathsTest.cxx"
	"MicroPolygonGridTest.cxx"
	"PolygonClipTest.cxx"
	"PolygonTest.cxx"
	"ProgramTest.cxx"
	"ProjectedSpaceTest.cxx"
	"RayHiderTest.cxx"
	"Reference.cxx"
	"ReyesReferenceTest.cxx"
	"SurfaceReferenceTest.cxx"
	"TriangulateTest.cxx"
	"TracedReferenceTest.cxx"
	"TraversalTest.cxx"
	"RenderContextTest.cxx"
	"RIBCameraTest.cxx"
	"RIBGeometryTest.cxx"
	"RIBCurvesTest.cxx"
	"RIBHandlerTest.cxx"
	"RIBOutputTest.cxx"
	"RIBStateTest.cxx"
	"RenderManTest.cxx"
	"RendererTest.cxx"
	"RgbTest.cxx"
	"ReyesPrimitiveTest.cxx"
	"SamplesTest.cxx"
	"SceneClientTest.cxx"
	"SceneDescriptionTest.cxx"
	"SceneGeometryTest.cxx"
	"SceneHitsTest.cxx"
	"SceneKindsTest.cxx"
	"SceneMaterialsTest.cxx"
	"SceneShadingTest.cxx"
	"SceneTest.cxx"
	"SceneWorldTest.cxx"
	"ShapesTest.cxx"
	"SpectrumTest.cxx"
	"SubdivisionTest.cxx"
	"SurfaceShadersTest.cxx"
	"SurfacesTest.cxx"
	"TextureCacheTest.cxx"
	"TextureLookupTest.cxx"
	"TextureMakerTest.cxx"
	"VertexTest.cxx"
	"ZSobolTest.cxx")

# The suite names the libraries it includes. It compares pictures through api/image and writes
# what it rendered through api/asset, reads RIB through api/render/offline, and builds loggers
# and reads what they wrote through api/log and the spdlog it carries. It reads the CPU's node
# layout and instruction sets through v3dlib_moya_cpu. glm and Boost::filesystem
# come through the api libraries, as a package does. It launches kernel code on the CPU device,
# and its main waits for TBB's threads, which libmoya links PRIVATE. It reads back the EXR files
# libmoya writes through OpenEXR and Imath, and checks colour conversions against OpenColorIO's own,
# which libmoya links PRIVATE too. It flattens MaterialX documents through MaterialX itself, to
# check the materials libmoya flattens against MaterialX's own. It denoises through OIDN, which
# libmoya does not link yet.
target_link_libraries(v3dtest_moya PRIVATE v3dlib_moya v3dlib_moya_cpu v3dlib_moya_kernel v3dlib_asset v3dlib_image v3dlib_log v3dlib_render_offline
	v3dlib_type Imath::Imath MaterialXCore MaterialXFormat OpenColorIO::OpenColorIO OpenEXR::OpenEXR OpenImageDenoise TBB::tbb)

# The command line cases run the moya executable itself, and the benchmark's case runs
# moya_bench, each by its path in the build tree, so both are built before the suite.
add_dependencies(v3dtest_moya moya moya_bench)
target_compile_definitions(v3dtest_moya PRIVATE V3D_MOYA_EXECUTABLE="$<TARGET_FILE:moya>" V3D_MOYA_BENCH_EXECUTABLE="$<TARGET_FILE:moya_bench>")

# The reference image is resolved against the working directory. v3d_add_test sets that to the
# directory of the executable, so the fixtures are copied beside it.
add_custom_command(TARGET v3dtest_moya POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_moya>/data"
	COMMENT "Copying moya test fixtures"
	VERBATIM)

# The suite traces through the CPU device, which loads its libraries from beside the executable.
v3d_moya_cpu_libraries(v3dtest_moya)

# The suite reads MaterialX's standard libraries from beside the executable, as moya does.
v3d_moya_materialx_libraries(v3dtest_moya)
