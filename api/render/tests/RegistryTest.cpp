/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>

#include "../realtime/Handle.h"
#include "../realtime/Registry.h"

BOOST_AUTO_TEST_SUITE(registry_test)

/**
 * A handle nobody has filled in refers to nothing, so a draw item that never named a
 * material can be told apart from one that named the first one registered.
 **/
BOOST_AUTO_TEST_CASE(a_default_handle_is_invalid) {
    v3d::render::realtime::TextureHandle handle;

    BOOST_CHECK(!handle.valid());
    BOOST_CHECK(handle != v3d::render::realtime::TextureHandle(0));
}

/**
 * Handles order by slot, which is what a sort key is built out of.
 **/
BOOST_AUTO_TEST_CASE(handles_order_by_slot) {
    v3d::render::realtime::PipelineHandle first(0);
    v3d::render::realtime::PipelineHandle second(1);

    BOOST_CHECK(first < second);
    BOOST_CHECK(first == v3d::render::realtime::PipelineHandle(0));
}

/**
 * Adding hands back a handle that resolves to what was added.
 **/
BOOST_AUTO_TEST_CASE(a_registered_resource_resolves) {
    v3d::render::realtime::Registry<struct TestTag, std::string> registry;

    v3d::render::realtime::Handle<struct TestTag> first = registry.add("first");
    v3d::render::realtime::Handle<struct TestTag> second = registry.add("second");

    BOOST_CHECK_EQUAL(registry.count(), 2);
    BOOST_REQUIRE(registry.resolve(first) != nullptr);
    BOOST_CHECK_EQUAL(*registry.resolve(first), "first");
    BOOST_CHECK_EQUAL(*registry.resolve(second), "second");
}

/**
 * Slots are handed out in order, so a handle's id is also its sort order.
 **/
BOOST_AUTO_TEST_CASE(slots_are_handed_out_in_order) {
    v3d::render::realtime::Registry<struct TestTag, std::string> registry;

    BOOST_CHECK_EQUAL(registry.add("first").id(), 0u);
    BOOST_CHECK_EQUAL(registry.add("second").id(), 1u);
    BOOST_CHECK_EQUAL(registry.add("third").id(), 2u);
}

/**
 * Resolving something that was never registered gives nothing rather than a stale
 * reference - a recorder walking a frame full of items has to be able to skip one.
 **/
BOOST_AUTO_TEST_CASE(an_unknown_handle_resolves_to_nothing) {
    v3d::render::realtime::Registry<struct TestTag, std::string> registry;
    registry.add("first");

    BOOST_CHECK(registry.resolve(v3d::render::realtime::Handle<struct TestTag>()) == nullptr);
    BOOST_CHECK(registry.resolve(v3d::render::realtime::Handle<struct TestTag>(7)) == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
