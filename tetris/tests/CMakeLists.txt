# The board and the tetrad need neither a window nor a device, so they are testable the same
# way an api library is. The renderer is not, and is not covered here.
v3d_add_test(tetris
	"TestMain.cxx"
	"GameBoardTest.cxx"
	"TetradTest.cxx"
	"../src/GameBoard.cxx"
	"../src/Piece.cxx"
	"../src/Tetrad.cxx")

# The board reads its shapes through asset::Manager, which constructs every loader it has -
# so the whole set of libraries behind those loaders has to be here even though only the
# text one is exercised.
target_link_libraries(v3dtest_tetris PRIVATE
	v3dlib_asset
	v3dlib_audio
	v3dlib_font
	v3dlib_image
	v3dlib_log
	${Boost_LIBRARIES}
	${SDL3_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG
	Freetype::Freetype)
