/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/Types.h>

#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::sl::runtime {

/**
 * A value a shader run holds: a type, a storage class and a buffer.
 *
 * The buffer is **one element wide when uniform and one per shading point when varying**.
 * `lane()` makes the two read the same way: a uniform value returns lane zero for every
 * point. A grid of many points and a single traced hit therefore share one code path.
 *
 * A string is always uniform. A coordinate space named by a string is therefore one matrix
 * for the whole batch.
 **/
class Value final {
 public:
    /**
     * Size the buffer for a type, a storage class and a batch. A varying value holds one
     * element per point of the batch. A uniform value holds one element whatever the batch.
     **/
    void reset(Type type, Storage storage, unsigned int batch);

    Type type() const;
    Storage storage() const;
    /**
     * How many elements the buffer holds: one when uniform, the batch when varying.
     **/
    unsigned int width() const;
    /**
     * How many floats one element is: one for a float, three for a point or a colour,
     * sixteen for a matrix, none for a string.
     **/
    unsigned int components() const;

    /**
     * Which element a given shading point reads. Zero for a uniform value at every point,
     * so one instruction serves both storage classes.
     **/
    unsigned int lane(unsigned int point) const;

    float number(unsigned int point) const;
    void number(unsigned int point, float value);
    glm::vec3 triple(unsigned int point) const;
    void triple(unsigned int point, const glm::vec3 & value);
    glm::mat4x4 matrix(unsigned int point) const;
    void matrix(unsigned int point, const glm::mat4x4 & value);

    /**
     * One component of one element, for an instruction that works a component at a time
     * whatever the type.
     **/
    float component(unsigned int point, unsigned int index) const;
    void component(unsigned int point, unsigned int index, float value);

    const std::string & text() const;
    void text(const std::string & value);

    /**
     * Copy another value's contents at one point, converting. A float fills every component,
     * except that a matrix takes it on the diagonal. The three point-like types copy their
     * components unchanged. A uniform source is read at lane zero for every point.
     **/
    void assign(const Value & other, unsigned int point);

 private:
    Type type_ = Type::FLOAT;
    Storage storage_ = Storage::UNIFORM;
    unsigned int width_ = 1;
    std::vector<float> numbers_;
    std::string text_;
};

};  // namespace v3d::render::offline::sl::runtime
