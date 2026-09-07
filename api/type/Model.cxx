/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Model.h"

#include <cstdint>
#include <vector>

namespace v3d::type {

Model::Model() {
}

std::vector<Model::Vertex>& Model::vertices() noexcept {
    return vertices_;
}

const std::vector<Model::Vertex>& Model::vertices() const noexcept {
    return vertices_;
}

std::vector<std::uint32_t>& Model::indices() noexcept {
    return indices_;
}

const std::vector<std::uint32_t>& Model::indices() const noexcept {
    return indices_;
}

Model::Material& Model::material() noexcept {
    return material_;
}

const Model::Material& Model::material() const noexcept {
    return material_;
}

std::size_t Model::vertexBytes() const noexcept {
    return vertices_.size() * sizeof(Vertex);
}

bool Model::empty() const noexcept {
    return vertices_.empty();
}

};  // namespace v3d::type
