# The scene is where pong's rules are - collision, scoring, the ai and the paddle run - and
# it holds nothing but entities and a dispatcher, so it needs neither a window nor a device.
# The engine and the renderer do, and are not covered here.
v3d_add_test(pong
	"TestMain.cxx"
	"BallTest.cxx"
	"GameStateTest.cxx"
	"PaddleTest.cxx"
	"PongSceneTest.cxx"
	"../src/Ball.cxx"
	"../src/GameState.cxx"
	"../src/Paddle.cxx"
	"../src/PongScene.cxx")

target_link_libraries(v3dtest_pong PRIVATE
	v3dlib_ecs
	v3dlib_event
	v3dlib_log
	EnTT::EnTT
	${Boost_LIBRARIES})
