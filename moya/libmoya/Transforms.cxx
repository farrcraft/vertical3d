/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Transforms.h"

#include <api/render/offline/Camera.h>
#include <api/render/offline/MovingTransform.h>

#include <map>
#include <string>

#include <glm/mat4x4.hpp>

namespace v3d::moya {

v3d::render::offline::MovingTransform & Transforms::current() {
    return current_;
}

const v3d::render::offline::MovingTransform & Transforms::current() const {
    return current_;
}

void Transforms::push() {
    saved_.push_back(current_);
}

void Transforms::pop() {
    if (saved_.empty()) {
        return;
    }
    current_ = saved_.back();
    saved_.pop_back();
}

bool Transforms::predefined(const std::string & name) {
    return name == "object" || name == "world" || name == "camera" || name == "screen" || name == "raster" ||
        name == "NDC";
}

bool Transforms::save(const std::string & name) {
    if (predefined(name)) {
        return false;
    }
    named_[name] = current_.reference();
    return true;
}

const std::map<std::string, glm::mat4x4> & Transforms::named() const {
    return named_;
}

bool Transforms::known(const std::string & name) const {
    return predefined(name) || named_.find(name) != named_.end();
}

bool Transforms::space(const std::string & name, const v3d::render::offline::Camera & camera, glm::mat4x4* matrix) const {
    if (name == "camera") {
        *matrix = camera.view;
        return true;
    }
    if (name == "screen") {
        *matrix = camera.screen;
        return true;
    }
    if (name == "raster") {
        *matrix = camera.raster;
        return true;
    }
    if (name == "NDC") {
        *matrix = camera.ndc;
        return true;
    }
    if (predefined(name)) {
        *matrix = glm::mat4x4(1.0F);
        return true;
    }
    const auto found = named_.find(name);
    if (found == named_.end()) {
        return false;
    }
    *matrix = found->second;
    return true;
}

};  // namespace v3d::moya
