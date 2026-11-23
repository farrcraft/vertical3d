# The interchange readers, run against fixture files. None of them needs a window or a device.
v3d_add_test(interchange
	"TestMain.cxx"
	"GltfTest.cxx"
	"RescaleTest.cxx")

target_link_libraries(v3dtest_interchange PRIVATE v3dlib_interchange v3dlib_curve)

# make_scene_fixture.py generates scene.glb with Blender. nodes.gltf is written by hand.
v3d_copy_directory(v3dtest_interchange fixtures "${CMAKE_CURRENT_SOURCE_DIR}/data" "data")
