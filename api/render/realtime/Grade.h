/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {

class DeviceContext;

namespace vulkan::frame {
class RenderTarget;
};  // namespace vulkan::frame

namespace vulkan::memory {
class Image;
};  // namespace vulkan::memory

namespace vulkan::pipeline {
class Sampler;
};  // namespace vulkan::pipeline

namespace vulkan::renderer {
class FullScreen;
};  // namespace vulkan::renderer

/**
 * A colour grade: a full-screen pass that looks every pixel of a scene up in a 16 cubed table,
 * which is how a game gives a whole frame its look in one step.
 *
 * The table comes from a strip, 256 by 16: sixteen slices of blue side by side, red across
 * each and green down it. With no strip, or one of the wrong size, the table is the identity
 * and the pass copies the scene. The table is indexed by linear colour (ADR-0066), so a lit
 * scene drawn into an sRGB target is graded as the light it holds, and a strip is baked for
 * that.
 *
 * The scene is read nearest, a texel per pixel, so the pass draws into a target the scene's
 * size. Its pass names the scene in Pass::reads() - ADR-0068.
 **/
class Grade final {
 public:
    /**
     * The table's edge, in entries.
     **/
    static const uint32_t SIZE = 16;

    /**
     * @param colour the format of what the grade draws into
     * @param depth the depth format of the pass it is drawn in, or undefined for none
     * @param strip the table, or empty for the identity
     * @throw std::runtime_error if the pipeline or the table cannot be made
     **/
    Grade(const boost::shared_ptr<log::Logger>& logger, const boost::shared_ptr<DeviceContext>& context,
        VkFormat colour, VkFormat depth, const boost::shared_ptr<image::Image>& strip = boost::shared_ptr<image::Image>());

    ~Grade();

    Grade(const Grade&) = delete;
    Grade& operator=(const Grade&) = delete;

    /**
     * Bind one slot of a scene target as what is graded, with the table.
     *
     * The same contract as FullScreen::source: a target that is resized is bound again, and a
     * source is released before the grade goes.
     *
     * @return the source submit() names
     **/
    MaterialHandle source(const vulkan::frame::RenderTarget& scene, uint32_t slot = 0);

    /**
     * Let a source go, and the scene's registration with it.
     *
     * @return whether there was anything to release
     **/
    bool release(const MaterialHandle& source);

    /**
     * Submit the grade of a source into a pass.
     **/
    void submit(const MaterialHandle& source, Pass* pass) const;

    /**
     * The table a strip describes, as RGBA texels with red fastest, then green, then blue -
     * the order a 3D image is uploaded in.
     *
     * @return the texels, or nothing for a strip that is not SIZE * SIZE by SIZE, in RGB or RGBA
     **/
    static std::vector<uint8_t> table(const boost::shared_ptr<image::Image>& strip);

    /**
     * @return the table that leaves every colour as it is, in the same order
     **/
    static std::vector<uint8_t> identity();

 private:
    boost::shared_ptr<DeviceContext> context_;
    boost::shared_ptr<vulkan::renderer::FullScreen> pass_;
    boost::shared_ptr<vulkan::memory::Image> table_;
    boost::shared_ptr<vulkan::pipeline::Sampler> nearest_;
    TextureHandle tableTexture_;
    std::map<MaterialHandle, TextureHandle> scenes_;
};

};  // namespace v3d::render::realtime
