# The suite runs as parts, grouped by what they cover, since in Debug it takes most of an hour as
# one test. A part that renders scenes through the integrator is labelled renders; the others
# check one piece of moya at a time. A new case's name starts with its file's prefix, which
# puts it in that file's part.
v3d_test_parts(moya
	PART scattering CASES moya_bsdf_* moya_conductor_* moya_dielectric_* moya_fuzz_* moya_hair_* moya_layering_* moya_thin_* moya_geometric_*
	PART lights RENDERS CASES moya_lights_*
	PART paths RENDERS CASES moya_paths_*
	PART transport RENDERS CASES moya_lightpaths_* moya_lightsampling_* moya_opacity_* moya_outputs_* moya_lpe_* moya_cryptomatte_*
		moya_deep_* moya_denoise_* moya_denoiser_* moya_denoisefeatures_* moya_firsthit_* moya_compositing_*
	PART volumes RENDERS CASES moya_volume_* moya_media_* moya_subsurface_*
	PART scene RENDERS CASES moya_scene_* moya_sceneclient_* moya_scenehits_* moya_scenematerials_* moya_session* moya_kept_* moya_assembler_*
		moya_events_*
	PART geometry CASES moya_builder_* moya_traversal_* moya_hit_* moya_hitview_* moya_instances_* moya_motion_* moya_shapes_* moya_curves_*
		moya_subdivision_* moya_displacement_* moya_triangulate_* moya_surfaces_* moya_cone_*
	PART materials RENDERS CASES moya_material_* moya_program_* moya_texture_* moya_normal_* moya_shaders_*
	PART foundation RENDERS CASES moya_foundation_* moya_output_* moya_colour_* moya_spectrum_* moya_rgb_* moya_zsobol_* moya_film_*
		moya_filmcombine_* moya_exr_* moya_compare_* moya_device_* moya_grid_* moya_checkpoint_*
	PART tools RENDERS CASES moya_bench_* moya_command_*)

# Moya is an offline renderer and links nothing from the realtime stack. Its suite needs
# neither a window nor a device.
v3d_add_test(moya
	"TestMain.cxx"
	"FoundationTest.cxx"
	"AssemblerTest.cxx"
	"BenchTest.cxx"
	"BsdfTest.cxx"
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
	"GridTest.cxx"
	"HairTest.cxx"
	"KeptTest.cxx"
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
	"HitTest.cxx"
	"HitViewTest.cxx"
	"InstancesTest.cxx"
	"LightSamplingTest.cxx"
	"LightPathsTest.cxx"
	"LightsKeptTest.cxx"
	"LightsTest.cxx"
	"LpeTest.cxx"
	"MaterialCompileTest.cxx"
	"MaterialLibraryTest.cxx"
	"MediaTest.cxx"
	"MotionTest.cxx"
	"NormalMapTest.cxx"
	"OpacityTest.cxx"
	"OutputsTest.cxx"
	"PathsReferenceTest.cxx"
	"PathsTest.cxx"
	"ProgramTest.cxx"
	"TriangulateTest.cxx"
	"TraversalTest.cxx"
	"RgbTest.cxx"
	"SceneClientTest.cxx"
	"SessionTest.cxx"
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
	"SubsurfaceTest.cxx"
	"SurfaceShadersTest.cxx"
	"SurfacesTest.cxx"
	"TextureCacheTest.cxx"
	"TextureLookupTest.cxx"
	"TextureMakerTest.cxx"
	"VolumeTest.cxx"
	"VolumeLightsTest.cxx"
	"VolumeOutputsTest.cxx"
	"VolumeShaderTest.cxx"
	"ZSobolTest.cxx")

# The suite names the libraries it includes. It compares pictures through api/image and writes
# what it rendered through api/asset, reads RIB through api/rib, and builds loggers
# and reads what they wrote through api/log and the spdlog it carries. It reads the CPU's node
# layout and instruction sets through v3dlib_moya_cpu. glm and Boost::filesystem
# come through the api libraries, as a package does. It launches kernel code on the CPU device,
# and its main waits for TBB's threads, which libmoya links PRIVATE. It reads back the EXR files
# libmoya writes through OpenEXR and Imath, and checks colour conversions against OpenColorIO's own,
# which libmoya links PRIVATE too. It flattens MaterialX documents through MaterialX itself, to
# check the materials libmoya flattens against MaterialX's own. It denoises through OIDN, which
# libmoya links PRIVATE. It writes and reads grids through OpenVDB, which libmoya links PRIVATE
# too, and converts them with NanoVDB's tools, which build from OpenVDB's grids only when told to.
target_link_libraries(v3dtest_moya PRIVATE v3dlib_moya v3dlib_moya_cpu v3dlib_moya_kernel v3dlib_asset v3dlib_image v3dlib_log v3dlib_rib
	Imath::Imath MaterialXCore MaterialXFormat OpenColorIO::OpenColorIO OpenEXR::OpenEXR OpenImageDenoise OpenVDB::openvdb TBB::tbb)
target_compile_definitions(v3dtest_moya PRIVATE NANOVDB_USE_OPENVDB)

# The command line cases run the moya executable itself, and the benchmark's case runs
# moya_bench, each by its path in the build tree, so both are built before the suite.
add_dependencies(v3dtest_moya moya moya_bench)
target_compile_definitions(v3dtest_moya PRIVATE V3D_MOYA_EXECUTABLE="$<TARGET_FILE:moya>" V3D_MOYA_BENCH_EXECUTABLE="$<TARGET_FILE:moya_bench>")

# The reference image is resolved against the working directory. v3d_add_test sets that to the
# directory of the executable, so the fixtures are copied beside it.
v3d_copy_directory(v3dtest_moya fixtures "${CMAKE_CURRENT_SOURCE_DIR}/data" "data")

# The suite traces through the CPU device, which loads its libraries from beside the executable.
v3d_moya_cpu_libraries(v3dtest_moya)

# The suite reads MaterialX's standard libraries from beside the executable, as moya does.
v3d_moya_materialx_libraries(v3dtest_moya)
