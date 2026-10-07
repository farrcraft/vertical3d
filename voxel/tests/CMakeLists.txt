# The meshing and terrain logic needs neither a window nor a device, so it is testable the
# same way an api library is. The renderer needs a device and is not covered here.
v3d_add_test(voxel
	"TestMain.cxx"
	"ChunkTest.cxx"
	"ChunkCullingTest.cxx"
	"FaceCullingTest.cxx"
	"MeshCacheTest.cxx"
	"MortonCodeTest.cxx"
	"NanosecondsTest.cxx"
	"PlayerTest.cxx"
	"../src/engine/Camera.cxx"
	"../src/engine/MortonCode.cxx"
	"../src/engine/Nanoseconds.cxx"
	"../src/game/Item.cxx"
	"../src/game/Player.cxx"
	"../src/noise/noiseutils.cpp"
	"../src/voxel/Chunk.cxx"
	"../src/voxel/ChunkCulling.cxx"
	"../src/voxel/FaceCulling.cxx"
	"../src/voxel/MeshCache.cxx"
	"../src/voxel/TerrainMap.cxx"
	"../src/voxel/Voxel.cxx")

# A source file property is scoped to the directory that sets it, so the exemption the app
# gives the vendored noise utility does not reach the suite that shares the file.
set_source_files_properties("../src/noise/noiseutils.cpp" PROPERTIES
	COMPILE_OPTIONS "/wd4100;/wd4189;/wd4244;/analyze-"
	SKIP_LINTING TRUE)

# libnoise reaches the suite the same way it reaches the app - named by the target that
# compiles its headers, rather than by the root for the whole tree.
target_include_directories(v3dtest_voxel SYSTEM PRIVATE "${V3D_ROOT}/vendor/libnoise/include/noise")
target_link_directories(v3dtest_voxel PRIVATE "${V3D_ROOT}/vendor/libnoise/Debug")

# A chunk is built against a TerrainMap, and the vtable of the one the tests derive from
# refers to the perlin implementation whether or not any test generates noise.
target_link_libraries(v3dtest_voxel PRIVATE
	v3dlib_type
	"libnoise")
