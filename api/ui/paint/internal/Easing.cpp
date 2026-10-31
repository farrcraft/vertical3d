/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Easing.h"

#include <api/ui/Transition.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/style/Resolver.h>

#include <glm/vec4.hpp>

namespace v3d::ui::paint::internal {

Easing::Easing(const style::Resolver& styles) :
    styles_(styles),
    timed_(false) {
}

void Easing::clock(const Clock& now) noexcept {
    clock_ = now;
    timed_ = true;
}

const Clock& Easing::clock() const noexcept {
    return clock_;
}

bool Easing::timed() const noexcept {
    return timed_;
}

glm::vec4 Easing::toward(Transition* transition, const glm::vec4& target) const {
    return transition->toward(target, clock_, timed_ ? styles_.base().hoverDuration : 0.0F);
}

glm::vec4 Easing::away(Transition* transition) const {
    return transition->away(clock_, timed_ ? styles_.base().hoverDuration : 0.0F);
}

float Easing::opened(Opening* opening) const {
    const float rest = 1.0F - opening->progress(clock_, timed_ ? styles_.base().openDuration : 0.0F);
    return 1.0F - (rest * rest);
}

};  // namespace v3d::ui::paint::internal
