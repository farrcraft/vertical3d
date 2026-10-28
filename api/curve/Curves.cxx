/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Curves.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "Curve.h"

#include <glm/vec3.hpp>

namespace v3d::curve {

namespace {

/**
 * Check a curve, and settle a Bezier curve's handles, before it is stored.
 **/
std::string prepare(Curve* curve) {
    std::string refused = problem(*curve);
    if (refused.empty()) {
        if (Bezier* bezier = std::get_if<Bezier>(&curve->shape)) {
            settle(bezier, curve->closed);
        }
    }
    return refused;
}

};  // namespace

std::size_t Curves::size(void) const noexcept {
    return curves_.size();
}

bool Curves::empty(void) const noexcept {
    return curves_.empty();
}

const Curve& Curves::curve(std::size_t index) const {
    return curves_.at(index);
}

std::string Curves::add(Curve curve) {
    std::string refused = prepare(&curve);
    if (!refused.empty()) {
        return refused;
    }
    curves_.push_back(std::move(curve));
    evaluated_.emplace_back();
    changes_++;
    return {};
}

std::string Curves::set(std::size_t index, Curve curve) {
    std::string refused = prepare(&curve);
    if (!refused.empty()) {
        return refused;
    }
    curves_.at(index) = std::move(curve);
    evaluated_[index].clear();
    changes_++;
    return {};
}

void Curves::remove(std::size_t index) {
    curves_.erase(curves_.begin() + static_cast<std::ptrdiff_t>(index));
    evaluated_.erase(evaluated_.begin() + static_cast<std::ptrdiff_t>(index));
    changes_++;
}

const std::vector<glm::vec3>& Curves::evaluated(std::size_t index) const {
    // a curve always evaluates to at least two points, so an empty list is one not yet worked out
    std::vector<glm::vec3>& points = evaluated_.at(index);
    if (points.empty()) {
        points = evaluate(curves_[index], resolution_);
    }
    return points;
}

unsigned int Curves::resolution(void) const noexcept {
    return resolution_;
}

void Curves::resolution(unsigned int pieces) {
    resolution_ = std::max(pieces, 1U);
    for (std::vector<glm::vec3>& points : evaluated_) {
        points.clear();
    }
    changes_++;
}

std::uint64_t Curves::changes(void) const noexcept {
    return changes_;
}

};  // namespace v3d::curve
