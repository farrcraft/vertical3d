/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/camera/Camera.h>

#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::camera {

namespace {

/** A parameter of the object, or its kind's fallback for a handle that names nothing. **/
const scene::Value & parameter(const scene::Scene & scene, const std::string & kind, const scene::Handle & object, const std::string & name) {
    if (object.valid() && scene.contains(object)) {
        return scene.value(object, name);
    }
    const auto index = scene.schema().find(kind);
    if (!index) {
        throw std::invalid_argument("the scene's schema has no kind " + kind);
    }
    for (const scene::Declaration & declaration : scene.schema().kind(*index).parameters) {
        if (declaration.name == name) {
            return declaration.fallback;
        }
    }
    throw std::invalid_argument("a " + kind + " has no parameter " + name);
}

float number(const scene::Value & value) {
    return value.floatArray().front();
}

};  // namespace

kernel::camera::Camera fromScene(const scene::Scene & scene) {
    const std::vector<scene::Handle> options = scene.objects("options");
    const scene::Handle settings = options.empty() ? scene::Handle {} : options.front();
    const auto read = [&](const std::string & name) -> const scene::Value & {
        return parameter(scene, "options", settings, name);
    };
    const scene::Handle handle = read("camera").handleArray().front();
    const auto look = [&](const std::string & name) -> const scene::Value & {
        return parameter(scene, "camera", handle, name);
    };

    kernel::camera::Camera out;
    const std::vector<std::int32_t> & resolution = read("resolution").integerArray();
    out.resolution = glm::vec2 { static_cast<float>(resolution[0]), static_cast<float>(resolution[1]) };

    float aspect = number(read("frameAspect"));
    if (aspect <= 0.0F) {
        aspect = (out.resolution.x * number(read("pixelAspect"))) / out.resolution.y;
    }
    const std::vector<float> & screen = look("screen").floatArray();
    if (screen.size() == 4) {
        out.screen = glm::vec4 { screen[0], screen[1], screen[2], screen[3] };
    } else if (aspect >= 1.0F) {
        out.screen = glm::vec4 { -aspect, aspect, -1.0F, 1.0F };
    } else {
        out.screen = glm::vec4 { -1.0F, 1.0F, -1.0F / aspect, 1.0F / aspect };
    }

    out.toWorld = look("transform").matrixAt(0);
    out.perspective = look("projection").stringArray().front() == "perspective";
    out.tanHalfFov = std::tan(glm::radians(number(look("fov"))) * 0.5F);
    out.near = number(look("near"));
    out.far = number(look("far"));

    const float fstop = number(look("fstop"));
    const float focalLength = number(look("focalLength"));
    const float focalDistance = number(look("focalDistance"));
    if (std::isfinite(fstop) && fstop > 0.0F && focalLength > 0.0F && focalDistance > 0.0F) {
        out.lensRadius = focalLength / (2.0F * fstop);
        out.focalDistance = focalDistance;
    }
    const std::vector<float> & shutter = look("shutter").floatArray();
    out.open = shutter[0];
    out.close = shutter[1];
    return out;
}

};  // namespace v3d::moya::camera
