add_executable(voxel
	"src/engine/Aligned.h"
	"src/engine/Camera.h" "src/engine/Camera.cxx"
	"src/engine/ChunkMeshBuilder.h" "src/engine/ChunkMeshBuilder.cxx"
	"src/engine/MortonCode.h" "src/engine/MortonCode.cxx"
	"src/engine/SceneUniforms.h"
	"src/game/GameState.h" "src/game/GameState.cxx"
	"src/game/Item.h" "src/game/Item.cxx"
	"src/game/Player.h" "src/game/Player.cxx"
	"src/noise/noiseutils.h" "src/noise/noiseutils.cpp"
	"src/voxel/Chunk.h" "src/voxel/Chunk.cxx"
	"src/voxel/ChunkMeshPool.h" "src/voxel/ChunkMeshPool.cxx"
	"src/voxel/FaceCulling.h" "src/voxel/FaceCulling.cxx"
	"src/voxel/MeshBuilder.h" "src/voxel/MeshBuilder.cxx"
	"src/voxel/MeshCache.h" "src/voxel/MeshCache.cxx"
	"src/voxel/TerrainMap.h" "src/voxel/TerrainMap.cxx"
	"src/voxel/Voxel.h" "src/voxel/Voxel.cxx"
	"src/voxel/World.h" "src/voxel/World.cxx"
	"src/Controller.h" "src/Controller.cxx"
	"src/DebugOverlay.h" "src/DebugOverlay.cxx"
	"src/Renderer.h" "src/Renderer.cxx"
	"src/Scene.h" "src/Scene.cxx"
	"src/Version.h" "src/main.cxx")

target_link_libraries(voxel PRIVATE
	v3dlib_asset
	v3dlib_config
	v3dlib_ecs
	v3dlib_engine
	v3dlib_event
	v3dlib_font
	v3dlib_image
	v3dlib_input
	v3dlib_log
	v3dlib_render
	v3dlib_type
	v3dlib_ui
	${Boost_LIBRARIES}
	${SDL3_LIBRARIES}
	${Vulkan_LIBRARIES}
	${PNG_LIBRARIES}
	JPEG::JPEG
	Freetype::Freetype
	"libnoise"
)

v3d_add_shader(voxel "shaders/voxel.vert")
v3d_add_shader(voxel "shaders/voxel.frag")

v3d_add_app_data(voxel)
v3d_add_shared_data(voxel)

# a console subsystem executable is given a console of its own alongside the app's window.
# main is reached through the WinMain that SDL_main.h supplies.
set_target_properties(voxel PROPERTIES WIN32_EXECUTABLE TRUE)

target_compile_options(voxel PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

add_subdirectory("tests")
