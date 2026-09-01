v3d_add_test(event
	"TestMain.cpp"
	"EngineTest.cpp"
	"EventTest.cpp"
	"MapperTest.cpp")

target_link_libraries(v3dtest_event PRIVATE v3dlib_event EnTT::EnTT)
