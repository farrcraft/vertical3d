# The meshing and terrain logic needs neither a window nor a device, so it is testable the
# same way an api library is. The mesh pool's hand-off of a replaced mesh is tested the same way.
# The controller is built so its commands can be checked against the bindings voxel ships, and
# is never started. The renderer needs a device and is not covered here.
v3d_add_test(voxel
	"TestMain.cxx"
	"BindingsTest.cxx"
	"CameraTest.cxx"
	"ChunkTest.cxx"
	"ChunkCullingTest.cxx"
	"ChunkMeshPoolTest.cxx"
	"FaceCullingTest.cxx"
	"MeshCacheTest.cxx"
	"MortonCodeTest.cxx"
	"NanosecondsTest.cxx"
	"PlayerTest.cxx"
	"../src/Controller.cxx"
	"../src/Renderer.cxx"
	"../src/Scene.cxx"
	"../src/engine/CameraController.cxx"
	"../src/engine/ChunkMeshBuilder.cxx"
	"../src/engine/MortonCode.cxx"
	"../src/engine/Nanoseconds.cxx"
	"../src/game/GameState.cxx"
	"../src/game/Item.cxx"
	"../src/game/Player.cxx"
	"../src/voxel/Chunk.cxx"
	"../src/voxel/ChunkCulling.cxx"
	"../src/voxel/ChunkMeshPool.cxx"
	"../src/voxel/FaceCulling.cxx"
	"../src/voxel/MeshBuilder.cxx"
	"../src/voxel/MeshCache.cxx"
	"../src/voxel/TerrainMap.cxx"
	"../src/voxel/TerrainNoise.cxx"
	"../src/voxel/Voxel.cxx"
	"../src/voxel/World.cxx")

# A chunk is built against a TerrainMap, whose noise comes from glm through the type library.
target_link_libraries(v3dtest_voxel PRIVATE
	v3dlib_type)

# The mesh pool holds the render library's meshes. The controller needs the rest of what the
# app links.
target_link_libraries(v3dtest_voxel PRIVATE
	v3dlib_engine
	v3dlib_event
	v3dlib_input
	v3dlib_log
	v3dlib_render
	v3dlib_shell
	v3dlib_ui
	v3dlib_version
	v3dlib_window)

v3d_copy_data(v3dtest_voxel app "${CMAKE_CURRENT_SOURCE_DIR}/../data")

# the renderer the controller holds embeds the app's shaders
v3d_add_shader(v3dtest_voxel "../shaders/voxel.vert")
v3d_add_shader(v3dtest_voxel "../shaders/voxel.frag")
