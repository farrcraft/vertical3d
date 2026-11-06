# The version is text and numbers. The test needs no window and no device.
v3d_add_test(version
	"TestMain.cpp"
	"VersionTest.cpp")

target_link_libraries(v3dtest_version PRIVATE v3dlib_version)
