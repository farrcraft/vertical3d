/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Registry.h>

#include <optional>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

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

/**
 * A released handle resolves to nothing at once, and hands back what it held so the caller
 * can destroy it.
 **/
BOOST_AUTO_TEST_CASE(a_released_handle_resolves_to_nothing) {
    v3d::render::realtime::Registry<struct TestTag, std::string> registry;
    v3d::render::realtime::Handle<struct TestTag> first = registry.add("first");
    v3d::render::realtime::Handle<struct TestTag> second = registry.add("second");

    std::optional<std::string> released = registry.release(first);

    BOOST_REQUIRE(released.has_value());
    BOOST_CHECK_EQUAL(*released, "first");
    BOOST_CHECK(registry.resolve(first) == nullptr);
    BOOST_REQUIRE(registry.resolve(second) != nullptr);
    BOOST_CHECK_EQUAL(*registry.resolve(second), "second");
    BOOST_CHECK_EQUAL(registry.count(), 1);
}

/**
 * A slot freed by a release is reused, and the old handle does not reach the new occupant
 * even though both name the same slot - the generation is what tells them apart.
 **/
BOOST_AUTO_TEST_CASE(a_reused_slot_refuses_the_old_handle) {
    v3d::render::realtime::Registry<struct TestTag, std::string> registry;
    v3d::render::realtime::Handle<struct TestTag> old = registry.add("old");
    registry.release(old);

    v3d::render::realtime::Handle<struct TestTag> reused = registry.add("new");

    BOOST_CHECK_EQUAL(reused.id(), old.id());
    BOOST_CHECK(reused != old);
    BOOST_CHECK(registry.resolve(old) == nullptr);
    BOOST_REQUIRE(registry.resolve(reused) != nullptr);
    BOOST_CHECK_EQUAL(*registry.resolve(reused), "new");
}

/**
 * Releasing a handle twice does nothing the second time, and in particular does not release
 * whatever has since moved into its slot.
 **/
BOOST_AUTO_TEST_CASE(a_second_release_is_a_no_op) {
    v3d::render::realtime::Registry<struct TestTag, std::string> registry;
    v3d::render::realtime::Handle<struct TestTag> old = registry.add("old");
    registry.release(old);
    v3d::render::realtime::Handle<struct TestTag> reused = registry.add("new");

    BOOST_CHECK(!registry.release(old).has_value());
    BOOST_CHECK(registry.resolve(reused) != nullptr);
    BOOST_CHECK_EQUAL(registry.count(), 1);
}

/**
 * Walking a registry with released slots visits only what it still holds, in slot order -
 * which is what a destructor walks to destroy.
 **/
BOOST_AUTO_TEST_CASE(a_walk_skips_released_slots) {
    v3d::render::realtime::Registry<struct TestTag, std::string> registry;
    registry.add("first");
    v3d::render::realtime::Handle<struct TestTag> second = registry.add("second");
    registry.add("third");
    registry.release(second);

    std::vector<std::string> walked;
    registry.each([&walked](const std::string& resource) { walked.push_back(resource); });

    BOOST_REQUIRE_EQUAL(walked.size(), 2);
    BOOST_CHECK_EQUAL(walked[0], "first");
    BOOST_CHECK_EQUAL(walked[1], "third");
}

/**
 * Clearing releases everything, so a handle from before the clear does not reach whatever is
 * added into its slot afterwards.
 **/
BOOST_AUTO_TEST_CASE(a_clear_refuses_every_earlier_handle) {
    v3d::render::realtime::Registry<struct TestTag, std::string> registry;
    v3d::render::realtime::Handle<struct TestTag> before = registry.add("before");

    registry.clear();
    v3d::render::realtime::Handle<struct TestTag> after = registry.add("after");

    BOOST_CHECK_EQUAL(registry.count(), 1);
    BOOST_CHECK(registry.resolve(before) == nullptr);
    BOOST_CHECK(registry.resolve(after) != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
