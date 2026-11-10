/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "Lights.h"

#include <api/log/Logger.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/requests/LightSource.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Placement.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include "Objects.h"
#include "Sampled.h"
#include "Warnings.h"

namespace v3d::moya::ri::internal {

namespace {

typedef v3d::render::offline::rib::ParameterList ParameterList;

/** A normalised point's radiant intensity is its emission over this. **/
constexpr float kPointShare = 4.0F;

/**
 * Makes a sphere light a point whose radiant intensity is RI's intensity, so that it lights a
 * surface with its intensity over the distance squared, as RI's lights do.
 **/
void asPoint(scene::Scene * scene, const scene::Handle & made, const ParameterList & parameters) {
    scene->set(made, "radius", scene::Value::number(0.0F));
    scene->set(made, "normalize", scene::Value::integer(1));
    scene->set(made, "intensity", scene::Value::number(kPointShare * parameters.number("intensity", 1.0F)));
}

};  // namespace

Lights::Lights(scene::Scene * scene, Objects * objects, Warnings * warnings, const boost::shared_ptr<v3d::log::Logger> & logger) :
    scene_(scene), objects_(objects), warnings_(warnings), logger_(logger) {
}

glm::vec3 Lights::pointOf(const ParameterList & parameters, const std::string & name, const glm::vec3 & fallback) {
    const std::vector<glm::vec3> found = parameters.points(name);
    return found.empty() ? fallback : found.front();
}

void Lights::clear() {
    lights_.clear();
    unmade_.clear();
    instances_ = 0;
}

scene::Handle Lights::light(const std::string & kind, const std::string & name, const ParameterList & parameters, const glm::vec3 & defaultTo,
    const Sampled & transformation) {
    const scene::Handle made = objects_->make(kind, name);
    const glm::mat4 placed = scene::aimed(pointOf(parameters, "from", glm::vec3(0.0F)), pointOf(parameters, "to", defaultTo));
    Sampled aimed = transformation;
    for (glm::mat4 & matrix : aimed.matrices) {
        matrix = matrix * placed;
    }
    aimed.set(scene_, made, "transform", logger_);
    scene_->set(made, "colour", scene::Value::colour(pointOf(parameters, "lightcolor", glm::vec3(1.0F))));
    scene_->set(made, "intensity", scene::Value::number(parameters.number("intensity", 1.0F)));
    return made;
}

bool Lights::source(const v3d::render::offline::rib::requests::LightSource & request, const ParameterList & parameters, const std::string & name,
    const Sampled & transformation) {
    scene::Handle made;
    if (request.shader == "pointlight") {
        // a point light has no direction, so it looks down z from where it is
        const glm::vec3 from = pointOf(parameters, "from", glm::vec3(0.0F));
        made = light("sphereLight", objects_->unique("sphereLight", name), parameters, from + glm::vec3(0.0F, 0.0F, 1.0F), transformation);
        asPoint(scene_, made, parameters);
    } else if (request.shader == "distantlight") {
        made = light("distantLight", objects_->unique("distantLight", name), parameters, glm::vec3(0.0F, 0.0F, 1.0F), transformation);
        scene_->set(made, "angle", scene::Value::number(0.0F));
    } else if (request.shader == "spotlight") {
        made = light("sphereLight", objects_->unique("sphereLight", name), parameters, glm::vec3(0.0F, 0.0F, 1.0F), transformation);
        asPoint(scene_, made, parameters);
        const float cone = parameters.number("coneangle", kConeAngle);
        const float delta = parameters.number("conedeltaangle", kConeDelta);
        scene_->set(made, "coneAngle", scene::Value::number(glm::degrees(cone)));
        scene_->set(made, "coneSoftness", scene::Value::number(cone > 0.0F ? std::clamp(delta / cone, 0.0F, 1.0F) : 0.0F));
    } else {
        warnings_->once("light " + request.shader, "moya makes no light of the shader \"" + request.shader + "\"");
        unmade_.insert(request.handle);
        return false;
    }
    lights_[request.handle] = Light { .light = made, .lit = {} };
    return true;
}

bool Lights::named(const std::string & handle) {
    if (unmade_.contains(handle)) {
        return false;
    }
    if (!lights_.contains(handle)) {
        warnings_->once("illuminate " + handle, "Illuminate names the light " + handle + ", which the world has not made");
        return false;
    }
    return true;
}

void Lights::lit(const scene::Handle & instance, const std::set<std::string> & on) {
    ++instances_;
    for (const std::string & handle : on) {
        const auto found = lights_.find(handle);
        if (found != lights_.end()) {
            found->second.lit.push_back(instance);
        }
    }
}

void Lights::glow(const scene::Handle & geometry, const std::string & name, const ParameterList & parameters, const Sampled & transformation) {
    const scene::Handle made = objects_->make("meshLight", objects_->unique("meshLight", name));
    scene_->set(made, "geometry", scene::Value::handle(geometry));
    transformation.set(scene_, made, "transform", logger_);
    scene_->set(made, "colour", scene::Value::colour(pointOf(parameters, "lightcolor", glm::vec3(1.0F))));
    scene_->set(made, "intensity", scene::Value::number(parameters.number("intensity", 1.0F)));
}

void Lights::finish() {
    for (const auto & [handle, made] : lights_) {
        if (made.lit.size() != instances_) {
            scene_->set(made.light, "includeRoot", scene::Value::integer(0));
            scene_->set(made.light, "instances", scene::Value::handles(made.lit));
        }
    }
}

};  // namespace v3d::moya::ri::internal
