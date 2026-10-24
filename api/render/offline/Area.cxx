/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Area.h"

#include <api/type/Random.h>

#include "Uniform.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

namespace v3d::render::offline {

namespace {

constexpr float PI = 3.14159265358979F;

/**
 * FNV-1a over the handle's bytes. It is written here rather than taken from std::hash, whose
 * values differ between standard libraries, so a light's samples are the same everywhere.
 **/
std::uint64_t fnv(const std::string & text) {
    constexpr std::uint64_t offset = 14695981039346656037ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;
    std::uint64_t hash = offset;
    for (const char letter : text) {
        hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(letter));
        hash *= prime;
    }
    return hash;
}

};  // namespace

Area::Area(const std::string & handle) : key_(fnv(handle)) {
}

void Area::add(const glm::vec3 & first, const glm::vec3 & second, const glm::vec3 & third) {
    // the triangles come before the spheres in the running sum, so a triangle is inserted
    // after the last triangle and every sphere's sum grows by its area
    const float size = 0.5F * glm::length(glm::cross(second - first, third - first));
    const std::size_t at = triangles_.size();
    const float before = at == 0 ? 0.0F : running_[at - 1];
    triangles_.push_back({first, second, third});
    running_.insert(running_.begin() + static_cast<std::ptrdiff_t>(at), before + size);
    for (std::size_t k = at + 1; k < running_.size(); k++) {
        running_[k] += size;
    }
}

void Area::add(const Sphere & sphere) {
    const float scale = std::cbrt(std::fabs(glm::determinant(glm::mat3(sphere.placement))));
    const float radius = std::fabs(sphere.radius) * scale;
    const float size = 4.0F * PI * radius * radius;
    spheres_.push_back(sphere);
    running_.push_back((running_.empty() ? 0.0F : running_.back()) + size);
}

bool Area::empty() const {
    return running_.empty() || !(running_.back() > 0.0F);
}

float Area::area() const {
    return running_.empty() ? 0.0F : running_.back();
}

glm::vec3 Area::sample(const glm::vec2 & square) const {
    if (empty()) {
        return glm::vec3(0.0F);
    }
    // the first number picks a piece by area, and what is left of it, rescaled to [0, 1),
    // places the point on that piece
    const float wanted = square.x * running_.back();
    const auto found = std::upper_bound(running_.begin(), running_.end(), wanted);
    const std::size_t piece = std::min(static_cast<std::size_t>(std::distance(running_.begin(), found)),
        running_.size() - 1);
    const float low = piece == 0 ? 0.0F : running_[piece - 1];
    const float span = running_[piece] - low;
    const float rest = span > 0.0F ? std::clamp((wanted - low) / span, 0.0F, 0.99999994F) : 0.0F;
    const glm::vec2 within(rest, square.y);
    if (piece < triangles_.size()) {
        const Corners & corners = triangles_[piece];
        return uniform::triangle(corners.a, corners.b, corners.c, within);
    }
    const Sphere & sphere = spheres_[piece - triangles_.size()];
    return glm::vec3(sphere.placement * glm::vec4(sphere.radius * uniform::sphere(within), 1.0F));
}

glm::vec3 Area::point(const glm::vec3 & shading, unsigned int index) const {
    // each coordinate's bits and the index are folded into the key one at a time, through the
    // generator's own mixing, so two points a bit apart start two unrelated sequences
    std::uint64_t seed = key_;
    for (int axis = 0; axis < 3; axis++) {
        seed = v3d::type::Random(seed ^ std::bit_cast<std::uint32_t>(shading[axis])).next();
    }
    v3d::type::Random random(seed ^ index);
    const float first = random.unit();
    const float second = random.unit();
    return sample(glm::vec2(first, second));
}

unsigned int Area::samples() const {
    return samples_;
}

void Area::samples(unsigned int count) {
    samples_ = count == 0 ? 1U : count;
}

std::uint64_t Area::key() const {
    return key_;
}

};  // namespace v3d::render::offline
