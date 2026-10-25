/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Curve.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace v3d::type::animation {

/**
 **/
Curve::Curve() {
}

/**
 **/
Curve::Curve(const std::vector<Key>& keys) :
    keys_(keys) {
    for (std::size_t index = 0; index < keys_.size(); index++) {
        const Key& key = keys_[index];
        if (!std::isfinite(key.frame) || !std::isfinite(key.value) || !std::isfinite(key.in) || !std::isfinite(key.out)) {
            throw std::invalid_argument("a curve's keys must be finite");
        }
        if (index > 0 && !(keys_[index - 1].frame + SAME_FRAME < key.frame)) {
            throw std::invalid_argument("a curve's keys must rise in frame");
        }
    }
    smooth();
}

/**
 **/
float Curve::sample(float frame) const {
    if (keys_.empty()) {
        return 0.0F;
    }
    const Key& first = keys_.front();
    const Key& last = keys_.back();
    if (!std::isfinite(frame) || frame <= first.frame) {
        return first.value;
    }
    if (frame >= last.frame) {
        return last.value;
    }
    const auto after = std::upper_bound(keys_.begin(), keys_.end(), frame,
        [](float at, const Key& key) { return at < key.frame; });
    const Key& to = *after;
    const Key& from = *(after - 1);
    const float span = to.frame - from.frame;
    const float along = (frame - from.frame) / span;
    switch (from.interpolation) {
        case Interpolation::Step:
            return from.value;
        case Interpolation::Linear:
            return from.value + ((to.value - from.value) * along);
        case Interpolation::Bezier:
            break;
    }
    // a cubic Hermite segment, whose slopes are scaled from per frame to per segment
    const float squared = along * along;
    const float cubed = squared * along;
    const float start = (2.0F * cubed) - (3.0F * squared) + 1.0F;
    const float leave = cubed - (2.0F * squared) + along;
    const float end = (-2.0F * cubed) + (3.0F * squared);
    const float arrive = cubed - squared;
    return (start * from.value) + (leave * span * from.out) + (end * to.value) + (arrive * span * to.in);
}

/**
 **/
void Curve::set(float frame, float value) {
    const auto at = std::lower_bound(keys_.begin(), keys_.end(), frame - SAME_FRAME,
        [](const Key& key, float least) { return key.frame < least; });
    if (at != keys_.end() && std::fabs(at->frame - frame) <= SAME_FRAME) {
        at->value = value;
    } else {
        keys_.insert(at, Key{ .frame = frame, .value = value });
    }
    smooth();
}

/**
 **/
bool Curve::remove(float frame) {
    const auto at = std::find_if(keys_.begin(), keys_.end(),
        [frame](const Key& key) { return std::fabs(key.frame - frame) <= SAME_FRAME; });
    if (at == keys_.end()) {
        return false;
    }
    keys_.erase(at);
    smooth();
    return true;
}

/**
 **/
bool Curve::empty() const noexcept {
    return keys_.empty();
}

/**
 **/
const std::vector<Curve::Key>& Curve::keys() const noexcept {
    return keys_;
}

/**
 **/
void Curve::smooth() {
    for (std::size_t index = 0; index < keys_.size(); index++) {
        Key& key = keys_[index];
        if (!key.automatic) {
            continue;
        }
        float slope = 0.0F;
        if (index > 0 && index + 1 < keys_.size()) {
            const Key& before = keys_[index - 1];
            const Key& after = keys_[index + 1];
            // a peak or a trough stays flat, so the curve does not overshoot it
            const bool turns = (key.value - before.value) * (after.value - key.value) <= 0.0F;
            slope = turns ? 0.0F : (after.value - before.value) / (after.frame - before.frame);
        }
        key.in = slope;
        key.out = slope;
    }
}

};  // namespace v3d::type::animation
