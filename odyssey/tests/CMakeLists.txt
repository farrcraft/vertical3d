# The map and what can be seen over it are where odyssey's own rules meet api/grid - what a
# tile kind means for passability and cover, what the file format admits, and how far sight
# reaches. Neither holds a window nor a device, so both are covered here; the engine and the
# renderer are not.
v3d_add_test(odyssey
	"TestMain.cxx"
	"MapTest.cxx"
	"SightTest.cxx"
	"../tile/Map.cpp"
	"../tile/Sight.cpp")

target_link_libraries(v3dtest_odyssey PRIVATE
	v3dlib_asset
	v3dlib_grid
	v3dlib_log
	${Boost_LIBRARIES})
