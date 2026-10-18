# Logger wraps spdlog's global registry. The test needs no window and no device.
v3d_add_test(log
	"TestMain.cpp"
	"LoggerTest.cpp")

target_link_libraries(v3dtest_log PRIVATE v3dlib_log Boost::filesystem)
