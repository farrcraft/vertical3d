/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>

#include <vulkan/vulkan.h>

#include <string_view>

#include <boost/shared_ptr.hpp>

namespace v3d::image {
class Image;
};  // namespace v3d::image

namespace v3d::render::realtime::vulkan::frame {

class Swapchain;

/**
 * A presented frame read back off the swapchain and written out as a png.
 *
 * Copying and writing are two calls because a queue submit sits between them: record() adds
 * the copy to the command buffer the frame is already being drawn into, and write() reads
 * the result once that submit has completed. What orders the two is the caller's - a fence
 * it already waits on, or a device wait.
 *
 * The readback allocation is made by the first record() and reused by every later one, so a
 * renderer that holds a Capture and never asks for one pays nothing for it.
 **/
class Capture final {
 public:
    /**
     * @param device the device to allocate the readback buffer on
     * @param logger where a written file is reported
     **/
    Capture(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<log::Logger>& logger);

    ~Capture() = default;

    Capture(const Capture&) = delete;
    Capture& operator=(const Capture&) = delete;

    /**
     * Copy an acquired swapchain image into the readback buffer.
     *
     * The image is handed back in the layout it arrived in, so a frame that is captured
     * presents exactly as one that is not.
     *
     * @param commands the buffer the frame was recorded into, still recording
     * @param image which of the chain's images was acquired
     * @pre the image is in PRESENT_SRC, which is where Recorder::record leaves it
     **/
    void record(VkCommandBuffer commands, const Swapchain& swapchain, uint32_t image);

    /**
     * Write what the last record() copied as a png.
     *
     * @pre the submit carrying that record() has completed
     * @return whether a file was written, which is false if nothing has been recorded or
     *         the writer could not open the path
     **/
    bool write(std::string_view filename);

    /**
     * Turn a copied swapchain image into one the writers understand.
     *
     * A chain is commonly BGRA and a png is RGBA, and the alpha a chain presents is not
     * meaningful once the image has been composited, so it is written opaque.
     *
     * Device free, so a test can pin the channel order without a chain to acquire from.
     *
     * @param pixels one tightly packed 4 byte texel per pixel, as the copy left them
     **/
    static boost::shared_ptr<image::Image> convert(const unsigned char* pixels, uint32_t width, uint32_t height,
        VkFormat format);

 private:
    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<log::Logger> logger_;
    boost::shared_ptr<memory::Buffer> readback_;
    uint32_t width_;
    uint32_t height_;
    VkFormat format_;
};

};  // namespace v3d::render::realtime::vulkan::frame
