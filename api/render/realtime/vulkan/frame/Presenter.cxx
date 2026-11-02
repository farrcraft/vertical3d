/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Presenter.h"

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/device/Result.h>

#include "Ring.h"
#include "Swapchain.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 **/
Presenter::Acquisition::Acquisition() noexcept :
image(0),
commands(VK_NULL_HANDLE) {
}

namespace {

/**
 * @return a new semaphore
 * @throw std::runtime_error if it cannot be made
 **/
VkSemaphore semaphore(VkDevice device) {
    VkSemaphoreCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkSemaphore made = VK_NULL_HANDLE;
    const VkResult result = vkCreateSemaphore(device, &info, nullptr, &made);
    device::check(result, "Unable to create a vulkan semaphore");
    return made;
}

};  // namespace

/**
 **/
Presenter::Joined::Joined(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<Swapchain>& swapchain, uint32_t framesInFlight) :
device_(device),
swapchain_(swapchain),
stale_(false) {
    try {
        for (uint32_t index = 0; index < framesInFlight; index++) {
            imageAvailable_.push_back(semaphore(device_->handle()));
        }
        reset();
    } catch (...) {
        destroySync();
        throw;
    }
}

/**
 **/
Presenter::Joined::~Joined() {
    destroySync();
}

/**
 **/
boost::shared_ptr<Swapchain> Presenter::Joined::swapchain() const noexcept {
    return swapchain_;
}

/**
 **/
bool Presenter::Joined::stale() const noexcept {
    return stale_;
}

/**
 **/
void Presenter::Joined::reset() {
    destroyImageSync();
    for (std::size_t index = 0; index < swapchain_->length(); index++) {
        renderFinished_.push_back(semaphore(device_->handle()));
    }
    stale_ = false;
}

/**
 **/
void Presenter::Joined::destroyImageSync() noexcept {
    for (VkSemaphore each : renderFinished_) {
        vkDestroySemaphore(device_->handle(), each, nullptr);
    }
    renderFinished_.clear();
}

/**
 **/
void Presenter::Joined::destroySync() noexcept {
    destroyImageSync();
    for (VkSemaphore each : imageAvailable_) {
        vkDestroySemaphore(device_->handle(), each, nullptr);
    }
    imageAvailable_.clear();
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
boost::shared_ptr<Presenter::Joined> Presenter::join(const boost::shared_ptr<Swapchain>& swapchain) {
    const boost::shared_ptr<Joined> joined = boost::make_shared<Joined>(device_, swapchain, ring_->framesInFlight());
    joined_.push_back(joined);
    return joined;
}

/**
 **/
void Presenter::leave(const boost::shared_ptr<Joined>& joined) {
    ring_->waitIdle();
    std::erase(joined_, joined);
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
    acquisition->joined.clear();
    for (const boost::shared_ptr<Joined>& joined : joined_) {
        // a joined swapchain with no image to give is left out of this frame, and its window
        // waits for the next
        if (joined->stale_ || !joined->swapchain_->valid() || joined->renderFinished_.empty()) {
            continue;
        }
        uint32_t taken = 0;
        const VkResult got = vkAcquireNextImageKHR(device_->handle(), joined->swapchain_->handle(), UINT64_MAX,
            joined->imageAvailable_[ring_->frame()], VK_NULL_HANDLE, &taken);
        if (got == VK_ERROR_OUT_OF_DATE_KHR) {
            joined->stale_ = true;
            continue;
        }
        device::check(got, "Unable to acquire a vulkan swapchain image", VK_SUBOPTIMAL_KHR);
        // a suboptimal image is drawn and presented, and the swapchain recreated after
        joined->stale_ = got == VK_SUBOPTIMAL_KHR;
        acquisition->joined.push_back({ .joined = joined, .image = taken });
    }
    acquisition->commands = ring_->begin();
    return Status::Ready;
}

/**
 **/
Presenter::Status Presenter::present(const Acquisition& acquisition) {
    VkResult result = vkEndCommandBuffer(acquisition.commands);
    device::check(result, "Unable to end a vulkan command buffer");

    std::vector<VkSemaphoreSubmitInfo> waits;
    std::vector<VkSemaphoreSubmitInfo> signals;
    std::vector<VkSemaphore> finished;
    std::vector<VkSwapchainKHR> chains;
    std::vector<uint32_t> images;
    // one image the frame drew into: what the submit waits on and signals, and what is presented
    struct Shown {
        VkSemaphore available;
        VkSemaphore done;
        VkSwapchainKHR chain;
        uint32_t image;
    };
    const auto take = [&](const Shown& shown) {
        VkSemaphoreSubmitInfo wait{};
        wait.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        wait.semaphore = shown.available;
        // nothing before the colour attachment write needs the image, so only that stage waits
        wait.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        waits.push_back(wait);

        VkSemaphoreSubmitInfo signal{};
        signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signal.semaphore = shown.done;
        // ALL_COMMANDS, not COLOR_ATTACHMENT_OUTPUT. The command buffer ends with the layout
        // transition into PRESENT_SRC, whose destination scope is the bottom of the pipe. A
        // semaphore signalled at COLOR_ATTACHMENT_OUTPUT does not wait for that transition, and
        // synchronization validation reports PRESENT_AFTER_WRITE. Presentation is not a pipeline
        // stage, so no narrower stage covers it.
        signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        signals.push_back(signal);

        finished.push_back(shown.done);
        chains.push_back(shown.chain);
        images.push_back(shown.image);
    };
    take({ .available = imageAvailable_[ring_->frame()], .done = renderFinished_[acquisition.image], .chain = swapchain_->handle(),
        .image = acquisition.image });
    for (const Taken& taken : acquisition.joined) {
        const Joined& joined = *taken.joined;
        take({ .available = joined.imageAvailable_[ring_->frame()], .done = joined.renderFinished_[taken.image],
            .chain = joined.swapchain_->handle(), .image = taken.image });
    }

    VkCommandBufferSubmitInfo commands{};
    commands.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    commands.commandBuffer = acquisition.commands;

    VkSubmitInfo2 submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submit.waitSemaphoreInfoCount = static_cast<uint32_t>(waits.size());
    submit.pWaitSemaphoreInfos = waits.data();
    submit.commandBufferInfoCount = 1;
    submit.pCommandBufferInfos = &commands;
    submit.signalSemaphoreInfoCount = static_cast<uint32_t>(signals.size());
    submit.pSignalSemaphoreInfos = signals.data();

    // signals the Ring's fence for this frame. The Ring waits on it before reusing the slot
    result = vkQueueSubmit2(device_->graphicsQueue(), 1, &submit, ring_->submitting());
    device::check(result, "Unable to submit a vulkan frame");

    // each swapchain's own result, so one window out of date does not hide another's
    std::vector<VkResult> results(chains.size(), VK_SUCCESS);
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(finished.size());
    presentInfo.pWaitSemaphores = finished.data();
    presentInfo.swapchainCount = static_cast<uint32_t>(chains.size());
    presentInfo.pSwapchains = chains.data();
    presentInfo.pImageIndices = images.data();
    presentInfo.pResults = results.data();

    result = vkQueuePresentKHR(device_->presentQueue(), &presentInfo);
    for (std::size_t index = 0; index < acquisition.joined.size(); index++) {
        const VkResult own = results[index + 1];
        if (own == VK_ERROR_OUT_OF_DATE_KHR || own == VK_SUBOPTIMAL_KHR) {
            acquisition.joined[index].joined->stale_ = true;
        }
    }
    // the call's result covers every swapchain. Where it is only one of them out of date, the
    // main swapchain's own result decides what this returns
    if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
        result = results[0];
    }

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
