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
 * The buffer is **one element wide when uniform and one per shading point when varying**,
 * and `lane()` is what makes the two read the same way - a uniform value answers lane zero
 * for every point. That is the whole of why a grid of a hundred points and talyn's single
 * hit are one code path rather than two: the batch is the execution model, and a batch of
 * one is not a special case of it.
 *
 * A string is uniform always. There is no per-point coordinate space to name, and allowing
 * one would make every transform a runtime string lookup.
 **/
class Value final {
 public:
    /**
     * Size the buffer for a type, a storage class and a batch. A varying value takes the
     * batch; a uniform one takes a single element whatever the batch is.
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
     * which is what lets one instruction serve both storage classes.
     **/
    unsigned int lane(unsigned int point) const;

    float number(unsigned int point) const;
    void number(unsigned int point, float value);
    glm::vec3 triple(unsigned int point) const;
    void triple(unsigned int point, const glm::vec3 & value);
    glm::mat4x4 matrix(unsigned int point) const;
    void matrix(unsigned int point, const glm::mat4x4 & value);

    /**
     * One component of one element, which is what an instruction that does not care what
     * the type means walks over.
     **/
    float component(unsigned int point, unsigned int index) const;
    void component(unsigned int point, unsigned int index, float value);

    const std::string & text() const;
    void text(const std::string & value);

    /**
     * Take another value's contents, converting: a float replicates across the components,
     * and the three point-like types keep theirs. The batch is this value's own, so a
     * uniform source spreads over a varying destination.
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
