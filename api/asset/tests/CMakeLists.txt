# The manager, the loader base and the document loaders need neither a window nor a device.
# They resolve a path and read a file. The loaders for pictures and models are tested in
# api/asset/media, and the loader for sound in api/audio.
v3d_add_test(asset
	"TestMain.cpp"
	"ManagerTest.cpp"
	"ManifestTest.cpp"
	"LoaderTest.cpp"
	"JsonTest.cpp"
	"JsonReadTest.cpp"
	"WriterTest.cpp"
	"MigrationTest.cpp"
	"TextTest.cpp"
	"WorkerTest.cpp")

target_link_libraries(v3dtest_asset PRIVATE v3dlib_asset v3dlib_log ${Boost_LIBRARIES})

# The manager resolves a relative path against the working directory, which v3d_add_test
# points at the executable, so the fixtures have to sit beside it.
v3d_copy_directory(v3dtest_asset fixtures "${CMAKE_CURRENT_SOURCE_DIR}/data" "data")
