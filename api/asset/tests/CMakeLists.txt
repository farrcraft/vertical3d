# The manager, the loader base and the two text-shaped loaders need neither a window nor a
# device - they resolve a path and read a file. The image, font and audio loaders are covered
# by their own libraries' suites.
v3d_add_test(asset
	"TestMain.cpp"
	"ManagerTest.cpp"
	"LoaderTest.cpp"
	"JsonTest.cpp"
	"TextTest.cpp")

# SDL3: v3dlib_asset reaches soloud through v3dlib_audio, and soloud's static backend
# names the SDL audio entry points, so a binary that links asset at all resolves them.
target_link_libraries(v3dtest_asset PRIVATE
	v3dlib_asset
	v3dlib_image
	v3dlib_font
	${Boost_LIBRARIES}
	${SDL3_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG
	Freetype::Freetype)

# The manager resolves a relative path against the working directory, which v3d_add_test
# points at the executable, so the fixtures have to sit beside it.
add_custom_command(TARGET v3dtest_asset POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_asset>/data"
	COMMENT "Copying asset test fixtures"
	VERBATIM)
