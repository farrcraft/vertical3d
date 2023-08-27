add_executable("pong" 
	"src/Ball.h" "src/Ball.cxx"
	"src/GameState.h" "src/GameState.cxx"
	"src/Paddle.h" "src/Paddle.cxx"
	"src/PongEngine.h" "src/PongEngine.cxx"
	"src/PongRenderer.h" "src/PongRenderer.cxx"
	"src/PongScene.h" "src/PongScene.cxx"
	"src/pong.cxx" )

target_link_libraries("pong" 
	v3dlib_core
	v3dlib_image
	${Boost_LIBRARIES}
	"turbojpeg-static" "jpeg-static"
	"libpng16d" "zlibd")