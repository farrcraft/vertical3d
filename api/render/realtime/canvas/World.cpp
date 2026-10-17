/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "World.h"

#include <cstddef>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

namespace v3d::render::realtime::canvas {

/**
 **/
World::Batch::Batch() noexcept :
firstIndex(0),
indices(0) {
}

/**
 **/
World::World() :
    tint_(1.0f) {
}

/**
 **/
void World::clear() {
    vertices_.clear();
    indices_.clear();
    batches_.clear();
    transforms_.reset();
    tint_ = glm::vec4(1.0f);
}

/**
 **/
void World::push() {
    transforms_.push();
}

/**
 **/
void World::pop() {
    transforms_.pop();
}

/**
 **/
void World::transform(const glm::mat4& applied) {
    transforms_.top() = transforms_.top() * applied;
}

/**
 **/
void World::translate(const glm::vec3& offset) {
    transforms_.top() = glm::translate(transforms_.top(), offset);
}

/**
 **/
const glm::mat4& World::transform() const noexcept {
    return transforms_.top();
}

void World::tint(const glm::vec4& colour) {
    tint_ = colour;
}

const glm::vec4& World::tint() const noexcept {
    return tint_;
}

/**
 **/
void World::quad(const Corners& corners, const glm::vec4& colour) {
    // the whole of the white texture, so the sample is opaque white wherever it lands
    quad(corners, glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), colour, TextureHandle());
}

/**
 **/
void World::quad(const Corners& corners, const glm::vec2& uv0, const glm::vec2& uv1,
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
const std::vector<World::Vertex>& World::vertices() const noexcept {
    return vertices_;
}

/**
 **/
const std::vector<uint32_t>& World::indices() const noexcept {
    return indices_;
}

/**
 **/
const std::vector<World::Batch>& World::batches() const noexcept {
    return batches_;
}

/**
 **/
bool World::empty() const noexcept {
    return indices_.empty();
}

/**
 **/
void World::open(const TextureHandle& texture) {
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
void World::vertex(const glm::vec3& position, const glm::vec2& uv, const glm::vec4& colour) {
    Vertex added;
    added.position = glm::vec3(transforms_.top() * glm::vec4(position, 1.0f));
    added.uv = uv;
    added.colour = colour * tint_;
    vertices_.push_back(added);
}

/**
 **/
void World::fan(uint32_t first) {
    const uint32_t order[6] = {0, 1, 2, 2, 3, 0};
    for (std::size_t index = 0; index < 6; index++) {
        indices_.push_back(first + order[index]);
    }
    batches_.back().indices += 6;
}

};  // namespace v3d::render::realtime::canvas
