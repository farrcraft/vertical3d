/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Hider.h"

#include <moya/libmoya/RenderContext.h>

namespace v3d::moya::reyes {

bool Hider::traces() const {
    return false;
}

void Hider::render(RenderContext* context, v3d::render::offline::FrameBuffer* planes) {
    context->bucket(planes);
}

};  // namespace v3d::moya::reyes
