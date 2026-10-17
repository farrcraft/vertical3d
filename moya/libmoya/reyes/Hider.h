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
 * The hider `Hider "hidden"` selects, and RI's default. Every primitive is bucketed, diced
 * into micropolygon grids and shaded, and each grid is hidden into the frame's samples.
 **/
class Hider final : public moya::Hider {
 public:
    bool traces() const override;
    void render(RenderContext* context, v3d::render::offline::FrameBuffer* planes) override;
};

};  // namespace v3d::moya::reyes
