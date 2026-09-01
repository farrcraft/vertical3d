/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../src/engine/MortonCode.h"
#include "../src/voxel/Chunk.h"

namespace {

    /**
     * A heightmap of one value, so that what a chunk fills to is a function of the ceiling
     * it was given and nothing else.
     **/
    class FlatTerrain final : public TerrainMap {
     public:
        explicit FlatTerrain(float height) : height_(height) {
        }

        float height(unsigned int x, unsigned int z) override {
            static_cast<void>(x);
            static_cast<void>(z);
            return height_;
        }

     private:
        float height_;
    };

    const unsigned int chunkSize = 16;

    /**
     * The world voxel builds: 256 wide, 256 deep and 64 tall, in chunks of 16.
     **/
    const unsigned int worldHeightInBlocks = 64;

};  // namespace

BOOST_AUTO_TEST_CASE(chunk_ceiling_is_measured_in_blocks_test) {
    // the ceiling is in blocks, not chunks. A heightmap is scaled against it, so passing the
    // world's chunk count instead flattens terrain in a 64 block world to four blocks and
    // leaves every chunk above the first empty
    FlatTerrain terrain(255.0f);

    Chunk full(&terrain, glm::ivec3(0, 1, 0), worldHeightInBlocks);
    BOOST_CHECK(!full.empty());
    BOOST_CHECK_EQUAL(full.blocks().size(), chunkSize * chunkSize * chunkSize);

    // the chunk count is what was passed before, and it leaves the same chunk empty
    Chunk starved(&terrain, glm::ivec3(0, 1, 0), worldHeightInBlocks / chunkSize);
    BOOST_CHECK(starved.empty());
    BOOST_CHECK_EQUAL(starved.blocks().size(), 0u);
}

BOOST_AUTO_TEST_CASE(chunk_column_scales_with_the_heightmap_test) {
    // half height over a 64 block ceiling is 32 blocks, so the chunk holding blocks 32 to 47
    // gets its bottom layer and nothing above it
    FlatTerrain terrain(128.0f);

    Chunk below(&terrain, glm::ivec3(0, 0, 0), worldHeightInBlocks);
    BOOST_CHECK_EQUAL(below.blocks().size(), chunkSize * chunkSize * chunkSize);

    Chunk edge(&terrain, glm::ivec3(0, 2, 0), worldHeightInBlocks);
    BOOST_CHECK_EQUAL(edge.blocks().size(), chunkSize * chunkSize);

    Chunk above(&terrain, glm::ivec3(0, 3, 0), worldHeightInBlocks);
    BOOST_CHECK(above.empty());
}

BOOST_AUTO_TEST_CASE(chunk_hides_faces_between_its_own_blocks_test) {
    FlatTerrain terrain(255.0f);
    Chunk chunk(&terrain, glm::ivec3(0, 0, 0), worldHeightInBlocks);

    // a block surrounded by its own chunk contributes no geometry at all
    const glm::ivec3 interior(8, 8, 8);
    BOOST_CHECK(chunk.hidden(Voxel::BLOCK_FACE_LEFT, interior));
    BOOST_CHECK(chunk.hidden(Voxel::BLOCK_FACE_RIGHT, interior));
    BOOST_CHECK(chunk.hidden(Voxel::BLOCK_FACE_TOP, interior));
    BOOST_CHECK(chunk.hidden(Voxel::BLOCK_FACE_BOTTOM, interior));
    BOOST_CHECK(chunk.hidden(Voxel::BLOCK_FACE_FRONT, interior));
    BOOST_CHECK(chunk.hidden(Voxel::BLOCK_FACE_BACK, interior));

    // one on the wall has a face its own chunk cannot answer for, which is what sends the
    // check across the seam
    BOOST_CHECK(!chunk.hidden(Voxel::BLOCK_FACE_LEFT, glm::ivec3(0, 8, 8)));
    BOOST_CHECK(!chunk.hidden(Voxel::BLOCK_FACE_RIGHT, glm::ivec3(15, 8, 8)));
}

BOOST_AUTO_TEST_CASE(chunk_positions_its_blocks_in_the_world_test) {
    FlatTerrain terrain(255.0f);
    const glm::ivec3 position(2, 1, 3);
    Chunk chunk(&terrain, position, worldHeightInBlocks);

    // the block at the chunk's own corner sits at the corner of the chunk in world blocks,
    // which is what MeshCache subtracts back off to make the mesh chunk local
    MortonCode codec;
    const boost::shared_ptr<Voxel> corner = chunk.blocks()[codec.encode(glm::ivec3(0, 0, 0))];
    BOOST_REQUIRE(corner);
    BOOST_CHECK_EQUAL(corner->position().x, static_cast<float>(position.x * chunkSize));
    BOOST_CHECK_EQUAL(corner->position().y, static_cast<float>(position.y * chunkSize));
    BOOST_CHECK_EQUAL(corner->position().z, static_cast<float>(position.z * chunkSize));
}
