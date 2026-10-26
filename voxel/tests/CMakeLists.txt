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
	"../src/engine/Camera.cxx"
	"../src/engine/ChunkMeshBuilder.cxx"
	"../src/engine/MortonCode.cxx"
	"../src/engine/Nanoseconds.cxx"
	"../src/game/GameState.cxx"
	"../src/game/Item.cxx"
	"../src/game/Player.cxx"
	"../src/noise/noiseutils.cpp"
	"../src/voxel/Chunk.cxx"
	"../src/voxel/ChunkCulling.cxx"
	"../src/voxel/ChunkMeshPool.cxx"
	"../src/voxel/FaceCulling.cxx"
	"../src/voxel/MeshBuilder.cxx"
	"../src/voxel/MeshCache.cxx"
	"../src/voxel/TerrainMap.cxx"
	"../src/voxel/Voxel.cxx"
	"../src/voxel/World.cxx")

# A source file property is scoped to the directory that sets it, so the exemption the app
# gives the vendored noise utility does not reach the suite that shares the file.
set_source_files_properties("../src/noise/noiseutils.cpp" PROPERTIES SKIP_LINTING TRUE)
if(MSVC)
	set_source_files_properties("../src/noise/noiseutils.cpp" PROPERTIES COMPILE_OPTIONS "/wd4100;/wd4189;/wd4244;/analyze-")
else()
	set_source_files_properties("../src/noise/noiseutils.cpp" PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter;-Wno-unused-variable")
endif()

# The suite names libnoise's include and link directories itself, as the app does. The root
# does not name them for the whole tree.
target_include_directories(v3dtest_voxel SYSTEM PRIVATE "${V3D_ROOT}/vendor/libnoise/include/noise")
target_link_directories(v3dtest_voxel PRIVATE "${V3D_ROOT}/vendor/libnoise/Debug")

# A chunk is built against a TerrainMap. The vtable of the TerrainMap the tests derive from
# refers to the perlin implementation, so the suite links libnoise even where no test generates
# noise.
target_link_libraries(v3dtest_voxel PRIVATE
	v3dlib_type
	"libnoise")

# The mesh pool holds the render library's meshes. The controller needs the rest of what the
# app links.
target_link_libraries(v3dtest_voxel PRIVATE
	v3dlib_asset
	v3dlib_config
	v3dlib_engine
	v3dlib_event
	v3dlib_input
	v3dlib_log
	v3dlib_render
	v3dlib_shell
	v3dlib_ui
	v3dlib_window)

v3d_copy_data(v3dtest_voxel app "${CMAKE_CURRENT_SOURCE_DIR}/../data")

# the renderer the controller holds embeds the app's shaders
v3d_add_shader(v3dtest_voxel "../shaders/voxel.vert")
v3d_add_shader(v3dtest_voxel "../shaders/voxel.frag")
