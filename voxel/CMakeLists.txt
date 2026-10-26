add_executable(voxel
	"src/engine/Aligned.h"
	"src/engine/Camera.h" "src/engine/Camera.cxx"
	"src/engine/ChunkMeshBuilder.h" "src/engine/ChunkMeshBuilder.cxx"
	"src/engine/ChunkVertex.h"
	"src/engine/MaterialInfo.h"
	"src/engine/MortonCode.h" "src/engine/MortonCode.cxx"
	"src/engine/Nanoseconds.h" "src/engine/Nanoseconds.cxx"
	"src/engine/SceneUniforms.h"
	"src/game/GameState.h" "src/game/GameState.cxx"
	"src/game/Item.h" "src/game/Item.cxx"
	"src/game/Player.h" "src/game/Player.cxx"
	"src/noise/noiseutils.h" "src/noise/noiseutils.cpp"
	"src/voxel/Chunk.h" "src/voxel/Chunk.cxx"
	"src/voxel/ChunkCulling.h" "src/voxel/ChunkCulling.cxx"
	"src/voxel/ChunkMeshPool.h" "src/voxel/ChunkMeshPool.cxx"
	"src/voxel/FaceCulling.h" "src/voxel/FaceCulling.cxx"
	"src/voxel/MeshBuilder.h" "src/voxel/MeshBuilder.cxx"
	"src/voxel/MeshCache.h" "src/voxel/MeshCache.cxx"
	"src/voxel/TerrainMap.h" "src/voxel/TerrainMap.cxx"
	"src/voxel/Voxel.h" "src/voxel/Voxel.cxx"
	"src/voxel/World.h" "src/voxel/World.cxx"
	"src/Controller.h" "src/Controller.cxx"
	"src/Renderer.h" "src/Renderer.cxx"
	"src/Scene.h" "src/Scene.cxx"
	"src/Version.h" "src/main.cxx")

target_link_libraries(voxel PRIVATE
	v3dlib_asset
	v3dlib_config
	v3dlib_engine
	v3dlib_event
	v3dlib_input
	v3dlib_log
	v3dlib_render
	v3dlib_type
	v3dlib_shell
	v3dlib_ui
	v3dlib_window
	"libnoise"
)

# libnoise is voxel's alone. It is not a vcpkg package: it is built from source under
# vendor/libnoise, and link_directories expects its artefacts under vendor/libnoise/Debug.
target_include_directories(voxel SYSTEM PRIVATE "${V3D_ROOT}/vendor/libnoise/include/noise")
target_link_directories(voxel PRIVATE "${V3D_ROOT}/vendor/libnoise/Debug")

v3d_add_shader(voxel "shaders/voxel.vert")
v3d_add_shader(voxel "shaders/voxel.frag")

v3d_add_app_data(voxel)
v3d_add_shared_data(voxel)

# build for the windows subsystem. A console subsystem executable opens a console beside the
# app's window. SDL_main.h supplies the WinMain that calls main.
set_target_properties(voxel PROPERTIES WIN32_EXECUTABLE TRUE)

# noiseutils is Jason Bevins' libnoise utility layer, vendored verbatim. The tree's warning
# level and both analysers skip it, as cpplint does, because a local fix would be lost the next
# time it is taken from upstream. /analyze- takes it out of MSVC's analysis, and SKIP_LINTING
# takes it out of clang-tidy's. A suppression list would have to name every finding that each
# later version reports.
set_source_files_properties("src/noise/noiseutils.cpp" PROPERTIES SKIP_LINTING TRUE)
if(MSVC)
	set_source_files_properties("src/noise/noiseutils.cpp" PROPERTIES COMPILE_OPTIONS "/wd4100;/wd4189;/wd4244;/analyze-")
else()
	# C4100 and C4189 are an unused parameter and an unused variable. C4244, a narrowing
	# conversion, is not among -Wall and -Wextra
	set_source_files_properties("src/noise/noiseutils.cpp" PROPERTIES COMPILE_OPTIONS "-Wno-unused-parameter;-Wno-unused-variable")
endif()

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
