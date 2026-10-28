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
 * The kind of the allocator on the device the renderer's context built. It is read from the
 * allocator, not from anything the renderer stored, because the test checks that the choice
 * reaches the device.
 **/
Kind built(const v3d::test::Windowed& windowed) {
    return windowed.renderer->context()->device()->allocator().kind();
}

/**
 * A renderer asked for one allocation per resource builds that device, and draws through it.
 **/
BOOST_AUTO_TEST_CASE(window_renderer_builds_the_allocator_it_is_asked_for_test) {
    const v3d::test::Windowed windowed(Kind::Direct);
    BOOST_TEST((built(windowed) == Kind::Direct));

    windowed.renderer->renderFrame();
    windowed.renderer->renderFrame();
    BOOST_TEST(windowed.silent());
}

/**
 * A renderer that names no kind suballocates.
 **/
BOOST_AUTO_TEST_CASE(window_renderer_suballocates_by_default_test) {
    const v3d::test::Windowed windowed;
    BOOST_TEST((built(windowed) == Kind::Suballocated));
}

};  // namespace
