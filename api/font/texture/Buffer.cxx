/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Buffer.h"

#include <cstddef>
#include <vector>

namespace v3d::font::texture {

Buffer::Buffer() : dirty_(true) {
}

void Buffer::addVertex(const glm::vec3& vertex) {
    vertices_.push_back(vertex);
}

void Buffer::addIndex(unsigned int index) {
    indices_.push_back(index);
}

void Buffer::addColor(const glm::vec4& color) {
    colors_.push_back(color);
}

void Buffer::addTextureCoordinate(const glm::vec2& uv) {
    uvs_.push_back(uv);
}

void Buffer::invalidate() {
    dirty_ = true;
}

void Buffer::dirty(bool state) {
    dirty_ = state;
}

bool Buffer::dirty() const {
    return dirty_;
}

std::vector<glm::vec3>& Buffer::vertices() {
    return vertices_;
}

std::vector<glm::vec2>& Buffer::uvs() {
    return uvs_;
}

std::vector<glm::vec4>& Buffer::colors() {
    return colors_;
}

std::vector<unsigned int>& Buffer::indices() {
    return indices_;
}

const std::vector<glm::vec3>& Buffer::vertices() const {
    return vertices_;
}

const std::vector<glm::vec2>& Buffer::uvs() const {
    return uvs_;
}

const std::vector<glm::vec4>& Buffer::colors() const {
    return colors_;
}

const std::vector<unsigned int>& Buffer::indices() const {
    return indices_;
}

void Buffer::clear() {
    indices_.clear();
    vertices_.clear();
    uvs_.clear();
    colors_.clear();
    dirty_ = true;
}

void Buffer::resize(size_t size) {
    vertices_.resize(size);
    uvs_.resize(size);
    colors_.resize(size);
    indices_.resize(size);
}

};  // namespace v3d::font::texture
