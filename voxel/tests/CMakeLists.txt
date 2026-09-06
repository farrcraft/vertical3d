# The meshing and terrain logic needs neither a window nor a device, so it is testable the
# same way an api library is. The renderer is not, and is not covered here - that waits on
# ADR 0007 like the rest of the device-side work.
v3d_add_test(voxel
	"TestMain.cxx"
	"ChunkTest.cxx"
	"FaceCullingTest.cxx"
	"MeshCacheTest.cxx"
	"MortonCodeTest.cxx"
	"../src/engine/MortonCode.cxx"
	"../src/noise/noiseutils.cpp"
	"../src/voxel/Chunk.cxx"
	"../src/voxel/FaceCulling.cxx"
	"../src/voxel/MeshCache.cxx"
	"../src/voxel/TerrainMap.cxx"
	"../src/voxel/Voxel.cxx")

# A source file property is scoped to the directory that sets it, so the exemption the app
# gives the vendored noise utility does not reach the suite that shares the file.
set_source_files_properties("../src/noise/noiseutils.cpp" PROPERTIES COMPILE_OPTIONS "/wd4100;/wd4189;/wd4244")

# libnoise reaches the suite the same way it reaches the app - named by the target that
# compiles its headers, rather than by the root for the whole tree.
target_include_directories(v3dtest_voxel SYSTEM PRIVATE "${V3D_ROOT}/vendor/libnoise/include/noise")
target_link_directories(v3dtest_voxel PRIVATE "${V3D_ROOT}/vendor/libnoise/Debug")

# A chunk is built against a TerrainMap, and the vtable of the one the tests derive from
# refers to the perlin implementation whether or not any test generates noise.
target_link_libraries(v3dtest_voxel PRIVATE
	${Boost_LIBRARIES}
	"libnoise")
