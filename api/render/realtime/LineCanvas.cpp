/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "LineCanvas.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace v3d::render::realtime {

namespace {

const float twoPi = 6.283185307179586f;

};  // namespace

/**
 **/
LineCanvas::Batch::Batch() noexcept :
clipped(false),
clip(0.0f),
firstVertex(0),
vertices(0) {
}

/**
 **/
LineCanvas::LineCanvas() {
    transforms_.push_back(glm::mat4(1.0f));
}

/**
 **/
void LineCanvas::clear() {
    vertices_.clear();
    batches_.clear();
    transforms_.clear();
    transforms_.push_back(glm::mat4(1.0f));
    clips_.clear();
}

/**
 **/
void LineCanvas::push() {
    transforms_.push_back(transforms_.back());
}

/**
 **/
void LineCanvas::pop() {
    // the identity at the bottom of the stack is the canvas's own and not a caller's to pop
    if (transforms_.size() > 1) {
        transforms_.pop_back();
    }
}

/**
 **/
void LineCanvas::transform(const glm::mat4& transform) {
    transforms_.back() = transforms_.back() * transform;
}

/**
 **/
void LineCanvas::translate(const glm::vec3& offset) {
    glm::mat4& current = transforms_.back();
    current[3][0] += current[0][0] * offset.x + current[1][0] * offset.y + current[2][0] * offset.z;
    current[3][1] += current[0][1] * offset.x + current[1][1] * offset.y + current[2][1] * offset.z;
    current[3][2] += current[0][2] * offset.x + current[1][2] * offset.y + current[2][2] * offset.z;
}

/**
 **/
const glm::mat4& LineCanvas::transform() const noexcept {
    return transforms_.back();
}

/**
 **/
void LineCanvas::clip(const glm::vec2& min, const glm::vec2& max) {
    glm::vec4 rect(std::min(min.x, max.x), std::min(min.y, max.y),
        std::max(min.x, max.x), std::max(min.y, max.y));

    if (!clips_.empty()) {
        const glm::vec4& outer = clips_.back();
        rect.x = std::max(rect.x, outer.x);
        rect.y = std::max(rect.y, outer.y);
        rect.z = std::min(rect.z, outer.z);
        rect.w = std::min(rect.w, outer.w);
    }
    // two clips that miss each other leave nothing rather than an inverted rectangle,
    // which is a validation error by the time it reaches a scissor
    rect.z = std::max(rect.x, rect.z);
    rect.w = std::max(rect.y, rect.w);

    clips_.push_back(rect);
}

/**
 **/
void LineCanvas::unclip() {
    if (!clips_.empty()) {
        clips_.pop_back();
    }
}

/**
 **/
void LineCanvas::open() {
    const bool clipped = !clips_.empty();
    const glm::vec4 clip = clipped ? clips_.back() : glm::vec4(0.0f);

    if (!batches_.empty() && batches_.back().clipped == clipped && batches_.back().clip == clip) {
        return;
    }
    Batch batch;
    batch.clipped = clipped;
    batch.clip = clip;
    batch.firstVertex = static_cast<uint32_t>(vertices_.size());
    batches_.push_back(batch);
}

/**
 **/
void LineCanvas::vertex(const glm::vec3& position, const glm::vec4& colour) {
    const glm::mat4& current = transforms_.back();

    Vertex added;
    added.position.x = current[0][0] * position.x + current[1][0] * position.y + current[2][0] * position.z + current[3][0];
    added.position.y = current[0][1] * position.x + current[1][1] * position.y + current[2][1] * position.z + current[3][1];
    added.position.z = current[0][2] * position.x + current[1][2] * position.y + current[2][2] * position.z + current[3][2];
    added.colour = colour;
    vertices_.push_back(added);
}

/**
 **/
void LineCanvas::line(const glm::vec3& from, const glm::vec3& to, const glm::vec4& colour) {
    open();
    vertex(from, colour);
    vertex(to, colour);
    batches_.back().vertices += 2;
}

/**
 **/
void LineCanvas::polyline(const std::vector<glm::vec3>& points, const glm::vec4& colour, bool closed) {
    if (points.size() < 2) {
        return;
    }
    for (std::size_t index = 1; index < points.size(); index++) {
        line(points[index - 1], points[index], colour);
    }
    if (closed) {
        line(points.back(), points.front(), colour);
    }
}

/**
 **/
void LineCanvas::box(const glm::vec3& min, const glm::vec3& max, const glm::vec4& colour) {
    const glm::vec3 corner[8] = {
        glm::vec3(min.x, min.y, min.z),
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(min.x, max.y, max.z)
    };
    // the near face, the far face, and the four struts between them
    const unsigned int edge[24] = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7
    };
    for (std::size_t index = 0; index < 24; index += 2) {
        line(corner[edge[index]], corner[edge[index + 1]], colour);
    }
}

/**
 **/
void LineCanvas::circle(const glm::vec3& centre, const glm::vec3& axisU, const glm::vec3& axisV, float radius,
    unsigned int sides, const glm::vec4& colour) {
    if (sides < 3) {
        return;
    }

    const float step = twoPi / static_cast<float>(sides);
    glm::vec3 previous = centre + axisU * radius;
    // the last step is a whole turn, so it lands back on the start and the ring closes
    // without a wrap case of its own
    for (unsigned int side = 1; side <= sides; side++) {
        const float angle = step * static_cast<float>(side);
        const glm::vec3 point = centre + axisU * (std::cos(angle) * radius) + axisV * (std::sin(angle) * radius);
        line(previous, point, colour);
        previous = point;
    }
}

/**
 **/
const std::vector<LineCanvas::Vertex>& LineCanvas::vertices() const noexcept {
    return vertices_;
}

/**
 **/
const std::vector<LineCanvas::Batch>& LineCanvas::batches() const noexcept {
    return batches_;
}

/**
 **/
bool LineCanvas::empty() const noexcept {
    return vertices_.empty();
}

};  // namespace v3d::render::realtime
