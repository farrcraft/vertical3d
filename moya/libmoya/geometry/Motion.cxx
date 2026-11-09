/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/Motion.h>

#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/scene/Samples.h>

#include <cmath>
#include <cstddef>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::geometry {

namespace {

/** The most steps the polar decomposition takes, which converges far sooner. **/
constexpr int kSteps = 100;

/** How near two steps come before the decomposition stops. **/
constexpr double kConverged = 1.0e-12;

/** A linear part of a determinant this small flattens space, and has no rotation. **/
constexpr double kFlat = 1.0e-30;

/** The largest of the differences between two matrices' elements. **/
double largest(const glm::dmat3 & first, const glm::dmat3 & second) {
    double out = 0.0;
    for (int column = 0; column < 3; ++column) {
        for (int row = 0; row < 3; ++row) {
            out = std::fmax(out, std::fabs(first[column][row] - second[column][row]));
        }
    }
    return out;
}

};  // namespace

kernel::geometry::Key decompose(const glm::mat4 & transform, float time) {
    kernel::geometry::Key out;
    out.time = time;
    out.translation = glm::vec3(transform[3]);
    const glm::dmat3 linear { glm::mat3(transform) };
    if (std::fabs(glm::determinant(linear)) < kFlat) {
        out.scale = glm::mat3(linear);
        return out;
    }
    // the rotation is the limit of averaging a matrix with its inverse transpose
    glm::dmat3 rotation = linear;
    for (int step = 0; step < kSteps; ++step) {
        const glm::dmat3 next = (rotation + glm::inverse(glm::transpose(rotation))) * 0.5;
        const double change = largest(next, rotation);
        rotation = next;
        if (change < kConverged) {
            break;
        }
    }
    if (glm::determinant(rotation) < 0.0) {
        rotation = -rotation;
    }
    out.rotation = glm::normalize(glm::quat(glm::mat3(rotation)));
    out.scale = glm::mat3(glm::transpose(rotation) * linear);
    return out;
}

std::vector<kernel::geometry::Key> keys(const scene::Samples & samples) {
    std::vector<kernel::geometry::Key> out;
    out.reserve(samples.values.size());
    for (std::size_t i = 0; i < samples.values.size(); ++i) {
        const float time = i < samples.times.size() ? samples.times[i] : 0.0F;
        kernel::geometry::Key key = decompose(samples.values[i].matrixAt(0), time);
        if (!out.empty() && glm::dot(out.back().rotation, key.rotation) < 0.0F) {
            key.rotation = -key.rotation;
        }
        out.push_back(key);
    }
    return out;
}

};  // namespace v3d::moya::geometry
