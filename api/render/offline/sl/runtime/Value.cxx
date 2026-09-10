/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Value.h"

#include <api/render/offline/sl/Types.h>

#include <string>

namespace v3d::render::offline::sl::runtime {

void Value::reset(Type type, Storage storage, unsigned int batch) {
    type_ = type;
    storage_ = storage;
    width_ = storage == Storage::VARYING ? batch : 1;
    if (width_ == 0) {
        width_ = 1;
    }
    numbers_.assign(static_cast<std::size_t>(width_) * sl::components(type), 0.0f);
    text_.clear();
}

Type Value::type() const {
    return type_;
}

Storage Value::storage() const {
    return storage_;
}

unsigned int Value::width() const {
    return width_;
}

unsigned int Value::components() const {
    return sl::components(type_);
}

unsigned int Value::lane(unsigned int point) const {
    // a uniform value answers lane zero for every point, which is what lets one instruction
    // serve both storage classes without asking which it has
    return storage_ == Storage::VARYING && point < width_ ? point : 0;
}

float Value::component(unsigned int point, unsigned int index) const {
    const unsigned int wide = components();
    if (wide == 0 || index >= wide) {
        return 0.0f;
    }
    return numbers_[static_cast<std::size_t>(lane(point)) * wide + index];
}

void Value::component(unsigned int point, unsigned int index, float value) {
    const unsigned int wide = components();
    if (wide == 0 || index >= wide) {
        return;
    }
    numbers_[static_cast<std::size_t>(lane(point)) * wide + index] = value;
}

float Value::number(unsigned int point) const {
    return component(point, 0);
}

void Value::number(unsigned int point, float value) {
    component(point, 0, value);
}

glm::vec3 Value::triple(unsigned int point) const {
    // a float read as a triple replicates, which is RI's promotion rather than a zero fill
    if (components() == 1) {
        return glm::vec3(component(point, 0));
    }
    return glm::vec3(component(point, 0), component(point, 1), component(point, 2));
}

void Value::triple(unsigned int point, const glm::vec3 & value) {
    for (unsigned int i = 0; i < 3 && i < components(); i++) {
        component(point, i, value[static_cast<int>(i)]);
    }
}

glm::mat4x4 Value::matrix(unsigned int point) const {
    if (components() != 16) {
        // a float is that multiple of the identity, which is what "matrix 1" means
        return glm::mat4x4(component(point, 0));
    }
    glm::mat4x4 value(1.0f);
    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            value[column][row] = component(point, static_cast<unsigned int>(column * 4 + row));
        }
    }
    return value;
}

void Value::matrix(unsigned int point, const glm::mat4x4 & value) {
    if (components() != 16) {
        return;
    }
    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            component(point, static_cast<unsigned int>(column * 4 + row), value[column][row]);
        }
    }
}

const std::string & Value::text() const {
    return text_;
}

void Value::text(const std::string & value) {
    text_ = value;
}

void Value::assign(const Value & other, unsigned int point) {
    if (type_ == Type::STRING) {
        text_ = other.text();
        return;
    }
    const unsigned int wide = components();
    if (wide == 0) {
        return;
    }
    if (other.components() == 1) {
        // a float replicates into every component, which is what "color c = 1" says
        const float single = other.number(point);
        for (unsigned int i = 0; i < wide; i++) {
            component(point, i, i < 3 || wide != 16 ? single : 0.0f);
        }
        if (wide == 16) {
            matrix(point, glm::mat4x4(single));
        }
        return;
    }
    for (unsigned int i = 0; i < wide; i++) {
        component(point, i, other.component(point, i));
    }
}

};  // namespace v3d::render::offline::sl::runtime
