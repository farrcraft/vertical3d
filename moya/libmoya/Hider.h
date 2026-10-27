/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>

#include <cstddef>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::moya {

class Curve;
class Polygon;
class RenderContext;

/**
 * How moya decides what the camera sees. A scene picks the hider with the `Hider` request.
 * Every hider reads the same graphics state and writes the same framebuffer. Hiders differ in
 * whether primitives are diced and bucketed, or traced.
 **/
class Hider {
 public:
    virtual ~Hider() = default;

    /**
     * Whether the hider sees the traced scene and nothing else. A primitive is then traced,
     * not diced and bucketed. Only such a hider draws a sphere, because only the tracer
     * intersects one.
     **/
    virtual bool traces() const = 0;

    /** The world begins, and the camera is fixed. **/
    virtual void begin(RenderContext* context) {
        (void)context;
    }
    /**
     * A placed polygon for a hider that buckets what it draws. A hider that traces sees the
     * traced scene instead, and is given nothing.
     **/
    virtual void add(const boost::shared_ptr<Polygon> & poly, RenderContext* context) {
        (void)poly;
        (void)context;
    }
    /**
     * A placed curve segment for a hider that buckets what it draws, as a polygon is given.
     **/
    virtual void add(const boost::shared_ptr<Curve> & curve, RenderContext* context) {
        (void)curve;
        (void)context;
    }
    /** How many primitives the hider has bucketed and not yet rendered. **/
    virtual std::size_t bucketed() const {
        return 0;
    }

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
