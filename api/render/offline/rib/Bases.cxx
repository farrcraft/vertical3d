/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bases.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>

#include "Requests.h"

namespace v3d::render::offline::rib {

namespace {

constexpr float SIXTH = 1.0F / 6.0F;
constexpr float TWO_THIRDS = 4.0F / 6.0F;

/**
 * RI's standard bases, as its specification gives them.
 **/
constexpr auto STANDARD = std::to_array<std::pair<const char*, Basis::Matrix>>({
    { "bezier", Basis::BEZIER },
    { "b-spline", { -SIXTH, 0.5F, -0.5F, SIXTH, 0.5F, -1, 0.5F, 0, -0.5F, 0, 0.5F, 0, SIXTH, TWO_THIRDS, SIXTH, 0 } },
    { "catmull-rom", { -0.5F, 1.5F, -1.5F, 0.5F, 1, -2.5F, 2, -0.5F, -0.5F, 0, 0.5F, 0, 0, 1, 0, 0 } },
    { "hermite", { 2, 1, -2, 1, -3, -2, 3, -1, 0, 1, 0, 0, 1, 0, 0, 0 } },
    { "power", { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 } },
});

};  // namespace

bool namedBasis(const std::string & name, Basis::Matrix * matrix) {
    const auto found = std::ranges::find_if(STANDARD, [&name](const auto& known) { return name == known.first; });
    if (found == STANDARD.end()) {
        return false;
    }
    *matrix = found->second;
    return true;
}

std::string basisName(const Basis::Matrix & matrix) {
    const auto found = std::ranges::find_if(STANDARD, [&matrix](const auto& known) { return matrix == known.second; });
    return found == STANDARD.end() ? std::string() : std::string(found->first);
}

};  // namespace v3d::render::offline::rib
