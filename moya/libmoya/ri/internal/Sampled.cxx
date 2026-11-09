/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Sampled.h"

#include <api/log/Logger.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <stdexcept>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::moya::ri::internal {

void Sampled::set(scene::Scene * scene, const scene::Handle & object, const std::string & parameter, const boost::shared_ptr<v3d::log::Logger> & logger) const {
    if (times.empty()) {
        scene->set(object, parameter, scene::Value::matrix(matrices.front()));
        return;
    }
    std::vector<scene::Value> values;
    values.reserve(matrices.size());
    for (const glm::mat4 & matrix : matrices) {
        values.push_back(scene::Value::matrix(matrix));
    }
    try {
        scene->set(object, parameter, times, values);
    } catch (const std::invalid_argument & error) {
        // a motion block whose times do not rise keeps its first time alone
        logger->get()->warn("a motion block's times are not in order, so it holds its first time alone - {}", error.what());
        scene->set(object, parameter, values.front());
    }
}

};  // namespace v3d::moya::ri::internal
