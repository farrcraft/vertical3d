/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/frame/Timings.h>

#include <cmath>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include "Headless.h"

using v3d::render::realtime::Frame;
using v3d::render::realtime::Pass;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;
using v3d::render::realtime::vulkan::frame::Timings;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 64;
const uint32_t height = 32;

};  // namespace

BOOST_AUTO_TEST_SUITE(timings_test)

/**
 * Two passes are timed under their names, in the order they were recorded, once the slot that
 * recorded them comes round again - and every span is a real duration.
 **/
BOOST_AUTO_TEST_CASE(each_pass_is_timed_under_its_name) {
    v3d::test::Headless headless(colourFormat, width, height);
    Timings& timings = headless.context->ring()->timings();
    if (!timings.enabled()) {
        BOOST_TEST_MESSAGE("the graphics queue writes no timestamps, so there is nothing to time");
        return;
    }

    boost::shared_ptr<RenderTarget> target =
        boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);
    Recorder::Target described;
    described.image = target->image();
    described.view = target->view();
    described.extent = target->extent();
    described.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    Frame frame;
    frame.pass("first")->clearColour(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    const boost::shared_ptr<Pass> second = frame.pass("second");
    second->keepColour();

    BOOST_CHECK(timings.last().empty());
    // a slot's spans are read when it is begun again, so one round of the ring and one more
    const uint32_t frames = headless.context->ring()->framesInFlight() + 1;
    for (uint32_t index = 0; index < frames; ++index) {
        VkCommandBuffer commands = headless.context->ring()->begin();
        Recorder::record(commands, frame, described, *headless.context->resources(),
            headless.context->frameUniforms().get(), &timings);
        headless.submitAndWait(commands);
    }

    const std::vector<Timings::Timing>& spans = timings.last();
    BOOST_REQUIRE_EQUAL(spans.size(), 2u);
    BOOST_CHECK_EQUAL(spans[0].name, "first");
    BOOST_CHECK_EQUAL(spans[1].name, "second");
    for (const Timings::Timing& span : spans) {
        BOOST_CHECK(std::isfinite(span.milliseconds));
        BOOST_CHECK(span.milliseconds >= 0.0);
        // a cleared 64 by 32 target, so anything near a second is a misread rather than a pass
        BOOST_CHECK(span.milliseconds < 1000.0);
    }
    BOOST_CHECK(headless.silent());
}

/**
 * A frame recorded with no timings is recorded as before, and the ring's timings read nothing
 * from it.
 **/
BOOST_AUTO_TEST_CASE(a_frame_recorded_without_timings_times_nothing) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target =
        boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);
    Recorder::Target described;
    described.image = target->image();
    described.view = target->view();
    described.extent = target->extent();
    described.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    Frame frame;
    frame.pass("colour");
    for (uint32_t index = 0; index < headless.context->ring()->framesInFlight() + 1; ++index) {
        VkCommandBuffer commands = headless.context->ring()->begin();
        Recorder::record(commands, frame, described, *headless.context->resources(), headless.context->frameUniforms().get());
        headless.submitAndWait(commands);
    }
    BOOST_CHECK(headless.context->ring()->timings().last().empty());
    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()
