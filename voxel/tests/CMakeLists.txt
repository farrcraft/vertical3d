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

# A chunk is built against a TerrainMap, and the vtable of the one the tests derive from
# refers to the perlin implementation whether or not any test generates noise.
target_link_libraries(v3dtest_voxel PRIVATE
	${Boost_LIBRARIES}
	"libnoise")
