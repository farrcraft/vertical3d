/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Placement.h"

#include <api/render/offline/MovingTransform.h>
#include <moya/libmoya/state/RenderContext.h>

#include <algorithm>
#include <array>
#include <limits>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Footprint.h"
#include "Samples.h"

namespace v3d::moya::reyes {

Placement::Placement(const glm::mat4x4 & toRaster, const Lens & lens,
    const v3d::render::offline::MovingTransform & motion, const glm::vec2 & shutter) :
    toRaster_(toRaster), radius_(lens.radius), focus_(lens.focus), motion_(motion), shutter_(shutter),
    fromReference_(glm::inverse(motion.reference())) {
}

void Placement::place(const Corners & eye) {
    eye_ = eye;
    if (motion_.moving()) {
        sweep();
        return;
    }
    for (unsigned int k = 0; k < 4; k++) {
        still_[k] = state::project(toRaster_, eye[k]);
        if (radius_ <= 0.0F) {
            continue;
        }
        across_[k] = state::project(toRaster_, lensed(eye[k], glm::vec2(1.0F, 0.0F))) - still_[k];
        down_[k] = state::project(toRaster_, lensed(eye[k], glm::vec2(0.0F, 1.0F))) - still_[k];
    }
    stillBound();
}

Placement::Corners Placement::at(const Samples::Sample & sample) const {
    if (motion_.moving()) {
        return moved(delta(sample.time), sample.lens);
    }
    if (radius_ <= 0.0F) {
        return still_;
    }
    Corners corners;
    for (unsigned int k = 0; k < 4; k++) {
        corners[k] = still_[k] + across_[k] * sample.lens.x + down_[k] * sample.lens.y;
    }
    return corners;
}

Placement::Corners Placement::at(const Samples::Sample & sample, const glm::mat4x4 & motion) const {
    return moved(motion, sample.lens);
}

glm::mat4x4 Placement::delta(float time) const {
    return motion_.at(time) * fromReference_;
}

bool Placement::reaches(const Samples::Sample & sample) const {
    if (!motion_.moving()) {
        return true;
    }
    const unsigned int slice = shutterSlice(sample.time, shutter_, STEPS);
    const glm::vec2 low = glm::vec2(glm::min(stepMin_[slice], stepMin_[slice + 1])) - glm::vec2(stride_);
    const glm::vec2 high = glm::vec2(glm::max(stepMax_[slice], stepMax_[slice + 1])) + glm::vec2(stride_);
    return sample.raster.x >= low.x && sample.raster.x <= high.x &&
        sample.raster.y >= low.y && sample.raster.y <= high.y;
}

const glm::vec3 & Placement::min() const {
    return min_;
}

const glm::vec3 & Placement::max() const {
    return max_;
}

glm::vec3 Placement::lensed(const glm::vec3 & point, const glm::vec2 & lens) const {
    if (radius_ <= 0.0F) {
        return point;
    }
    const glm::vec2 shift = lens * radius_ * (1.0F - (point.z / focus_));
    return glm::vec3(point.x - shift.x, point.y - shift.y, point.z);
}

Placement::Corners Placement::moved(const glm::mat4x4 & delta, const glm::vec2 & lens) const {
    Corners corners;
    for (unsigned int k = 0; k < 4; k++) {
        corners[k] = state::project(toRaster_, lensed(glm::vec3(delta * glm::vec4(eye_[k], 1.0F)), lens));
    }
    return corners;
}

void Placement::grow(const Corners & corners) {
    for (const glm::vec3 & corner : corners) {
        min_ = glm::min(min_, corner);
        max_ = glm::max(max_, corner);
    }
}

void Placement::stillBound() {
    min_ = glm::vec3(std::numeric_limits<float>::max());
    max_ = glm::vec3(-std::numeric_limits<float>::max());
    Samples::Sample sample;
    for (const glm::vec2 & lens : EXTREMES) {
        sample.lens = lens;
        grow(at(sample));
        if (radius_ <= 0.0F) {
            break;
        }
    }
}

void Placement::sweep() {
    min_ = glm::vec3(std::numeric_limits<float>::max());
    max_ = glm::vec3(-std::numeric_limits<float>::max());
    stride_ = 0.0F;
    Corners previous;
    for (unsigned int i = 0; i <= STEPS; i++) {
        const float time = shutter_.x + ((shutter_.y - shutter_.x) * static_cast<float>(i) / STEPS);
        const glm::mat4x4 moving = delta(time);
        stepMin_[i] = glm::vec3(std::numeric_limits<float>::max());
        stepMax_[i] = glm::vec3(-std::numeric_limits<float>::max());
        for (const glm::vec2 & lens : EXTREMES) {
            for (const glm::vec3 & corner : moved(moving, lens)) {
                stepMin_[i] = glm::min(stepMin_[i], corner);
                stepMax_[i] = glm::max(stepMax_[i], corner);
            }
            if (radius_ <= 0.0F) {
                break;
            }
        }
        min_ = glm::min(min_, stepMin_[i]);
        max_ = glm::max(max_, stepMax_[i]);
        const Corners centre = moved(moving, glm::vec2(0.0F));
        if (i > 0) {
            for (unsigned int k = 0; k < 4; k++) {
                stride_ = std::max(stride_, glm::length(glm::vec2(centre[k]) - glm::vec2(previous[k])));
            }
        }
        previous = centre;
    }
    min_ -= glm::vec3(stride_, stride_, 0.0F);
    max_ += glm::vec3(stride_, stride_, 0.0F);
}

};  // namespace v3d::moya::reyes
