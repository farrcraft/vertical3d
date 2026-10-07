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

std::vector<Model::Material>& Model::materials() noexcept {
    return materials_;
}

const std::vector<Model::Material>& Model::materials() const noexcept {
    return materials_;
}

std::vector<Model::Part>& Model::parts() noexcept {
    return parts_;
}

const std::vector<Model::Part>& Model::parts() const noexcept {
    return parts_;
}

Skeleton& Model::skeleton() noexcept {
    return skeleton_;
}

const Skeleton& Model::skeleton() const noexcept {
    return skeleton_;
}

std::vector<Model::Influence>& Model::influences() noexcept {
    return influences_;
}

const std::vector<Model::Influence>& Model::influences() const noexcept {
    return influences_;
}

std::vector<animation::Clip>& Model::clips() noexcept {
    return clips_;
}

const std::vector<animation::Clip>& Model::clips() const noexcept {
    return clips_;
}

std::size_t Model::vertexBytes() const noexcept {
    return vertices_.size() * sizeof(Vertex);
}

bool Model::empty() const noexcept {
    return vertices_.empty();
}

};  // namespace v3d::type
