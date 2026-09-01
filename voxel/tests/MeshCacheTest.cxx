/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../src/voxel/MeshCache.h"
#include "../src/voxel/Voxel.h"

#include <boost/make_shared.hpp>
#include <glm/geometric.hpp>

namespace {

    /**
     * Room for one block with every face cut - six quads.
     **/
    boost::shared_ptr<MeshCache> cache() {
        return boost::make_shared<MeshCache>(24, 12, 6);
    }

    boost::shared_ptr<Voxel> block(const glm::vec3& position) {
        return boost::make_shared<Voxel>(Voxel::BLOCK_TYPE_STONE, position);
    }

};  // namespace

BOOST_AUTO_TEST_CASE(a_face_is_four_vertices_indexed_six_times_test) {
    // the defect this covers: a quad used to be six unique vertices with the indices running
    // 0, 1, 2, 3... - a quarter of the mesh carrying no information
    boost::shared_ptr<MeshCache> mesh = cache();
    mesh->extract(block(glm::vec3(0.0f, 0.0f, 0.0f)), Voxel::BLOCK_FACE_FRONT, glm::vec3(0.0f));

    BOOST_CHECK_EQUAL(mesh->vertexCount(), 4u);
    BOOST_CHECK_EQUAL(mesh->triCount(), 2u);
    BOOST_CHECK_EQUAL(mesh->faceCount(), 1u);

    // the two triangles share an edge, so between them they name one vertex twice
    const glm::ivec3 first = mesh->tris()[0];
    const glm::ivec3 second = mesh->tris()[1];
    BOOST_CHECK_EQUAL(first.x, second.x);
    BOOST_CHECK_EQUAL(first.z, second.y);
}

BOOST_AUTO_TEST_CASE(a_face_records_its_triangles_direction_and_type_test) {
    boost::shared_ptr<MeshCache> mesh = cache();
    mesh->extract(block(glm::vec3(0.0f, 0.0f, 0.0f)), Voxel::BLOCK_FACE_TOP, glm::vec3(0.0f));

    // the face is what the vertex shader reads its normal and its material out of
    const glm::ivec4 face = mesh->faces()[0];
    BOOST_CHECK_EQUAL(face.x, 0);
    BOOST_CHECK_EQUAL(face.y, 1);
    BOOST_CHECK_EQUAL(face.z, static_cast<int>(Voxel::BLOCK_FACE_TOP));
    BOOST_CHECK_EQUAL(face.w, static_cast<int>(Voxel::BLOCK_TYPE_STONE));
}

BOOST_AUTO_TEST_CASE(faces_are_wound_counter_clockwise_from_outside_test) {
    // which is what lets the pipeline cull back faces - and, because the camera's projection
    // flips y, what makes its front face setting clockwise
    boost::shared_ptr<MeshCache> mesh = cache();
    mesh->extract(block(glm::vec3(0.0f, 0.0f, 0.0f)), Voxel::BLOCK_FACE_FRONT, glm::vec3(0.0f));

    const glm::ivec3 tri = mesh->tris()[0];
    const glm::vec3* vertices = mesh->vertices();
    const glm::vec3 normal = glm::normalize(glm::cross(
        vertices[tri.y] - vertices[tri.x],
        vertices[tri.z] - vertices[tri.x]));

    // the front face of a block looks down positive z
    BOOST_CHECK_CLOSE(normal.z, 1.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(the_chunk_origin_is_subtracted_test) {
    // a chunk's geometry is built around its own corner, so that where it sits in the world
    // stays a push constant rather than something baked into every vertex
    boost::shared_ptr<MeshCache> mesh = cache();
    const glm::vec3 origin(16.0f, 32.0f, 48.0f);
    mesh->extract(block(origin), Voxel::BLOCK_FACE_BOTTOM, origin);

    const glm::vec3* vertices = mesh->vertices();
    for (size_t i = 0; i < mesh->vertexCount(); i++) {
        BOOST_CHECK(vertices[i].x >= 0.0f && vertices[i].x <= 1.0f);
        BOOST_CHECK(vertices[i].y >= 0.0f && vertices[i].y <= 1.0f);
        BOOST_CHECK(vertices[i].z >= 0.0f && vertices[i].z <= 1.0f);
    }
}

BOOST_AUTO_TEST_CASE(a_block_with_no_visible_faces_contributes_nothing_test) {
    boost::shared_ptr<MeshCache> mesh = cache();
    mesh->extract(block(glm::vec3(0.0f)), Voxel::BLOCK_FACE_NONE, glm::vec3(0.0f));
    BOOST_CHECK_EQUAL(mesh->vertexCount(), 0u);

    // air is never meshed, whatever faces it is offered
    mesh->extract(boost::make_shared<Voxel>(Voxel::BLOCK_TYPE_AIR, glm::vec3(0.0f)), Voxel::BLOCK_FACE_ALL, glm::vec3(0.0f));
    BOOST_CHECK_EQUAL(mesh->vertexCount(), 0u);
}

BOOST_AUTO_TEST_CASE(every_face_of_a_block_fits_the_cache_test) {
    boost::shared_ptr<MeshCache> mesh = cache();
    mesh->extract(block(glm::vec3(0.0f)), Voxel::BLOCK_FACE_ALL, glm::vec3(0.0f));

    BOOST_CHECK_EQUAL(mesh->faceCount(), 6u);
    BOOST_CHECK_EQUAL(mesh->vertexCount(), 24u);
    BOOST_CHECK_EQUAL(mesh->triCount(), 12u);

    // reset is what a chunk is remeshed through, and it keeps the allocation
    mesh->reset();
    BOOST_CHECK_EQUAL(mesh->faceCount(), 0u);
    BOOST_CHECK_EQUAL(mesh->vertexCount(), 0u);
    BOOST_CHECK_EQUAL(mesh->triCount(), 0u);
}
