# The map, sight and the world of regions are where odyssey's own rules meet api/grid. They
# decide what a tile kind means for passability and cover, what the file formats admit, how far
# sight reaches, and when a region is loaded and released. None holds a window nor a device, so
# all are covered here; the engine and the renderer are not.
v3d_add_test(odyssey
	"TestMain.cxx"
	"MapTest.cxx"
	"SightTest.cxx"
	"WorldTest.cxx"
	"../tile/Map.cpp"
	"../tile/Sight.cpp"
	"../tile/World.cpp")

target_link_libraries(v3dtest_odyssey PRIVATE
	v3dlib_asset
	v3dlib_grid
	v3dlib_log
	v3dlib_type)
