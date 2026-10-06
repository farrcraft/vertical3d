# The manager, the loader base and the two text-shaped loaders need neither a window nor a
# device - they resolve a path and read a file. The loaders for pictures and models are
# api/asset/media's, and the one for sound api/audio's, each with its own suite.
v3d_add_test(asset
	"TestMain.cpp"
	"ManagerTest.cpp"
	"LoaderTest.cpp"
	"JsonTest.cpp"
	"WriterTest.cpp"
	"MigrationTest.cpp"
	"TextTest.cpp")

target_link_libraries(v3dtest_asset PRIVATE v3dlib_asset ${Boost_LIBRARIES})

# The manager resolves a relative path against the working directory, which v3d_add_test
# points at the executable, so the fixtures have to sit beside it.
add_custom_command(TARGET v3dtest_asset POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_asset>/data"
	COMMENT "Copying asset test fixtures"
	VERBATIM)
