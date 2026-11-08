/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Schema.h>

#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace v3d::moya::scene {

const char * name(Interpolation interpolation) {
    switch (interpolation) {
        case Interpolation::Constant: return "constant";
        case Interpolation::Uniform: return "uniform";
        case Interpolation::Varying: return "varying";
        case Interpolation::Vertex: return "vertex";
        case Interpolation::FaceVarying: return "facevarying";
    }
    return "unknown";
}

void Schema::check(const Declaration & declaration) const {
    if (declaration.name.empty()) {
        throw std::invalid_argument("a parameter with no name");
    }
    if (declaration.fallback.type() != declaration.type) {
        throw std::invalid_argument("parameter " + declaration.name + " is a " + name(declaration.type) + " with a fallback of type " +
            name(declaration.fallback.type()));
    }
    if (!declaration.array && declaration.fallback.size() != 1) {
        throw std::invalid_argument("parameter " + declaration.name + " holds one element, and its fallback holds " +
            std::to_string(declaration.fallback.size()));
    }
    if (declaration.type == Type::Handle) {
        if (!find(declaration.target)) {
            throw std::invalid_argument("parameter " + declaration.name + " names no kind of object: " + declaration.target);
        }
    } else if (!declaration.target.empty()) {
        throw std::invalid_argument("parameter " + declaration.name + " names a kind but is not a handle");
    }
}

std::uint32_t Schema::add(const Kind & kind) {
    if (kind.name.empty()) {
        throw std::invalid_argument("a kind with no name");
    }
    if (find(kind.name)) {
        throw std::invalid_argument("two kinds named " + kind.name);
    }
    for (std::size_t i = 0; i < kind.parameters.size(); ++i) {
        check(kind.parameters[i]);
        for (std::size_t before = 0; before < i; ++before) {
            if (kind.parameters[before].name == kind.parameters[i].name) {
                throw std::invalid_argument("kind " + kind.name + " has two parameters named " + kind.parameters[i].name);
            }
        }
    }
    kinds_.push_back(kind);
    return static_cast<std::uint32_t>(kinds_.size() - 1);
}

const Kind & Schema::kind(std::uint32_t index) const {
    return kinds_.at(index);
}

std::optional<std::uint32_t> Schema::find(const std::string & name) const {
    for (std::size_t i = 0; i < kinds_.size(); ++i) {
        if (kinds_[i].name == name) {
            return static_cast<std::uint32_t>(i);
        }
    }
    return std::nullopt;
}

std::uint32_t Schema::size() const noexcept {
    return static_cast<std::uint32_t>(kinds_.size());
}

};  // namespace v3d::moya::scene
