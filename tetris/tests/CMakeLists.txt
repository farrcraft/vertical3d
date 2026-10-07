# The board and the tetrad need neither a window nor a device, so they are testable the same
# way an api library is. The renderer is not, and is not covered here.
v3d_add_test(tetris
	"TestMain.cxx"
	"GameBoardTest.cxx"
	"TetradTest.cxx"
	"../src/GameBoard.cxx"
	"../src/Piece.cxx"
	"../src/Tetrad.cxx")

# The board reads its shapes as text through asset::Manager, which registers its text loader
# itself, so the suite needs only the asset library and the logger.
target_link_libraries(v3dtest_tetris PRIVATE
	v3dlib_asset
	v3dlib_log)
