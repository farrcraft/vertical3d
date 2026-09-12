/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Headless.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace v3d::test {

/**
 **/
bool deviceAvailable() {
    try {
        boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
        // no windowing extensions: what is being asked is whether anything can draw, not
        // whether anything can present
        boost::shared_ptr<render::realtime::vulkan::device::Instance> instance =
            boost::make_shared<render::realtime::vulkan::device::Instance>(logger, std::vector<const char*>());
        render::realtime::vulkan::device::Device device(logger, instance);
        return device.handle() != VK_NULL_HANDLE;
    } catch (const std::exception& error) {
        // the console rather than the logger: the logger writes to a file beside the
        // executable, and what this message answers is why a run skipped, which is read from
        // the run's output. A machine with no gpu and a machine whose loader cannot find the
        // one it has both end up here, and the two are told apart by what is printed
        std::cerr << "no device to draw with: " << error.what() << "\n";
        return false;
    }
}

/**
 **/
Headless::Headless(VkFormat colour, uint32_t width, uint32_t height) {
    // where a captured frame is written, beside the executable. The suites that render against
    // a committed reference do the same, and nothing creates it for them either
    boost::filesystem::create_directory("data_out");

    logger = boost::make_shared<v3d::log::Logger>();
    instance = boost::make_shared<render::realtime::vulkan::device::Instance>(logger, std::vector<const char*>());
    device = boost::make_shared<render::realtime::vulkan::device::Device>(logger, instance);
    context = boost::make_shared<render::realtime::DeviceContext>(logger, device, colour, VkExtent2D{width, height});
}

/**
 **/
Headless::~Headless() {
    // the context waits as it goes, but a case that threw may have left a submission running
    if (context) {
        context->ring()->waitIdle();
    }
}

/**
 **/
void Headless::submitAndWait(VkCommandBuffer commands) {
    VkResult result = vkEndCommandBuffer(commands);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Unable to end the test's command buffer");
    }

    VkCommandBufferSubmitInfo buffer{};
    buffer.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    buffer.commandBuffer = commands;

    VkSubmitInfo2 submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &buffer;

    result = vkQueueSubmit2(device->graphicsQueue(), 1, &submit, context->ring()->fence());
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Unable to submit the test's command buffer");
    }

    context->ring()->waitFrame();
    context->ring()->advance();
}

/**
 **/
bool Headless::silent() const {
    // the layer being on is half the assertion: where it is not installed nothing was watching,
    // and no errors reported reads exactly like a clean run
    if (!instance->validating()) {
        BOOST_TEST_MESSAGE("the validation layer is not installed - this case asserts nothing");
        return false;
    }
    if (instance->errors() > 0) {
        BOOST_TEST_MESSAGE("first validation error: " << instance->firstError());
    }
    return instance->errors() == 0;
}

};  // namespace v3d::test
