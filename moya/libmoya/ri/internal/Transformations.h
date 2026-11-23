/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include "Sampled.h"

namespace v3d::moya::ri::internal {

/**
 * RI's current transformation, the stack TransformBegin saves it on, an open motion block, and
 * the spaces CoordinateSystem names.
 *
 * Each transform request applies to the current transformation, which holds a sample at each
 * time of a motion block. Inside a block, each request gives the transformation at the block's
 * next time, built on the transformation the block opened with. Outside a block, a request
 * applies to every sample, so what follows a block moves with it.
 **/
class Transformations final {
 public:
    /** The current transformation. **/
    const Sampled & current() const;

    /** Make a saved transformation current, as AttributeEnd does. **/
    void restore(const Sampled & saved);

    /** Make the identity current, as an object's start does. **/
    void reset();

    /**
     * Take WorldBegin: the current transformation becomes the camera's and the identity is made
     * current. The spaces the world before named are forgotten, and `"camera"` names the camera's.
     *
     * @return the camera's transformation, from camera space to world space
     **/
    Sampled worldBegin();

    /** Save the current transformation, as TransformBegin does. **/
    void push();

    /** Make the transformation the last push() saved current. With none saved, nothing changes. **/
    void pop();

    /** A transform request that concatenates a matrix to the current transformation. **/
    void concat(const glm::mat4 & matrix);

    /** A transform request that replaces the current transformation with a matrix. **/
    void replace(const glm::mat4 & matrix);

    /** Open a motion block of the times given, built on the current transformation. **/
    void motionBegin(const std::vector<float> & times);

    /** Whether a motion block is open. **/
    bool inMotion() const;

    /** The open motion block's times. **/
    const std::vector<float> & times() const;

    /**
     * Close the open motion block. Its requests become the current transformation, a sample at
     * each of its times. A block that held one request, or named one time, moves nothing. With no
     * block open, nothing changes.
     **/
    void motionEnd();

    /** Name the current transformation as a space. An empty name names nothing. **/
    void name(const std::string & space);

    /**
     * Make a space's transformation current, as CoordSysTransform does. `"world"` is the identity.
     *
     * @return false, changing nothing, when no request named the space
     **/
    bool use(const std::string & space);

    /**
     * The matrix that takes a space to world space, at the first time: `"object"` the current
     * transformation's, `"world"` the identity, and any other the space a request named.
     *
     * @return none when no request named the space
     **/
    std::optional<glm::mat4> toWorld(const std::string & space) const;

 private:
    Sampled current_;
    std::vector<Sampled> saved_;
    std::map<std::string, Sampled> spaces_;

    /**
     * An open motion block's times, the transformation it opened with, and each time's
     * request.
     **/
    bool motion_ { false };
    std::vector<float> times_;
    Sampled base_;
    std::vector<glm::mat4> poses_;
};

};  // namespace v3d::moya::ri::internal
