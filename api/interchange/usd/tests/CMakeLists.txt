# The USD reader, run against fixture files. None of them needs a window or a device.
v3d_add_test(interchange_usd
	"TestMain.cxx"
	"UsdTest.cxx")

target_link_libraries(v3dtest_interchange_usd PRIVATE v3dlib_interchange_usd v3dlib_interchange v3dlib_curve)
v3d_copy_usd_plugins(v3dtest_interchange_usd)

# The fixtures are the interchange suite's: make_scene_fixture.py writes scene.usda beside
# scene.glb, so both readers are checked against one scene. keyed_root.usda and curves.usda are
# written by hand.
v3d_copy_directory(v3dtest_interchange_usd fixtures "${CMAKE_CURRENT_SOURCE_DIR}/../../tests/data" "data")
