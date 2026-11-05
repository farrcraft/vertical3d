add_executable(voxel
	"src/engine/CameraController.h" "src/engine/CameraController.cxx"
	"src/engine/ChunkMeshBuilder.h" "src/engine/ChunkMeshBuilder.cxx"
	"src/engine/ChunkVertex.h"
	"src/engine/MaterialInfo.h"
	"src/engine/MortonCode.h" "src/engine/MortonCode.cxx"
	"src/engine/Nanoseconds.h" "src/engine/Nanoseconds.cxx"
	"src/engine/SceneUniforms.h"
	"src/game/GameState.h" "src/game/GameState.cxx"
	"src/game/Item.h" "src/game/Item.cxx"
	"src/game/Player.h" "src/game/Player.cxx"
	"src/voxel/Chunk.h" "src/voxel/Chunk.cxx"
	"src/voxel/ChunkCulling.h" "src/voxel/ChunkCulling.cxx"
	"src/voxel/ChunkMeshPool.h" "src/voxel/ChunkMeshPool.cxx"
	"src/voxel/FaceCulling.h" "src/voxel/FaceCulling.cxx"
	"src/voxel/MeshBuilder.h" "src/voxel/MeshBuilder.cxx"
	"src/voxel/MeshCache.h" "src/voxel/MeshCache.cxx"
	"src/voxel/TerrainMap.h" "src/voxel/TerrainMap.cxx"
	"src/voxel/TerrainNoise.h" "src/voxel/TerrainNoise.cxx"
	"src/voxel/Voxel.h" "src/voxel/Voxel.cxx"
	"src/voxel/World.h" "src/voxel/World.cxx"
	"src/Controller.h" "src/Controller.cxx"
	"src/Renderer.h" "src/Renderer.cxx"
	"src/Scene.h" "src/Scene.cxx"
	"src/main.cxx")

target_link_libraries(voxel PRIVATE
	v3dlib_engine
	v3dlib_event
	v3dlib_input
	v3dlib_log
	v3dlib_render
	v3dlib_type
	v3dlib_shell
	v3dlib_ui
	v3dlib_version
	v3dlib_window
)

v3d_add_shader(voxel "shaders/voxel.vert")
v3d_add_shader(voxel "shaders/voxel.frag")

v3d_add_app_data(voxel)
v3d_add_app_icon(voxel "data/icon.svg")
v3d_install_app(voxel)
v3d_add_shared_data(voxel)

# build for the windows subsystem. A console subsystem executable opens a console beside the
# app's window. SDL_main.h supplies the WinMain that calls main.
set_target_properties(voxel PROPERTIES WIN32_EXECUTABLE TRUE)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
