/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>
#include <moya/libmoya/Hider.h>

namespace v3d::moya {
class RenderContext;
};  // namespace v3d::moya

namespace v3d::moya::reyes {

/**
 * `Hider "hidden"`, RI's default: every primitive diced into micropolygon grids, shaded, and
 * hidden into the samples of the buckets it reaches.
 **/
class Hider final : public moya::Hider {
 public:
    bool traces() const override;
    void render(RenderContext* context, v3d::render::offline::FrameBuffer* planes) override;
};

};  // namespace v3d::moya::reyes
