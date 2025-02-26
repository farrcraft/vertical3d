add_executable(tetris
	"src/Controller.h" "src/Controller.cxx" 
	"src/GameBoard.h" "src/GameBoard.cxx"
	"src/Piece.h" "src/Piece.cxx"
	"src/Renderer.h" "src/Renderer.cxx"
	"src/Tetrad.h" "src/Tetrad.cxx"
	"src/TetrisScene.h" "src/TetrisScene.cxx"
	"src/tetris.cxx")

target_link_libraries(tetris
	v3dlib_core
	v3dlib_image
	${Boost_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG
)

target_compile_options(tetris PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")
