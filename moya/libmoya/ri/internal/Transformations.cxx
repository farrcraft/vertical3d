/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Transformations.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>

#include "Sampled.h"

namespace v3d::moya::ri::internal {

const Sampled & Transformations::current() const {
    return current_;
}

void Transformations::restore(const Sampled & saved) {
    current_ = saved;
}

void Transformations::reset() {
    current_ = Sampled();
}

Sampled Transformations::worldBegin() {
    // the transformation now takes world space to camera space, and the camera holds the other way
    Sampled camera = current_;
    for (glm::mat4 & matrix : camera.matrices) {
        matrix = glm::inverse(matrix);
    }
    current_ = Sampled();
    spaces_.clear();
    spaces_["camera"] = camera;
    return camera;
}

void Transformations::push() {
    saved_.push_back(current_);
}

void Transformations::pop() {
    if (saved_.empty()) {
        return;
    }
    current_ = saved_.back();
    saved_.pop_back();
}

void Transformations::concat(const glm::mat4 & matrix) {
    if (motion_) {
        // each request builds on the transformation the block opened with, at its own time
        const std::size_t at = poses_.size();
        const bool same = base_.times == times_;
        poses_.push_back((same && at < base_.matrices.size() ? base_.matrices[at] : base_.matrices.front()) * matrix);
        return;
    }
    for (glm::mat4 & sample : current_.matrices) {
        sample = sample * matrix;
    }
}

void Transformations::replace(const glm::mat4 & matrix) {
    if (motion_) {
        poses_.push_back(matrix);
        return;
    }
    current_ = Sampled { .times = {}, .matrices = { matrix } };
}

void Transformations::motionBegin(const std::vector<float> & times) {
    motion_ = true;
    times_ = times;
    base_ = current_;
    poses_.clear();
}

bool Transformations::inMotion() const {
    return motion_;
}

const std::vector<float> & Transformations::times() const {
    return times_;
}

void Transformations::motionEnd() {
    if (!motion_) {
        return;
    }
    motion_ = false;
    const std::size_t count = std::min(poses_.size(), times_.size());
    if (count == 1 || (count == 0 && !poses_.empty())) {
        current_ = Sampled { .times = {}, .matrices = { poses_.front() } };
    } else if (count > 1) {
        current_ = Sampled { .times = std::vector<float>(times_.begin(), times_.begin() + static_cast<std::ptrdiff_t>(count)),
            .matrices = std::vector<glm::mat4>(poses_.begin(), poses_.begin() + static_cast<std::ptrdiff_t>(count)) };
    }
}

void Transformations::name(const std::string & space) {
    if (!space.empty()) {
        spaces_[space] = current_;
    }
}

bool Transformations::use(const std::string & space) {
    if (space == "world") {
        current_ = Sampled();
        return true;
    }
    const auto found = spaces_.find(space);
    if (found == spaces_.end()) {
        return false;
    }
    current_ = found->second;
    return true;
}

std::optional<glm::mat4> Transformations::toWorld(const std::string & space) const {
    if (space == "object") {
        return current_.matrices.front();
    }
    if (space == "world") {
        return glm::mat4(1.0F);
    }
    const auto found = spaces_.find(space);
    if (found == spaces_.end()) {
        return std::nullopt;
    }
    return found->second.matrices.front();
}

};  // namespace v3d::moya::ri::internal
