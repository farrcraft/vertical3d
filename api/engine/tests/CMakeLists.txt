# The engine without a window: the feature mask, the config and mappings initialize() reads,
# and the quit flag the loop stops on. No test asks for Feature::Window, which would need SDL
# and a device.
v3d_add_test(engine
	"TestMain.cpp"
	"ApplicationTest.cpp"
	"FeatureTest.cpp"
	"AccumulatorTest.cpp"
	"StatisticsTest.cpp"
	"EngineTest.cpp"
	"SettingsTest.cpp"
	"IsometricControllerTest.cpp")

# SDL3: the engine names SDL directly.
target_link_libraries(v3dtest_engine PRIVATE
	v3dlib_engine
	v3dlib_config
	v3dlib_asset
	v3dlib_event
	v3dlib_log
	v3dlib_input
	v3dlib_type
	glm::glm
	EnTT::EnTT
	${Boost_LIBRARIES}
	${SDL3_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG
	Freetype::Freetype
	Vulkan::Vulkan)

# A fixture is an app path rather than a data path - initialize() appends "data/" to it - and
# the manager resolves what it is given against the working directory, which v3d_add_test
# points at the executable.
add_custom_command(TARGET v3dtest_engine POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
		"$<TARGET_FILE_DIR:v3dtest_engine>/fixtures"
	COMMENT "Copying engine test fixtures"
	VERBATIM)
