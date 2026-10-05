/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>

namespace v3d::moya {

class RenderContext;

/**
 * How moya decides what the camera sees, which a scene picks with `Hider` - ADR-0078. Both
 * read one graphics state and write one framebuffer; what differs is whether primitives are
 * diced and bucketed or traced.
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
    virtual unsigned int samplesTaken(unsigned int column, unsigned int row) const {
        (void)column;
        (void)row;
        return 0;
    }
};

/**
 * `Hider "hidden"`, RI's default: every primitive diced into micropolygon grids, shaded, and
 * hidden into the samples of the buckets it reaches.
 **/
class ReyesHider final : public Hider {
 public:
    bool traces() const override;
    void render(RenderContext* context, v3d::render::offline::FrameBuffer* planes) override;
};

};  // namespace v3d::moya
