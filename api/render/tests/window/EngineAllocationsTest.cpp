/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/memory/Allocator.h>

#include <boost/test/unit_test.hpp>

#include "Windowed.h"

namespace {

using Kind = v3d::render::realtime::vulkan::memory::Allocator::Kind;

/**
 * The kind of the allocator on the device the engine's context built. It is read from the
 * allocator, not from anything the engine stored, because the test checks that the choice
 * reaches the device.
 **/
Kind built(const v3d::test::Windowed& windowed) {
    return windowed.engine->context()->device()->allocator().kind();
}

/**
 * An engine asked for a suballocating device builds one, and draws through it.
 **/
BOOST_AUTO_TEST_CASE(engine3d_builds_the_allocator_it_is_asked_for_test) {
    const v3d::test::Windowed windowed(Kind::Suballocated);
    BOOST_TEST((built(windowed) == Kind::Suballocated));

    windowed.engine->renderFrame();
    windowed.engine->renderFrame();
    BOOST_TEST(windowed.silent());
}

/**
 * An engine that names no kind builds one allocation per resource.
 **/
BOOST_AUTO_TEST_CASE(engine3d_allocates_directly_by_default_test) {
    const v3d::test::Windowed windowed;
    BOOST_TEST((built(windowed) == Kind::Direct));
}

};  // namespace
