/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::moya::ri::internal {

/** RI's current transformation: one matrix, or a matrix at each of a motion block's times. **/
struct Sampled {
    std::vector<float> times{};
    std::vector<glm::mat4> matrices { glm::mat4(1.0F) };

    /**
     * Set an object's parameter to these matrices: the one matrix, or a sample at each time.
     * Times that do not rise are logged, and the first matrix is set alone.
     **/
    void set(scene::Scene * scene, const scene::Handle & object, const std::string & parameter, const boost::shared_ptr<v3d::log::Logger> & logger) const;
};

};  // namespace v3d::moya::ri::internal
