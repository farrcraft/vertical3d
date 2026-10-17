/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>

#include <glm/vec2.hpp>

namespace v3d::moya {

class RenderContext;

/**
 * How moya decides what the camera sees. A scene picks the hider with the `Hider` request.
 * Both hiders read one graphics state and write one framebuffer. They differ in whether
 * primitives are diced and bucketed or traced.
 **/
class Hider {
 public:
    virtual ~Hider() = default;

    /**
     * Whether the hider sees the traced scene and nothing else. A primitive is then traced
     * rather than diced and bucketed, and a sphere, which only the tracer intersects, can be
     * drawn.
     **/
    virtual bool traces() const = 0;

    /**
     * Draw the frame the context has gathered into its planes: colour from RED, then
     * COVERAGE and DEPTH.
     **/
    virtual void render(RenderContext* context, v3d::render::offline::FrameBuffer* planes) = 0;

    /**
     * How many samples a pixel took in the last render, or zero for a hider that takes what
     * PixelSamples names.
     **/
    virtual unsigned int samplesTaken(const glm::uvec2 & pixel) const {
        (void)pixel;
        return 0;
    }
};

};  // namespace v3d::moya
