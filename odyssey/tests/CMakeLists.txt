# The map is where odyssey's own rules meet api/grid - what a tile kind means for
# passability and cover, and what the file format admits. It holds neither a window nor a
# device, so it is covered here; the engine and the renderer are not.
v3d_add_test(odyssey
	"TestMain.cxx"
	"MapTest.cxx"
	"../tile/Map.cpp")

target_link_libraries(v3dtest_odyssey PRIVATE
	v3dlib_asset
	v3dlib_grid
	v3dlib_log
	${Boost_LIBRARIES})
