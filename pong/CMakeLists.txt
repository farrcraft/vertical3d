add_executable(pong
	"src/component/Direction.h"
	"src/component/Offset.h"
	"src/component/PaddleSize.h"
	"src/component/Score.h"
	"src/component/Size.h"
	"src/component/Travel.h"
	"src/Ball.h" "src/Ball.cxx"
	"src/GameState.h" "src/GameState.cxx"
	"src/Paddle.h" "src/Paddle.cxx"
	"src/PongEngine.h" "src/PongEngine.cxx"
	"src/PongRenderer.h" "src/PongRenderer.cxx"
	"src/PongScene.h" "src/PongScene.cxx"
	"src/pong.cxx"
)

target_link_libraries(pong PRIVATE
	v3dlib_asset
	v3dlib_audio
	v3dlib_config
	v3dlib_ecs
	v3dlib_engine
	v3dlib_event
	v3dlib_log
	v3dlib_render
	v3dlib_type
	v3dlib_ui
)

v3d_add_app_data(pong)
v3d_add_shared_data(pong)

# build for the windows subsystem. A console subsystem executable opens a console beside the
# app's window. SDL_main.h supplies the WinMain that calls main.
set_target_properties(pong PROPERTIES WIN32_EXECUTABLE TRUE)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
