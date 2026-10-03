/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <voxel/src/engine/Camera.h>
#include <voxel/src/voxel/ChunkCulling.h>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

using v3d::type::geometry::Frustum;

namespace {

const float chunkSize = 16.0f;

/**
 * The frustum voxel culls against, from voxel's own camera at the origin looking down -z with
 * the projection Renderer::resize gives it.
 **/
Frustum view() {
    Camera camera;
    camera.perspective(90.0f, 1.0f, 0.1f, 1000.0f);
    return Frustum(camera.projection() * camera.view());
}

};  // namespace

BOOST_AUTO_TEST_SUITE(chunk_culling_test)

/**
 * A chunk's box runs from its corner to the far side of its last block.
 **/
BOOST_AUTO_TEST_CASE(a_chunk_spans_its_blocks) {
    const v3d::type::geometry::AABBox bounds = chunkBounds(glm::vec3(16.0f, 0.0f, -32.0f), chunkSize);

    BOOST_CHECK(bounds.min() == glm::vec3(16.0f, 0.0f, -32.0f));
    BOOST_CHECK(bounds.max() == glm::vec3(32.0f, 16.0f, -16.0f));
}

/**
 * A chunk ahead of the camera is drawn.
 **/
BOOST_AUTO_TEST_CASE(a_chunk_ahead_is_in_view) {
    BOOST_CHECK(chunkInView(view(), glm::vec3(-8.0f, -8.0f, -40.0f), chunkSize));
}

/**
 * A chunk behind the camera is not.
 **/
BOOST_AUTO_TEST_CASE(a_chunk_behind_is_culled) {
    BOOST_CHECK(!chunkInView(view(), glm::vec3(-8.0f, -8.0f, 24.0f), chunkSize));
}

/**
 * A chunk the camera stands inside crosses the near plane and is drawn - it is the one whose
 * near faces fill the screen.
 **/
BOOST_AUTO_TEST_CASE(a_chunk_across_the_near_plane_is_in_view) {
    BOOST_CHECK(chunkInView(view(), glm::vec3(-8.0f, -8.0f, -8.0f), chunkSize));
}

BOOST_AUTO_TEST_SUITE_END()
