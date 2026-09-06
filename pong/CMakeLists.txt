add_executable(pong
	"src/Ball.h" "src/Ball.cxx"
	"src/GameState.h" "src/GameState.cxx"
	"src/Paddle.h" "src/Paddle.cxx"
	"src/PongEngine.h" "src/PongEngine.cxx"
	"src/PongRenderer.h" "src/PongRenderer.cxx"
	"src/PongScene.h" "src/PongScene.cxx"
	"src/pong.cxx"
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(pong PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(pong PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(pong PUBLIC WINVER=0x0A00)

target_link_libraries(pong PRIVATE
	v3dlib_asset
	v3dlib_audio
	v3dlib_config
	v3dlib_ecs
	v3dlib_engine
	v3dlib_event
	v3dlib_image
	v3dlib_input
	v3dlib_font
	v3dlib_log
	v3dlib_render
	v3dlib_type
	v3dlib_ui
)

v3d_add_shared_data(pong)

# a console subsystem executable is given a console of its own alongside the app's window.
# main is reached through the WinMain that SDL_main.h supplies.
set_target_properties(pong PROPERTIES WIN32_EXECUTABLE TRUE)

target_compile_options(pong PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
