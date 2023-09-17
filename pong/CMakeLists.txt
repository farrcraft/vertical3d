add_executable("pong" 
	"src/Ball.h" "src/Ball.cxx"
	"src/GameState.h" "src/GameState.cxx"
	"src/Paddle.h" "src/Paddle.cxx"
	"src/PongEngine.h" "src/PongEngine.cxx"
	"src/PongRenderer.h" "src/PongRenderer.cxx"
	"src/PongScene.h" "src/PongScene.cxx"
	"src/pong.cxx" )

target_link_libraries("pong"
	v3dlib_asset
	v3dlib_audio
	v3dlib_config
	v3dlib_ecs
	v3dlib_engine
	v3dlib_event
	v3dlib_gl
	v3dlib_image
	v3dlib_font
	v3dlib_render
	v3dlib_ui
	${Boost_LIBRARIES}
	${SDL2_LIBRARIES}
	"turbojpeg-static" "jpeg-static"
	"libpng16d" "zlibd" "glew32")
