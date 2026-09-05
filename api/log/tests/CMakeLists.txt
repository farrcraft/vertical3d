# The wrapper is a name and two levels over spdlog's global registry - no window, no device.
v3d_add_test(log
	"TestMain.cpp"
	"LoggerTest.cpp")

target_link_libraries(v3dtest_log PRIVATE v3dlib_log)
