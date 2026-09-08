/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "WorldCanvas.h"

#include <cstddef>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

namespace v3d::render::realtime {

/**
 **/
WorldCanvas::Batch::Batch() noexcept :
firstIndex(0),
indices(0) {
}

/**
 **/
WorldCanvas::WorldCanvas() {
    transforms_.push_back(glm::mat4(1.0f));
}

/**
 **/
void WorldCanvas::clear() {
    vertices_.clear();
    indices_.clear();
    batches_.clear();
    transforms_.clear();
    transforms_.push_back(glm::mat4(1.0f));
}

/**
 **/
void WorldCanvas::push() {
    transforms_.push_back(transforms_.back());
}

/**
 **/
void WorldCanvas::pop() {
    // the identity at the bottom of the stack is the canvas's own and not a caller's to pop
    if (transforms_.size() > 1) {
        transforms_.pop_back();
    }
}

/**
 **/
void WorldCanvas::transform(const glm::mat4& applied) {
    transforms_.back() = transforms_.back() * applied;
}

/**
 **/
void WorldCanvas::translate(const glm::vec3& offset) {
    transforms_.back() = glm::translate(transforms_.back(), offset);
}

/**
 **/
const glm::mat4& WorldCanvas::transform() const noexcept {
    return transforms_.back();
}

/**
 **/
void WorldCanvas::quad(const Corners& corners, const glm::vec4& colour) {
    // the whole of the white texture, so the sample is opaque white wherever it lands
    quad(corners, glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), colour, TextureHandle());
}

/**
 **/
void WorldCanvas::quad(const Corners& corners, const glm::vec2& uv0, const glm::vec2& uv1,
    const glm::vec4& colour, const TextureHandle& texture) {
    open(texture);

    const uint32_t first = static_cast<uint32_t>(vertices_.size());
    vertex(corners[0], glm::vec2(uv0.x, uv0.y), colour);
    vertex(corners[1], glm::vec2(uv1.x, uv0.y), colour);
    vertex(corners[2], glm::vec2(uv1.x, uv1.y), colour);
    vertex(corners[3], glm::vec2(uv0.x, uv1.y), colour);
    fan(first);
}

/**
 **/
const std::vector<WorldCanvas::Vertex>& WorldCanvas::vertices() const noexcept {
    return vertices_;
}

/**
 **/
const std::vector<uint32_t>& WorldCanvas::indices() const noexcept {
    return indices_;
}

/**
 **/
const std::vector<WorldCanvas::Batch>& WorldCanvas::batches() const noexcept {
    return batches_;
}

/**
 **/
bool WorldCanvas::empty() const noexcept {
    return indices_.empty();
}

/**
 **/
void WorldCanvas::open(const TextureHandle& texture) {
    if (!batches_.empty() && batches_.back().texture == texture) {
        return;
    }
    Batch batch;
    batch.texture = texture;
    batch.firstIndex = static_cast<uint32_t>(indices_.size());
    batch.indices = 0;
    batches_.push_back(batch);
}

/**
 **/
void WorldCanvas::vertex(const glm::vec3& position, const glm::vec2& uv, const glm::vec4& colour) {
    Vertex added;
    added.position = glm::vec3(transforms_.back() * glm::vec4(position, 1.0f));
    added.uv = uv;
    added.colour = colour;
    vertices_.push_back(added);
}

/**
 **/
void WorldCanvas::fan(uint32_t first) {
    const uint32_t order[6] = {0, 1, 2, 2, 3, 0};
    for (std::size_t index = 0; index < 6; index++) {
        indices_.push_back(first + order[index]);
    }
    batches_.back().indices += 6;
}

};  // namespace v3d::render::realtime
