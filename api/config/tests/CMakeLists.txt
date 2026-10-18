# Config reads JSON files through an asset manager, and needs no window or device. Each case
# has its own fixture directory, because load() always reads config.json from the manager's
# root.
v3d_add_test(config
	"TestMain.cpp"
	"ConfigTest.cpp"
	"TypeTest.cpp"
	"CameraProfilesTest.cpp"
	"SpriteSheetsTest.cpp"
	"WindowSizeTest.cpp")

target_link_libraries(v3dtest_config PRIVATE
	v3dlib_config
	v3dlib_asset
	v3dlib_log
	v3dlib_type
	${Boost_LIBRARIES})

# The manager resolves a relative path against the working directory. v3d_add_test sets that
# to the executable's directory, so the fixtures are copied there.
add_custom_command(TARGET v3dtest_config POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_config>/data"
	COMMENT "Copying config test fixtures"
	VERBATIM)
