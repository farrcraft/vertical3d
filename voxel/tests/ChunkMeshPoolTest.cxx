/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/memory/Mesh.h>
#include <voxel/src/voxel/ChunkMeshPool.h>

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec3.hpp>

using v3d::render::realtime::vulkan::memory::Mesh;

namespace {

/**
 * A mesh pointer whose lifetime is a counter's, so a case can watch it without a device. It
 * points at nothing, and the pool never dereferences what it holds.
 **/
boost::shared_ptr<Mesh> watched(const boost::shared_ptr<int>& owner) {
    return boost::shared_ptr<Mesh>(owner, static_cast<Mesh*>(nullptr));
}

BOOST_AUTO_TEST_SUITE(chunk_mesh_pool_test)

/**
 * Remeshing a chunk hands the old mesh to the retire function, which keeps it alive until it
 * runs what it was given. A frame in flight can still draw the old mesh until then.
 **/
BOOST_AUTO_TEST_CASE(a_replaced_mesh_is_retired_not_destroyed) {
    std::vector<std::function<void()>> retired;
    ChunkMeshPool pool([&retired](std::function<void()> destroy) { retired.push_back(std::move(destroy)); });

    const boost::shared_ptr<int> first = boost::make_shared<int>(1);
    const boost::shared_ptr<int> second = boost::make_shared<int>(2);
    pool.add(7, watched(first), glm::vec3(0.0F), 16.0F);
    BOOST_CHECK(retired.empty());

    pool.add(7, watched(second), glm::vec3(0.0F), 16.0F);
    BOOST_REQUIRE_EQUAL(retired.size(), 1U);
    // the pool's entry is gone, and the retire function holds the only other reference
    BOOST_CHECK_EQUAL(first.use_count(), 2);
    BOOST_CHECK_EQUAL(second.use_count(), 2);

    retired.front()();
    BOOST_CHECK_EQUAL(first.use_count(), 1);
}

/**
 * A chunk meshed for the first time retires nothing.
 **/
BOOST_AUTO_TEST_CASE(a_new_chunk_retires_nothing) {
    std::size_t retired = 0;
    ChunkMeshPool pool([&retired](const std::function<void()>&) { retired++; });

    const boost::shared_ptr<int> owner = boost::make_shared<int>(1);
    pool.add(1, watched(owner), glm::vec3(0.0F), 16.0F);
    pool.add(2, watched(owner), glm::vec3(16.0F, 0.0F, 0.0F), 16.0F);

    BOOST_CHECK_EQUAL(retired, 0U);
    BOOST_CHECK_EQUAL(pool.entries().size(), 2U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
