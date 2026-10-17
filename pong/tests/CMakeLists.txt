# The scene is where pong's rules are - collision, scoring, the ai and the paddle run - and
# it holds nothing but entities and a dispatcher, so it needs neither a window nor a device.
# The engine is built so its commands can be checked against the bindings pong ships, and is
# never started, so it needs neither either.
v3d_add_test(pong
	"TestMain.cxx"
	"BallTest.cxx"
	"BindingsTest.cxx"
	"GameStateTest.cxx"
	"PaddleTest.cxx"
	"PongSceneTest.cxx"
	"../src/Ball.cxx"
	"../src/GameState.cxx"
	"../src/Paddle.cxx"
	"../src/PongEngine.cxx"
	"../src/PongRenderer.cxx"
	"../src/PongScene.cxx")

target_link_libraries(v3dtest_pong PRIVATE
	v3dlib_asset
	v3dlib_audio
	v3dlib_config
	v3dlib_ecs
	v3dlib_engine
	v3dlib_event
	v3dlib_log
	v3dlib_render
	v3dlib_type
	v3dlib_ui)

v3d_copy_data(v3dtest_pong app "${CMAKE_CURRENT_SOURCE_DIR}/../data")
