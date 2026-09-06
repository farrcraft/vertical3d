add_executable(tetris
	"src/Controller.h" "src/Controller.cxx"
	"src/GameBoard.h" "src/GameBoard.cxx"
	"src/Piece.h" "src/Piece.cxx"
	"src/Renderer.h" "src/Renderer.cxx"
	"src/Tetrad.h" "src/Tetrad.cxx"
	"src/TetrisScene.h" "src/TetrisScene.cxx"
	"src/tetris.cxx")

target_link_libraries(tetris PRIVATE
	v3dlib_asset
	v3dlib_audio
	v3dlib_config
	v3dlib_ecs
	v3dlib_engine
	v3dlib_event
	v3dlib_font
	v3dlib_image
	v3dlib_input
	v3dlib_log
	v3dlib_render
	v3dlib_type
	v3dlib_ui
)

v3d_add_app_data(tetris)
v3d_add_shared_data(tetris)

# a console subsystem executable is given a console of its own alongside the app's window.
# main is reached through the WinMain that SDL_main.h supplies.
set_target_properties(tetris PROPERTIES WIN32_EXECUTABLE TRUE)

target_compile_options(tetris PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
