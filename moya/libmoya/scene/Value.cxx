/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::scene {

namespace {

/** The floats in a matrix, and in a colour, point, vector or normal. **/
constexpr std::size_t kMatrixFloats = 16;
constexpr std::size_t kTripleFloats = 3;

};  // namespace

const char * name(Type type) {
    switch (type) {
        case Type::Integer: return "integer";
        case Type::Float: return "float";
        case Type::String: return "string";
        case Type::Colour: return "colour";
        case Type::Point: return "point";
        case Type::Vector: return "vector";
        case Type::Normal: return "normal";
        case Type::Matrix: return "matrix";
        case Type::Handle: return "handle";
    }
    return "unknown";
}

std::size_t floatsOf(Type type) {
    switch (type) {
        case Type::Float: return 1;
        case Type::Colour:
        case Type::Point:
        case Type::Vector:
        case Type::Normal: return kTripleFloats;
        case Type::Matrix: return kMatrixFloats;
        case Type::Integer:
        case Type::String:
        case Type::Handle: return 0;
    }
    return 0;
}

bool Handle::valid() const noexcept {
    return generation != 0;
}

Value::Value() : elements_(std::vector<std::int32_t>()) {
}

Value::Value(Type type, std::variant<std::vector<std::int32_t>, std::vector<float>, std::vector<std::string>, std::vector<Handle>> elements,
    std::string space) :
    type_(type), elements_(std::move(elements)), space_(std::move(space)) {
}

Value Value::integers(const std::vector<std::int32_t> & values) {
    return Value(Type::Integer, values);
}

Value Value::floats(const std::vector<float> & values) {
    return Value(Type::Float, values);
}

Value Value::strings(const std::vector<std::string> & values) {
    return Value(Type::String, values);
}

Value Value::triples(Type type, const std::vector<glm::vec3> & values, const std::string & space) {
    std::vector<float> elements;
    elements.reserve(values.size() * kTripleFloats);
    for (const glm::vec3 & value : values) {
        elements.push_back(value.x);
        elements.push_back(value.y);
        elements.push_back(value.z);
    }
    return Value(type, elements, space);
}

Value Value::colours(const std::vector<glm::vec3> & values, const std::string & space) {
    return triples(Type::Colour, values, space);
}

Value Value::points(const std::vector<glm::vec3> & values) {
    return triples(Type::Point, values, "");
}

Value Value::vectors(const std::vector<glm::vec3> & values) {
    return triples(Type::Vector, values, "");
}

Value Value::normals(const std::vector<glm::vec3> & values) {
    return triples(Type::Normal, values, "");
}

Value Value::matrices(const std::vector<glm::mat4> & values) {
    std::vector<float> elements;
    elements.reserve(values.size() * kMatrixFloats);
    for (const glm::mat4 & value : values) {
        for (glm::length_t column = 0; column < 4; ++column) {
            for (glm::length_t row = 0; row < 4; ++row) {
                elements.push_back(value[column][row]);
            }
        }
    }
    return Value(Type::Matrix, elements);
}

Value Value::handles(const std::vector<Handle> & values) {
    return Value(Type::Handle, values);
}

Value Value::integer(std::int32_t value) {
    return integers({ value });
}

Value Value::number(float value) {
    return floats({ value });
}

Value Value::string(const std::string & value) {
    return strings({ value });
}

Value Value::colour(const glm::vec3 & value, const std::string & space) {
    return colours({ value }, space);
}

Value Value::point(const glm::vec3 & value) {
    return points({ value });
}

Value Value::vector(const glm::vec3 & value) {
    return vectors({ value });
}

Value Value::normal(const glm::vec3 & value) {
    return normals({ value });
}

Value Value::matrix(const glm::mat4 & value) {
    return matrices({ value });
}

Value Value::handle(const Handle & value) {
    return handles({ value });
}

Type Value::type() const noexcept {
    return type_;
}

std::size_t Value::size() const noexcept {
    const std::size_t floats = floatsOf(type_);
    if (const std::vector<float> * elements = std::get_if<std::vector<float>>(&elements_); elements != nullptr && floats != 0) {
        return elements->size() / floats;
    }
    if (const std::vector<std::int32_t> * elements = std::get_if<std::vector<std::int32_t>>(&elements_); elements != nullptr) {
        return elements->size();
    }
    if (const std::vector<std::string> * elements = std::get_if<std::vector<std::string>>(&elements_); elements != nullptr) {
        return elements->size();
    }
    if (const std::vector<Handle> * elements = std::get_if<std::vector<Handle>>(&elements_); elements != nullptr) {
        return elements->size();
    }
    return 0;
}

const std::string & Value::space() const noexcept {
    return space_;
}

const std::vector<std::int32_t> & Value::integerArray() const {
    if (type_ != Type::Integer) {
        throw std::logic_error(std::string("a ") + name(type_) + " value read as integers");
    }
    return std::get<std::vector<std::int32_t>>(elements_);
}

const std::vector<float> & Value::floatArray() const {
    if (floatsOf(type_) == 0) {
        throw std::logic_error(std::string("a ") + name(type_) + " value read as floats");
    }
    return std::get<std::vector<float>>(elements_);
}

const std::vector<std::string> & Value::stringArray() const {
    if (type_ != Type::String) {
        throw std::logic_error(std::string("a ") + name(type_) + " value read as strings");
    }
    return std::get<std::vector<std::string>>(elements_);
}

const std::vector<Handle> & Value::handleArray() const {
    if (type_ != Type::Handle) {
        throw std::logic_error(std::string("a ") + name(type_) + " value read as handles");
    }
    return std::get<std::vector<Handle>>(elements_);
}

glm::vec3 Value::triple(std::size_t element) const {
    if (floatsOf(type_) != kTripleFloats) {
        throw std::logic_error(std::string("a ") + name(type_) + " value read as triples");
    }
    const std::vector<float> & elements = std::get<std::vector<float>>(elements_);
    const std::size_t first = element * kTripleFloats;
    return { elements.at(first), elements.at(first + 1), elements.at(first + 2) };
}

glm::mat4 Value::matrixAt(std::size_t element) const {
    if (type_ != Type::Matrix) {
        throw std::logic_error(std::string("a ") + name(type_) + " value read as matrices");
    }
    const std::vector<float> & elements = std::get<std::vector<float>>(elements_);
    const std::size_t first = element * kMatrixFloats;
    if (first + kMatrixFloats > elements.size()) {
        throw std::out_of_range("a matrix value has no element " + std::to_string(element));
    }
    glm::mat4 result(1.0F);
    for (glm::length_t column = 0; column < 4; ++column) {
        for (glm::length_t row = 0; row < 4; ++row) {
            result[column][row] = elements[first + (static_cast<std::size_t>(column) * 4) + static_cast<std::size_t>(row)];
        }
    }
    return result;
}

};  // namespace v3d::moya::scene
