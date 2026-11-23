/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Opening.h"

#include "Clock.h"

#include <algorithm>

namespace v3d::ui {

float Opening::progress(const Clock& clock, float duration) {
    // drawn twice in one frame is still open, and a frame skipped means it was closed between
    const bool continuing = seen_ && clock.frame >= frame_ && clock.frame <= frame_ + 1;
    if (!continuing) {
        start_ = clock.now;
    }
    seen_ = true;
    frame_ = clock.frame;
    if (duration <= 0.0F) {
        return 1.0F;
    }
    return std::clamp((clock.now - start_) / duration, 0.0F, 1.0F);
}

};  // namespace v3d::ui
