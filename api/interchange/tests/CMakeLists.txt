# The interchange readers, run against fixture files. None of them needs a window or a device.
v3d_add_test(interchange
	"TestMain.cxx"
	"GltfTest.cxx")

target_link_libraries(v3dtest_interchange PRIVATE v3dlib_interchange)

# make_scene_fixture.py generates scene.glb with Blender. nodes.gltf is written by hand.
add_custom_command(TARGET v3dtest_interchange POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_interchange>/data"
	COMMENT "Copying interchange test fixtures"
	VERBATIM)
