/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../src/voxel/FaceCulling.h"

namespace {

    const int chunkSize = 16;
    const int high = chunkSize - 1;

};  // namespace

BOOST_AUTO_TEST_CASE(seam_lookup_moves_the_block_as_well_as_the_chunk_test) {
    // the defect this covers: the three faces that look out the high side of a chunk moved
    // the chunk they looked in but left the block where it was, so they asked whether the
    // block at 15 of the next chunk along was solid when the one against the face is at 0
    const glm::ivec3 chunk(1, 1, 1);

    const Neighbour right = neighbourAcrossSeam(Voxel::BLOCK_FACE_RIGHT, glm::ivec3(high, 4, 4), chunk, chunkSize);
    BOOST_REQUIRE(right.crosses);
    BOOST_CHECK_EQUAL(right.chunk.x, 2);
    BOOST_CHECK_EQUAL(right.block.x, 0);

    const Neighbour front = neighbourAcrossSeam(Voxel::BLOCK_FACE_FRONT, glm::ivec3(4, 4, high), chunk, chunkSize);
    BOOST_REQUIRE(front.crosses);
    BOOST_CHECK_EQUAL(front.chunk.z, 2);
    BOOST_CHECK_EQUAL(front.block.z, 0);

    const Neighbour top = neighbourAcrossSeam(Voxel::BLOCK_FACE_TOP, glm::ivec3(4, high, 4), chunk, chunkSize);
    BOOST_REQUIRE(top.crosses);
    BOOST_CHECK_EQUAL(top.chunk.y, 2);
    BOOST_CHECK_EQUAL(top.block.y, 0);
}

BOOST_AUTO_TEST_CASE(seam_lookup_from_the_low_side_test) {
    const glm::ivec3 chunk(1, 1, 1);

    const Neighbour left = neighbourAcrossSeam(Voxel::BLOCK_FACE_LEFT, glm::ivec3(0, 4, 4), chunk, chunkSize);
    BOOST_REQUIRE(left.crosses);
    BOOST_CHECK_EQUAL(left.chunk.x, 0);
    BOOST_CHECK_EQUAL(left.block.x, high);

    const Neighbour back = neighbourAcrossSeam(Voxel::BLOCK_FACE_BACK, glm::ivec3(4, 4, 0), chunk, chunkSize);
    BOOST_REQUIRE(back.crosses);
    BOOST_CHECK_EQUAL(back.chunk.z, 0);
    BOOST_CHECK_EQUAL(back.block.z, high);

    const Neighbour bottom = neighbourAcrossSeam(Voxel::BLOCK_FACE_BOTTOM, glm::ivec3(4, 0, 4), chunk, chunkSize);
    BOOST_REQUIRE(bottom.crosses);
    BOOST_CHECK_EQUAL(bottom.chunk.y, 0);
    BOOST_CHECK_EQUAL(bottom.block.y, high);
}

BOOST_AUTO_TEST_CASE(seam_lookup_leaves_the_other_axes_alone_test) {
    const Neighbour right = neighbourAcrossSeam(Voxel::BLOCK_FACE_RIGHT, glm::ivec3(high, 7, 3), glm::ivec3(1, 2, 3), chunkSize);
    BOOST_REQUIRE(right.crosses);
    BOOST_CHECK_EQUAL(right.chunk.y, 2);
    BOOST_CHECK_EQUAL(right.chunk.z, 3);
    BOOST_CHECK_EQUAL(right.block.y, 7);
    BOOST_CHECK_EQUAL(right.block.z, 3);
}

BOOST_AUTO_TEST_CASE(an_interior_face_crosses_no_seam_test) {
    const glm::ivec3 chunk(1, 1, 1);
    const glm::ivec3 block(4, 4, 4);

    BOOST_CHECK(!neighbourAcrossSeam(Voxel::BLOCK_FACE_LEFT, block, chunk, chunkSize).crosses);
    BOOST_CHECK(!neighbourAcrossSeam(Voxel::BLOCK_FACE_RIGHT, block, chunk, chunkSize).crosses);
    BOOST_CHECK(!neighbourAcrossSeam(Voxel::BLOCK_FACE_TOP, block, chunk, chunkSize).crosses);
    BOOST_CHECK(!neighbourAcrossSeam(Voxel::BLOCK_FACE_BOTTOM, block, chunk, chunkSize).crosses);
}

BOOST_AUTO_TEST_CASE(the_edge_of_the_world_has_no_chunk_beyond_it_test) {
    // there is no chunk at minus one, so those faces are drawn rather than looked up
    const glm::ivec3 origin(0, 0, 0);

    BOOST_CHECK(!neighbourAcrossSeam(Voxel::BLOCK_FACE_LEFT, glm::ivec3(0, 4, 4), origin, chunkSize).crosses);
    BOOST_CHECK(!neighbourAcrossSeam(Voxel::BLOCK_FACE_BACK, glm::ivec3(4, 4, 0), origin, chunkSize).crosses);
    BOOST_CHECK(!neighbourAcrossSeam(Voxel::BLOCK_FACE_BOTTOM, glm::ivec3(4, 0, 4), origin, chunkSize).crosses);
}
