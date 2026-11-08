/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Primvars.h"

#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/type/Checked.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::ri::internal {

namespace {

typedef v3d::render::offline::rib::Declaration Declaration;

/** The floats of a matrix, and of an hpoint. **/
constexpr std::size_t kMatrixFloats = 16;
constexpr std::size_t kHpointFloats = 4;

/** The integers a parameter may hold, which every float in the range converts to exactly. **/
constexpr std::int32_t kLargest = 2000000000;
constexpr std::int32_t kSmallest = -kLargest;

/** Floats in threes. **/
std::vector<glm::vec3> triples(const std::vector<float> & floats) {
    std::vector<glm::vec3> found;
    found.reserve(floats.size() / 3);
    for (std::size_t i = 0; i + 2 < floats.size(); i += 3) {
        found.emplace_back(floats[i], floats[i + 1], floats[i + 2]);
    }
    return found;
}

};  // namespace

std::optional<scene::Value> valueOf(const v3d::render::offline::rib::ParameterList & parameters, const std::string & name) {
    const Declaration * declaration = parameters.declaration(name);
    if (declaration == nullptr) {
        return std::nullopt;
    }
    const std::vector<float> & floats = parameters.floats(name);
    switch (declaration->type()) {
        case Declaration::Type::FLOAT: return scene::Value::floats(floats);
        case Declaration::Type::INTEGER: {
            // an integer too large for the scene's drops the parameter, as one the reader cannot
            // type does
            std::vector<std::int32_t> integers;
            integers.reserve(floats.size());
            for (const float value : floats) {
                const std::optional<std::int32_t> integer = v3d::type::toInteger(value, {.minimum = kSmallest, .maximum = kLargest});
                if (!integer) {
                    return std::nullopt;
                }
                integers.push_back(*integer);
            }
            return scene::Value::integers(integers);
        }
        case Declaration::Type::STRING: return scene::Value::strings(parameters.strings(name));
        case Declaration::Type::COLOR: return scene::Value::colours(triples(floats));
        case Declaration::Type::POINT: return scene::Value::points(triples(floats));
        case Declaration::Type::VECTOR: return scene::Value::vectors(triples(floats));
        case Declaration::Type::NORMAL: return scene::Value::normals(triples(floats));
        case Declaration::Type::MATRIX: {
            std::vector<glm::mat4> matrices;
            for (std::size_t i = 0; i + kMatrixFloats <= floats.size(); i += kMatrixFloats) {
                matrices.push_back(glm::make_mat4(&floats[i]));
            }
            return scene::Value::matrices(matrices);
        }
        case Declaration::Type::HPOINT: {
            std::vector<glm::vec3> points;
            for (std::size_t i = 0; i + kHpointFloats <= floats.size(); i += kHpointFloats) {
                const float w = floats[i + 3] == 0.0F ? 1.0F : floats[i + 3];
                points.emplace_back(floats[i] / w, floats[i + 1] / w, floats[i + 2] / w);
            }
            return scene::Value::points(points);
        }
    }
    return std::nullopt;
}

scene::Interpolation interpolationOf(Declaration::Storage storage) {
    switch (storage) {
        case Declaration::Storage::CONSTANT: return scene::Interpolation::Constant;
        case Declaration::Storage::UNIFORM: return scene::Interpolation::Uniform;
        case Declaration::Storage::VARYING: return scene::Interpolation::Varying;
        case Declaration::Storage::VERTEX: return scene::Interpolation::Vertex;
    }
    return scene::Interpolation::Constant;
}

void declare(scene::Scene * scene, const scene::Handle & object, const v3d::render::offline::rib::ParameterList & parameters,
    const std::set<std::string> & except, bool constant) {
    for (const std::string & name : parameters.names()) {
        if (except.contains(name)) {
            continue;
        }
        const std::optional<scene::Value> value = valueOf(parameters, name);
        if (!value) {
            continue;
        }
        const Declaration * declared = parameters.declaration(name);
        const bool one = constant && value->size() == 1;
        scene->declare(object, scene::Declaration { .name = name,
            .type = value->type(),
            .count = one ? 1U : scene::Declaration::kAny,
            .fallback = *value,
            .interpolation = constant ? scene::Interpolation::Constant : interpolationOf(declared->storage()) });
    }
}

};  // namespace v3d::moya::ri::internal
