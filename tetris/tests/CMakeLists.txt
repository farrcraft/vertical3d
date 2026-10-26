# The board and the tetrad need neither a window nor a device, so they are testable the same
# way an api library is. The controller is built so its commands can be checked against the
# bindings tetris ships, and is never started. The renderer is not covered here.
v3d_add_test(tetris
	"TestMain.cxx"
	"BindingsTest.cxx"
	"GameBoardTest.cxx"
	"TetradTest.cxx"
	"../src/Controller.cxx"
	"../src/GameBoard.cxx"
	"../src/Piece.cxx"
	"../src/Renderer.cxx"
	"../src/Tetrad.cxx"
	"../src/TetrisScene.cxx")

target_link_libraries(v3dtest_tetris PRIVATE
	v3dlib_asset
	v3dlib_asset_media
	v3dlib_config
	v3dlib_engine
	v3dlib_event
	v3dlib_image
	v3dlib_log
	v3dlib_render
	v3dlib_shell
	v3dlib_ui
	v3dlib_window)

v3d_copy_data(v3dtest_tetris app "${CMAKE_CURRENT_SOURCE_DIR}/../data")
