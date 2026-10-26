/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/pipeline/Formats.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {

class DeviceContext;

class RenderTarget;

namespace vulkan::pipeline {
class Sampler;
};  // namespace vulkan::pipeline

namespace renderer {
class FullScreen;
};  // namespace renderer

/**
 * A colour grade: a full-screen pass that looks every pixel of a scene up in a 16 cubed table.
 * It gives a whole frame its look in one step.
 *
 * The table comes from a strip, 256 by 16: sixteen slices of blue side by side, red across
 * each and green down it. With no strip, or one of the wrong size, the table is the identity
 * and the pass copies the scene. The table maps a colour as it is shown to the colour it is
 * shown as, so a strip graded in an image editor grades a frame as it looked there. A scene in
 * an sRGB target is encoded before the lookup, and a result drawn into one is decoded before it
 * is stored, so neither encoding changes what the table means.
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
     * @param formats the colour format of what the grade draws into, and the depth format of
     *        the pass it is drawn in, or undefined for none
     * @param strip the table, or empty for the identity
     * @throw std::runtime_error if the pipeline or the table cannot be made
     **/
    Grade(const boost::shared_ptr<log::Logger>& logger, const boost::shared_ptr<DeviceContext>& context,
        const vulkan::pipeline::Formats& formats, const boost::shared_ptr<image::Image>& strip = boost::shared_ptr<image::Image>());

    /**
     * Release the table, and every source not yet released, through the Ring, so a frame in
     * flight finishes with them.
     **/
    ~Grade();

    Grade(const Grade&) = delete;
    Grade& operator=(const Grade&) = delete;

    /**
     * Bind one slot of a scene target as what is graded, with the table.
     *
     * After the target is resized, call source() again. A source not released by the time the
     * Grade is destroyed is released by its destructor. If binding throws, the scene's
     * registration is released before the exception leaves.
     *
     * @return the source submit() names
     * @throw std::runtime_error if the scene has no colour image, or the source cannot be bound
     **/
    MaterialHandle source(const RenderTarget& scene, uint32_t slot = 0);

    /**
     * Release a source, and the scene's registration with it.
     *
     * @return whether there was anything to release
     **/
    bool release(const MaterialHandle& source);

    /**
     * Grade with a different table from the next frame built: a zone's own look, or a step of
     * a slow change between two tables a game lerps itself.
     *
     * A new table is made and the old one retired through the Ring, so a frame in flight
     * finishes with the table it was recorded against. The new table's upload does not wait
     * for the GPU. Every source keeps the handle source() gave it.
     * Call this before the frame's submit(): a submit() earlier in the same frame still draws
     * with the old table.
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
     * The table a strip describes, as RGBA texels with red fastest, then green, then blue,
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
     * table. The material changes when the table is replaced. The handle the caller holds is
     * the first material, which is this entry's key.
     **/
    struct Source final {
        TextureHandle scene;
        MaterialHandle current;
        bool decoded;   /**< the scene is an _SRGB target, read as linear **/
    };

    /**
     * @return the pass a source's material belongs to
     **/
    renderer::FullScreen& pass(const Source& source) const;

    /**
     * Register a table's texels as a texture, uploaded into an image of its own.
     **/
    TextureHandle createTable(const std::vector<uint8_t>& texels);

    boost::shared_ptr<log::Logger> logger_;
    boost::shared_ptr<DeviceContext> context_;
    boost::shared_ptr<renderer::FullScreen> shown_;     /**< for a scene that holds display values **/
    boost::shared_ptr<renderer::FullScreen> decoded_;   /**< for an _SRGB scene, read as linear **/
    boost::shared_ptr<vulkan::pipeline::Sampler> nearest_;
    TextureHandle tableTexture_;
    std::map<MaterialHandle, Source> sources_;
};

};  // namespace v3d::render::realtime
