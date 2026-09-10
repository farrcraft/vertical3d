/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Parameters.h"

#include <map>
#include <string>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

namespace v3d::render::offline::rib {

namespace {

const std::vector<float> NO_FLOATS;
const std::vector<std::string> NO_STRINGS;

};  // namespace

void ParameterList::add(const std::string & name, const Declaration & declaration,
    const std::vector<float> & values, const std::vector<std::string> & strings) {
    Parameter parameter;
    parameter.declaration = declaration;
    parameter.values = values;
    parameter.strings = strings;
    parameters_[name] = parameter;
}

bool ParameterList::has(const std::string & name) const {
    return parameters_.contains(name);
}

std::vector<std::string> ParameterList::names() const {
    std::vector<std::string> found;
    found.reserve(parameters_.size());
    for (const auto & entry : parameters_) {
        found.push_back(entry.first);
    }
    return found;
}

std::size_t ParameterList::size() const {
    return parameters_.size();
}

const Declaration * ParameterList::declaration(const std::string & name) const {
    const std::map<std::string, Parameter>::const_iterator found = parameters_.find(name);
    return found == parameters_.end() ? nullptr : &found->second.declaration;
}

const std::vector<float> & ParameterList::floats(const std::string & name) const {
    const std::map<std::string, Parameter>::const_iterator found = parameters_.find(name);
    return found == parameters_.end() ? NO_FLOATS : found->second.values;
}

const std::vector<std::string> & ParameterList::strings(const std::string & name) const {
    const std::map<std::string, Parameter>::const_iterator found = parameters_.find(name);
    return found == parameters_.end() ? NO_STRINGS : found->second.strings;
}

std::vector<glm::vec3> ParameterList::points(const std::string & name) const {
    const std::vector<float> & values = floats(name);
    std::vector<glm::vec3> points;
    points.reserve(values.size() / 3);
    for (std::size_t i = 0; i + 2 < values.size(); i += 3) {
        points.push_back(glm::vec3(values[i], values[i + 1], values[i + 2]));
    }
    return points;
}

float ParameterList::number(const std::string & name, float fallback) const {
    const std::vector<float> & values = floats(name);
    return values.empty() ? fallback : values[0];
}

std::string ParameterList::string(const std::string & name, const std::string & fallback) const {
    const std::vector<std::string> & values = strings(name);
    return values.empty() ? fallback : values[0];
}

glm::mat4x4 ParameterList::matrix(const std::string & name) const {
    const std::vector<float> & values = floats(name);
    if (values.size() < 16) {
        return glm::mat4x4(1.0f);
    }
    return glm::make_mat4(values.data());
}

};  // namespace v3d::render::offline::rib
