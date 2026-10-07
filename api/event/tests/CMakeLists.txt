v3d_add_test(event
	"TestMain.cpp"
	"BindingsTest.cpp"
	"EngineTest.cpp"
	"EventTest.cpp"
	"MapperTest.cpp")

target_link_libraries(v3dtest_event PRIVATE v3dlib_event v3dlib_log EnTT::EnTT ${Boost_LIBRARIES})
