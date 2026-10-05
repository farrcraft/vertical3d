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
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::image {
class Image;
};  // namespace v3d::image

namespace v3d::render::realtime::vulkan::frame {

class Swapchain;

/**
 * A drawn image read back off the device and written out as a png - a presented frame off the
 * swapchain, or an offscreen target a pass drew into.
 *
 * Copying and writing are two calls because a queue submit sits between them: record() adds
 * the copy to the command buffer the frame is already being drawn into, and write() reads
 * the result once that frame's fence has signalled. The caller is responsible for that
 * synchronisation - a fence it already waits on, or a device wait. A record() with no
 * write() after it is not detected: the copy runs and is discarded.
 *
 * The readback allocation is made by the first record() and reused by every later one, so a
 * renderer that holds a Capture and never asks for one pays nothing for it.
 **/
class Capture final {
 public:
    /**
     * What a capture reads from. A swapchain image and a render target differ only in these
     * four things, so record() takes them rather than either class.
     **/
    struct Source {
        Source() noexcept;

        VkImage image;         /**< the image to copy out of **/
        VkExtent2D extent;     /**< its size **/
        VkFormat format;       /**< its colour format, which decides the channel order **/
        VkImageLayout layout;  /**< what it is in when record() is called, and what it is left in **/
        /**
         * Whether to copy the depth aspect rather than colour. Only a D32_SFLOAT image can be
         * read this way, because its copy is one plain float per texel, which a test compares.
         * It is also the depth format the renderer prefers.
         **/
        bool depth;
    };

    /**
     * @param device the device to allocate the readback buffer on
     * @param logger where a written file is reported
     **/
    Capture(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<log::Logger>& logger);

    ~Capture() = default;

    Capture(const Capture&) = delete;
    Capture& operator=(const Capture&) = delete;

    /**
     * Copy an image into the readback buffer.
     *
     * The image is handed back in the layout it arrived in, so an image that is captured is
     * used afterwards exactly as one that is not.
     *
     * @param commands a command buffer that is still recording
     * @pre the image is in source.layout, and what wrote it is an attachment write
     * @throw std::runtime_error for a depth source in a format other than D32_SFLOAT
     **/
    void record(VkCommandBuffer commands, const Source& source);

    /**
     * Copy an acquired swapchain image into the readback buffer.
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
     * What the last record() of a depth source copied, one float per pixel in row order.
     * Compared in a test rather than written as a png, because a picture of depth would round
     * it to the eight bits a png channel holds.
     *
     * @pre the submit carrying that record() has completed
     * @return the depths, or nothing when the last record() was not of a depth source
     **/
    std::vector<float> depth() const;

    /**
     * Turn a copied image into one the writers understand.
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
    bool depth_;
};

};  // namespace v3d::render::realtime::vulkan::frame
