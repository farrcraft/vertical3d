# Components and the system base are plain data over an entt registry - no window, no device
# and no files, so there is nothing to copy beside the executable.
v3d_add_test(ecs
	"TestMain.cpp"
	"ComponentTest.cpp"
	"SystemTest.cpp")

target_link_libraries(v3dtest_ecs PRIVATE v3dlib_ecs)
