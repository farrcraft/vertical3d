/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

namespace v3d::moya::colour {

namespace internal {
class Processor;
};  // namespace internal

/**
 * A conversion between colour spaces that an OpenColorIO configuration gives. It is safe to apply
 * from any number of threads at once.
 **/
class Transform final {
 public:
    explicit Transform(boost::shared_ptr<const internal::Processor> processor);

    /** One colour, converted. **/
    glm::vec3 apply(const glm::vec3 & rgb) const;

    /**
     * Converts the first three floats of each of a run of pixels in place, leaving the rest.
     *
     * @param values the first pixel's first float
     * @param pixels how many pixels
     * @param stride the floats from one pixel's start to the next's, at least 3
     **/
    void apply(float * values, std::size_t pixels, std::size_t stride) const;

 private:
    boost::shared_ptr<const internal::Processor> processor_;
};

};  // namespace v3d::moya::colour
