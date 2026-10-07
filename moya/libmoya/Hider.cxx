/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Hider.h"

#include "RenderContext.h"

namespace v3d::moya {

bool ReyesHider::traces() const {
    return false;
}

void ReyesHider::render(RenderContext* context, v3d::render::offline::FrameBuffer* planes) {
    context->bucket(planes);
}

};  // namespace v3d::moya
