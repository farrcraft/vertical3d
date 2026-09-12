/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Presenter.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include <cstddef>
#include <sstream>
#include <stdexcept>
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
    createSync();
}

/**
 **/
Presenter::~Presenter() {
    // nothing may be waiting on a semaphore when it is destroyed. The ring's fences are the
    // ring's to wait on, and it outlives this because this holds it
    ring_->waitIdle();

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

    // one per frame rather than per image: a frame waits on it before drawing, which is the
    // ring's turn rather than the chain's
    for (uint32_t index = 0; index < ring_->framesInFlight(); index++) {
        VkSemaphore semaphore = VK_NULL_HANDLE;
        VkResult result = vkCreateSemaphore(device_->handle(), &semaphoreInfo, nullptr, &semaphore);
        if (result != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to create a vulkan semaphore - " << device::resultString(result);
            throw std::runtime_error(msg.str());
        }
        imageAvailable_.push_back(semaphore);
    }

    reset();
}

/**
 **/
void Presenter::destroyImageSync() {
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
        VkResult result = vkCreateSemaphore(device_->handle(), &semaphoreInfo, nullptr, &semaphore);
        if (result != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to create a vulkan semaphore - " << device::resultString(result);
            throw std::runtime_error(msg.str());
        }
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
    // a window with no area has no chain to draw into
    if (!swapchain_->valid() || renderFinished_.empty()) {
        return Status::Skip;
    }

    // the wait belongs here rather than being left to begin(): the semaphore the acquire
    // signals is one per frame, and this slot's may still be pending from its last turn until
    // that submission completes. begin() waits again, which costs nothing on a fence that is
    // already signalled, and it is begin() that unsignals - so a chain found out of date below
    // leaves the ring exactly as it was found
    ring_->waitFrame();

    uint32_t image = 0;
    VkResult result = vkAcquireNextImageKHR(device_->handle(), swapchain_->handle(), UINT64_MAX, imageAvailable_[ring_->frame()],
        VK_NULL_HANDLE, &image);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // the semaphore was not signalled, so nothing is left waiting by giving up here
        return Status::OutOfDate;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::stringstream msg;
        msg << "Unable to acquire a vulkan swapchain image - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }
    // a suboptimal image can still be drawn and presented - rebuild the chain afterwards
    suboptimal_ = result == VK_SUBOPTIMAL_KHR;

    acquisition->image = image;
    acquisition->commands = ring_->begin();
    return Status::Ready;
}

/**
 **/
Presenter::Status Presenter::present(const Acquisition& acquisition) {
    VkResult result = vkEndCommandBuffer(acquisition.commands);
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to end a vulkan command buffer - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    VkSemaphoreSubmitInfo wait{};
    wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    wait.semaphore = imageAvailable_[ring_->frame()];
    // nothing before the colour attachment write needs the image, so only that stage waits
    wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSemaphoreSubmitInfo signal{};
    signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signal.semaphore = renderFinished_[acquisition.image];
    // every stage, not the colour attachment one: the last thing the command buffer does to
    // the image is the layout transition into PRESENT_SRC, whose destination scope is the
    // bottom of the pipe. A semaphore signalled at COLOR_ATTACHMENT_OUTPUT does not wait for
    // that transition, so the presentation engine reads an image still being moved - which
    // synchronization validation reports as PRESENT_AFTER_WRITE. Presentation is not a
    // pipeline stage, so there is no narrower stage that is the right one here.
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

    // the ring's fence, which is what its next turn around waits on - ADR-0051
    result = vkQueueSubmit2(device_->graphicsQueue(), 1, &submit, ring_->fence());
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to submit a vulkan frame - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

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
        // the submission stands either way - the chain is rebuilt before the next frame
        suboptimal_ = false;
        return Status::OutOfDate;
    }
    if (result != VK_SUCCESS) {
        std::stringstream msg;
        msg << "Unable to present a vulkan swapchain image - " << device::resultString(result);
        throw std::runtime_error(msg.str());
    }

    return Status::Ready;
}

};  // namespace v3d::render::realtime::vulkan::frame
