add_executable(tetris
	"src/Controller.h" "src/Controller.cxx" 
	"src/GameBoard.h" "src/GameBoard.cxx"
	"src/Piece.h" "src/Piece.cxx"
	"src/Renderer.h" "src/Renderer.cxx"
	"src/Tetrad.h" "src/Tetrad.cxx"
	"src/TetrisScene.h" "src/TetrisScene.cxx"
	"src/tetris.cxx")

target_link_libraries(tetris
	v3dlib_asset
	v3dlib_audio
	v3dlib_config
	v3dlib_ecs
	v3dlib_engine
	v3dlib_event
	v3dlib_font
	v3dlib_gl
	v3dlib_image
	v3dlib_input
	v3dlib_log
	v3dlib_render
	v3dlib_type
	${Boost_LIBRARIES}
	${SDL3_LIBRARIES}
	${Vulkan_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG
	OpenGL::GL
	GLEW::GLEW
	Freetype::Freetype
	"soloud"
)

target_compile_options(tetris PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")
