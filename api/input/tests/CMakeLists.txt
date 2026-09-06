v3d_add_test(input
	"TestMain.cpp"
	"KeyboardTest.cpp"
	"MouseTest.cpp")

target_link_libraries(v3dtest_input PRIVATE v3dlib_input v3dlib_event EnTT::EnTT ${SDL3_LIBRARIES})
