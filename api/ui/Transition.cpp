/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Transition.h"

#include <algorithm>

#include <glm/common.hpp>

namespace v3d::ui {

glm::vec4 Transition::toward(const glm::vec4& target, const Clock& clock, float duration) {
    const float now = clock.now;
    if (!started_ || duration <= 0.0F) {
        from_ = target;
        to_ = target;
        start_ = now;
        duration_ = 0.0F;
        started_ = true;
        return target;
    }
    if (target != to_) {
        from_ = shown(now);
        // nothing was showing, so whatever colour it held does not matter. Fading in from it
        // would darken the new colour on the way, so it fades in its own
        if (from_.a <= 0.0F) {
            from_ = glm::vec4(target.r, target.g, target.b, 0.0F);
        }
        to_ = target;
        start_ = now;
        duration_ = duration;
    }
    return shown(now);
}

glm::vec4 Transition::away(const Clock& clock, float duration) {
    return toward(glm::vec4(to_.r, to_.g, to_.b, 0.0F), clock, duration);
}

glm::vec4 Transition::shown(float now) const noexcept {
    if (duration_ <= 0.0F) {
        return to_;
    }
    const float share = std::clamp((now - start_) / duration_, 0.0F, 1.0F);
    return glm::mix(from_, to_, share);
}

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
