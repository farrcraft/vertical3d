/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Camera.h"

#include <string>

#include <glm/matrix.hpp>

namespace v3d::render::offline {

glm::vec3 Camera::eye() const {
    return glm::vec3(glm::inverse(view)[3]);
}

bool Camera::fromWorld(const std::string & name, glm::mat4x4* matrix) const {
    if (name == "world") {
        *matrix = glm::mat4x4(1.0F);
        return true;
    }
    if (name == "camera") {
        *matrix = view;
        return true;
    }
    if (name == "screen") {
        *matrix = screen * view;
        return true;
    }
    if (name == "raster") {
        *matrix = raster * view;
        return true;
    }
    if (name == "NDC") {
        *matrix = ndc * view;
        return true;
    }
    return false;
}

bool Camera::fromCamera(const std::string & name, glm::mat4x4* matrix) const {
    if (name == "world") {
        *matrix = glm::inverse(view);
        return true;
    }
    if (name == "camera") {
        *matrix = glm::mat4x4(1.0F);
        return true;
    }
    if (name == "screen") {
        *matrix = screen;
        return true;
    }
    if (name == "raster") {
        *matrix = raster;
        return true;
    }
    if (name == "NDC") {
        *matrix = ndc;
        return true;
    }
    return false;
}

};  // namespace v3d::render::offline
