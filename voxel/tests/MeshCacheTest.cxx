/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <voxel/src/voxel/MeshCache.h>
#include <voxel/src/voxel/Voxel.h>

#include <cstddef>

#include <boost/test/unit_test.hpp>

#include <boost/make_shared.hpp>
#include <glm/geometric.hpp>

namespace {

/**
 * Room for one block with every face cut - six quads.
 **/
boost::shared_ptr<MeshCache> cache() {
    return boost::make_shared<MeshCache>(MeshCache::Capacity{.vertices = 24, .tris = 12, .faces = 6});
}

boost::shared_ptr<Voxel> block(const glm::vec3& position) {
    return boost::make_shared<Voxel>(Voxel::BLOCK_TYPE_STONE, position);
}

BOOST_AUTO_TEST_CASE(a_face_is_four_vertices_indexed_six_times_test) {
    // a face is four corners drawn as two triangles, so its six indices name four vertices.
    // Indices running 0, 1, 2, 3 and so on would carry no information
    const boost::shared_ptr<MeshCache> mesh = cache();
    mesh->extract(block(glm::vec3(0.0F, 0.0F, 0.0F)), Voxel::BLOCK_FACE_FRONT, glm::vec3(0.0F));

    BOOST_CHECK_EQUAL(mesh->vertexCount(), 4U);
    BOOST_CHECK_EQUAL(mesh->triCount(), 2U);
    BOOST_CHECK_EQUAL(mesh->faceCount(), 1U);

    // the two triangles share an edge, so both name that edge's two vertices
    const glm::ivec3 first = mesh->tris()[0];
    const glm::ivec3 second = mesh->tris()[1];
    BOOST_CHECK_EQUAL(first.x, second.x);
    BOOST_CHECK_EQUAL(first.z, second.y);
}

BOOST_AUTO_TEST_CASE(a_face_records_its_triangles_direction_and_type_test) {
    const boost::shared_ptr<MeshCache> mesh = cache();
    mesh->extract(block(glm::vec3(0.0F, 0.0F, 0.0F)), Voxel::BLOCK_FACE_TOP, glm::vec3(0.0F));

    // the vertex shader reads its normal and its material from the face
    const glm::ivec4 face = mesh->faces()[0];
    BOOST_CHECK_EQUAL(face.x, 0);
    BOOST_CHECK_EQUAL(face.y, 1);
    BOOST_CHECK_EQUAL(face.z, static_cast<int>(Voxel::BLOCK_FACE_TOP));
    BOOST_CHECK_EQUAL(face.w, static_cast<int>(Voxel::BLOCK_TYPE_STONE));
}

BOOST_AUTO_TEST_CASE(faces_are_wound_counter_clockwise_from_outside_test) {
    // counter clockwise winding lets the pipeline cull back faces. Because the camera's
    // projection flips y, the pipeline's front face setting is clockwise
    const boost::shared_ptr<MeshCache> mesh = cache();
    mesh->extract(block(glm::vec3(0.0F, 0.0F, 0.0F)), Voxel::BLOCK_FACE_FRONT, glm::vec3(0.0F));

    const glm::ivec3 tri = mesh->tris()[0];
    const glm::vec3* vertices = mesh->vertices();
    const glm::vec3 normal = glm::normalize(glm::cross(
        vertices[tri.y] - vertices[tri.x],
        vertices[tri.z] - vertices[tri.x]));

    // the front face of a block looks down positive z
    BOOST_CHECK_CLOSE(normal.z, 1.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(the_chunk_origin_is_subtracted_test) {
    // a chunk's geometry is built around its own corner, so that where it sits in the world
    // stays a push constant rather than something baked into every vertex
    const boost::shared_ptr<MeshCache> mesh = cache();
    const glm::vec3 origin(16.0F, 32.0F, 48.0F);
    mesh->extract(block(origin), Voxel::BLOCK_FACE_BOTTOM, origin);

    const glm::vec3* vertices = mesh->vertices();
    for (size_t i = 0; i < mesh->vertexCount(); i++) {
        BOOST_CHECK(vertices[i].x >= 0.0F && vertices[i].x <= 1.0F);
        BOOST_CHECK(vertices[i].y >= 0.0F && vertices[i].y <= 1.0F);
        BOOST_CHECK(vertices[i].z >= 0.0F && vertices[i].z <= 1.0F);
    }
}

BOOST_AUTO_TEST_CASE(a_block_with_no_visible_faces_contributes_nothing_test) {
    const boost::shared_ptr<MeshCache> mesh = cache();
    mesh->extract(block(glm::vec3(0.0F)), Voxel::BLOCK_FACE_NONE, glm::vec3(0.0F));
    BOOST_CHECK_EQUAL(mesh->vertexCount(), 0U);

    // air is never meshed, whatever faces it is offered
    mesh->extract(boost::make_shared<Voxel>(Voxel::BLOCK_TYPE_AIR, glm::vec3(0.0F)), Voxel::BLOCK_FACE_ALL, glm::vec3(0.0F));
    BOOST_CHECK_EQUAL(mesh->vertexCount(), 0U);
}

BOOST_AUTO_TEST_CASE(every_face_of_a_block_fits_the_cache_test) {
    const boost::shared_ptr<MeshCache> mesh = cache();
    mesh->extract(block(glm::vec3(0.0F)), Voxel::BLOCK_FACE_ALL, glm::vec3(0.0F));

    BOOST_CHECK_EQUAL(mesh->faceCount(), 6U);
    BOOST_CHECK_EQUAL(mesh->vertexCount(), 24U);
    BOOST_CHECK_EQUAL(mesh->triCount(), 12U);

    // a chunk is remeshed through reset, which keeps the allocation
    mesh->reset();
    BOOST_CHECK_EQUAL(mesh->faceCount(), 0U);
    BOOST_CHECK_EQUAL(mesh->vertexCount(), 0U);
    BOOST_CHECK_EQUAL(mesh->triCount(), 0U);
}

};  // namespace
