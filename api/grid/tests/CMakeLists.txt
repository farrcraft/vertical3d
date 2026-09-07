v3d_add_test(grid
	"TestMain.cpp"
	"TileGridTest.cpp"
	"PathfindingTest.cpp"
	"LineOfSightTest.cpp"
	"OverlayTest.cpp")

target_link_libraries(v3dtest_grid PRIVATE v3dlib_grid)
