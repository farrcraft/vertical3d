/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Presenter.h"

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Result.h>

#include "Ring.h"
#include "Swapchain.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 **/
Presenter::Acquisition::Acquisition() noexcept :
image(0),
commands(VK_NULL_HANDLE) {
}

/**
 **/
Presenter::Presenter(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Swapchain>& swapchain,
    const boost::shared_ptr<Ring>& ring) :
    logger_(logger),
    device_(ring->device()),
    swapchain_(swapchain),
    ring_(ring),
    suboptimal_(false) {
    try {
        createSync();
    } catch (...) {
        // nothing runs the destructor of an object whose constructor threw, so the semaphores
        // made before the failure are destroyed here
        destroySync();
        throw;
    }
}

/**
 **/
Presenter::~Presenter() {
    // no submission may still use a semaphore when it is destroyed. The Ring destroys its own
    // fences, and outlives the Presenter because the Presenter holds it
    ring_->waitIdleNoThrow();
    destroySync();
}

/**
 **/
void Presenter::destroySync() noexcept {
    destroyImageSync();
    for (VkSemaphore semaphore : imageAvailable_) {
        vkDestroySemaphore(device_->handle(), semaphore, nullptr);
    }
    imageAvailable_.clear();
}

/**
 **/
void Presenter::createSync() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // one per frame in flight, not per swapchain image. The acquire needs the semaphore before
    // it returns the image index, so the Ring's frame() indexes it
    for (uint32_t index = 0; index < ring_->framesInFlight(); index++) {
        VkSemaphore semaphore = VK_NULL_HANDLE;
        const VkResult result = vkCreateSemaphore(device_->handle(), &semaphoreInfo, nullptr, &semaphore);
        device::check(result, "Unable to create a vulkan semaphore");
        imageAvailable_.push_back(semaphore);
    }

    reset();
}

/**
 **/
void Presenter::destroyImageSync() noexcept {
    for (VkSemaphore semaphore : renderFinished_) {
        vkDestroySemaphore(device_->handle(), semaphore, nullptr);
    }
    renderFinished_.clear();
}

/**
 **/
void Presenter::reset() {
    destroyImageSync();

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (std::size_t index = 0; index < swapchain_->length(); index++) {
        VkSemaphore semaphore = VK_NULL_HANDLE;
        const VkResult result = vkCreateSemaphore(device_->handle(), &semaphoreInfo, nullptr, &semaphore);
        device::check(result, "Unable to create a vulkan semaphore");
        renderFinished_.push_back(semaphore);
    }

    suboptimal_ = false;
}

/**
 **/
boost::shared_ptr<Ring> Presenter::ring() const noexcept {
    return ring_;
}

/**
 **/
Presenter::Status Presenter::acquire(Acquisition* acquisition) {
    // a window with no area has no swapchain to draw into
    if (!swapchain_->valid() || renderFinished_.empty()) {
        return Status::Skip;
    }

    // wait before acquiring, not in begin(). The acquire signals this frame's semaphore, and
    // that semaphore may still be in use by the frame's last submission until it completes.
    // begin() waits again, but a signalled fence returns at once. Only the submit unsignals
    // the fence, so an OutOfDate return below leaves the ring unchanged
    ring_->waitFrame();

    uint32_t image = 0;
    const VkResult result = vkAcquireNextImageKHR(device_->handle(), swapchain_->handle(), UINT64_MAX, imageAvailable_[ring_->frame()],
        VK_NULL_HANDLE, &image);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // the semaphore was not signalled, so nothing is left waiting by giving up here
        return Status::OutOfDate;
    }
    device::check(result, "Unable to acquire a vulkan swapchain image", VK_SUBOPTIMAL_KHR);
    // a suboptimal image can still be drawn and presented. present() then returns OutOfDate
    suboptimal_ = result == VK_SUBOPTIMAL_KHR;

    acquisition->image = image;
    acquisition->commands = ring_->begin();
    return Status::Ready;
}

/**
 **/
Presenter::Status Presenter::present(const Acquisition& acquisition) {
    VkResult result = vkEndCommandBuffer(acquisition.commands);
    device::check(result, "Unable to end a vulkan command buffer");

    VkSemaphoreSubmitInfo wait{};
    wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    wait.semaphore = imageAvailable_[ring_->frame()];
    // nothing before the colour attachment write needs the image, so only that stage waits
    wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo signal{};
    signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signal.semaphore = renderFinished_[acquisition.image];
    // ALL_COMMANDS, not COLOR_ATTACHMENT_OUTPUT. The command buffer ends with the layout
    // transition into PRESENT_SRC, whose destination scope is the bottom of the pipe. A
    // semaphore signalled at COLOR_ATTACHMENT_OUTPUT does not wait for that transition, and
    // synchronization validation reports PRESENT_AFTER_WRITE. Presentation is not a pipeline
    // stage, so no narrower stage covers it.
    signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

    VkCommandBufferSubmitInfo commands{};
    commands.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commands.commandBuffer = acquisition.commands;

    VkSubmitInfo2 submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit.waitSemaphoreInfoCount = 1;
    submit.pWaitSemaphoreInfos = &wait;
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &commands;
    submit.signalSemaphoreInfoCount = 1;
    submit.pSignalSemaphoreInfos = &signal;

    // signals the Ring's fence for this frame. The Ring waits on it before reusing the slot
    result = vkQueueSubmit2(device_->graphicsQueue(), 1, &submit, ring_->submitting());
    device::check(result, "Unable to submit a vulkan frame");

    VkSwapchainKHR chain = swapchain_->handle();
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinished_[acquisition.image];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &chain;
    presentInfo.pImageIndices = &acquisition.image;

    result = vkQueuePresentKHR(device_->presentQueue(), &presentInfo);

    ring_->advance();

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || suboptimal_) {
        // the submit has already happened. The caller recreates the swapchain before the
        // next frame
        suboptimal_ = false;
        return Status::OutOfDate;
    }
    device::check(result, "Unable to present a vulkan swapchain image");

    return Status::Ready;
}

};  // namespace v3d::render::realtime::vulkan::frame
