# Config reads JSON files through an asset manager and holds what it read - no window, no
# device. A fixture directory per case, because load() always reads config.json out of the
# manager's own path.
v3d_add_test(config
	"TestMain.cpp"
	"ConfigTest.cpp"
	"TypeTest.cpp"
	"CameraProfilesTest.cpp"
	"SpriteSheetsTest.cpp")

# SDL3: v3dlib_config reaches the mixer through v3dlib_asset and v3dlib_audio.
target_link_libraries(v3dtest_config PRIVATE
	v3dlib_config
	v3dlib_asset
	v3dlib_type
	v3dlib_image
	v3dlib_font
	${Boost_LIBRARIES}
	${SDL3_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG
	Freetype::Freetype)

# The manager resolves a relative path against the working directory, which v3d_add_test
# points at the executable, so the fixtures have to sit beside it.
add_custom_command(TARGET v3dtest_config POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_config>/data"
	COMMENT "Copying config test fixtures"
	VERBATIM)
