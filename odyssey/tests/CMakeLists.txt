# The map, sight and the world of regions are where odyssey's own rules meet api/grid. They
# decide what a tile kind means for passability and cover, what the file formats admit, how far
# sight reaches, and when a region is loaded and released. None holds a window or a device, so
# all are covered here. The engine is built so its commands can be checked against the
# bindings odyssey ships, and is never started. The renderer is not covered.
v3d_add_test(odyssey
	"TestMain.cxx"
	"BindingsTest.cxx"
	"MapTest.cxx"
	"SightTest.cxx"
	"WorldTest.cxx"
	"../engine/Engine.cpp"
	"../engine/Player.cpp"
	"../render/Renderer.cpp"
	"../system/Movement.cpp"
	"../tile/Map.cpp"
	"../tile/Sight.cpp"
	"../tile/World.cpp"
	"../ui/Screen.cpp")

target_link_libraries(v3dtest_odyssey PRIVATE
	v3dlib_asset
	v3dlib_asset_media
	v3dlib_config
	v3dlib_ecs
	v3dlib_engine
	v3dlib_event
	v3dlib_grid
	v3dlib_input
	v3dlib_log
	v3dlib_render
	v3dlib_type)

v3d_copy_data(v3dtest_odyssey app "${CMAKE_CURRENT_SOURCE_DIR}/../data")
