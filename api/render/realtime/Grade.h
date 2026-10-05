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

namespace vulkan::pipeline {
class Sampler;
};  // namespace vulkan::pipeline

namespace vulkan::renderer {
class FullScreen;
};  // namespace vulkan::renderer

/**
 * A colour grade: a full-screen pass that looks every pixel of a scene up in a 16 cubed table.
 * It gives a whole frame its look in one step.
 *
 * The table comes from a strip, 256 by 16: sixteen slices of blue side by side, red across
 * each and green down it. With no strip, or one of the wrong size, the table is the identity
 * and the pass copies the scene. The table is indexed by linear colour, so a lit scene drawn
 * into an sRGB target is graded as the light it holds, and a strip must be baked for that.
 *
 * The scene is read nearest, a texel per pixel, so the pass draws into a target the scene's
 * size. The pass the grade is drawn in must name the scene target in Pass::reads().
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
     * Grade with a different table from the next frame built: a zone's own look, or a step of
     * a slow change between two tables a game lerps itself.
     *
     * The table is made anew and the old one released through the ring, so a frame in flight
     * finishes with the table it was recorded against. Every source keeps the handle source()
     * gave it. Call this before the frame's submit(), because it releases the material a submit
     * earlier in the same frame would have used.
     *
     * @param texels SIZE cubed RGBA texels in the order table() gives them
     * @return false for texels of the wrong count, which leave the table as it was
     **/
    bool replace(const std::vector<uint8_t>& texels);

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
    /**
     * A scene being graded: its registration, and the material that pairs it with the current
     * table. The material changes when the table is replaced; the handle the caller holds is
     * the first one, which is this entry's key.
     **/
    struct Source final {
        TextureHandle scene;
        MaterialHandle current;
    };

    /**
     * Register a table's texels as a texture, uploaded into an image of its own.
     **/
    TextureHandle createTable(const std::vector<uint8_t>& texels);

    boost::shared_ptr<DeviceContext> context_;
    boost::shared_ptr<vulkan::renderer::FullScreen> pass_;
    boost::shared_ptr<vulkan::pipeline::Sampler> nearest_;
    TextureHandle tableTexture_;
    std::map<MaterialHandle, Source> sources_;
};

};  // namespace v3d::render::realtime
