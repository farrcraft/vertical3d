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
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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

/** The UsdLux light each of RenderMan's analytic lights becomes. **/
const std::map<std::string, std::string> & renderManKinds() {
    static const std::map<std::string, std::string> kinds = {
        { "PxrRectLight", "rectLight" },
        { "PxrDiskLight", "diskLight" },
        { "PxrSphereLight", "sphereLight" },
        { "PxrCylinderLight", "cylinderLight" },
        { "PxrDistantLight", "distantLight" },
        { "PxrDomeLight", "domeLight" } };
    return kinds;
}

/**
 * Turns a RenderMan light's space onto UsdLux's. A RenderMan light faces +z where UsdLux's faces
 * -z, and lays a rect's texture the other way round, so it is mirrored through its origin.
 * RenderMan's dome has its pole on +z, where UsdLux's has it on +y.
 **/
glm::mat4 toUsdLux(const std::string & kind) {
    if (kind == "domeLight") {
        return glm::mat4(glm::vec4(0.0F, -1.0F, 0.0F, 0.0F), glm::vec4(0.0F, 0.0F, 1.0F, 0.0F), glm::vec4(-1.0F, 0.0F, 0.0F, 0.0F),
            glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    }
    return glm::scale(glm::mat4(1.0F), glm::vec3(-1.0F));
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
    grouped(made, parameters);
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
    } else if (request.shader == "ambientlight") {
        // RI's ambient light lights every point alike: a dome of its colour that shadows nothing
        // and the camera does not see
        made = light("domeLight", objects_->unique("domeLight", name), parameters, glm::vec3(0.0F, 0.0F, 1.0F), transformation);
        scene_->set(made, "shadowIncludeRoot", scene::Value::integer(0));
        scene_->set(made, "camera", scene::Value::integer(0));
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

void Lights::grouped(const scene::Handle & made, const ParameterList & parameters) {
    if (parameters.has("lightGroup")) {
        scene_->set(made, "lightGroup", scene::Value::string(parameters.string("lightGroup")));
    }
}

void Lights::rename(const scene::Handle & made, const std::string & kind, const ParameterList & parameters) {
    // what every RenderMan light holds
    static constexpr Renamed kEvery[] = { { .from = "intensity", .to = "intensity", .integer = false },
        { .from = "exposure", .to = "exposure", .integer = false }, { .from = "enableTemperature", .to = "enableColorTemperature", .integer = true },
        { .from = "temperature", .to = "colorTemperature", .integer = false } };
    // what a cone and an IES profile shape a light with a shape by
    static constexpr Renamed kShaping[] = { { .from = "coneAngle", .to = "coneAngle", .integer = false },
        { .from = "coneSoftness", .to = "coneSoftness", .integer = false }, { .from = "iesProfileScale", .to = "iesAngleScale", .integer = false },
        { .from = "iesProfileNormalize", .to = "iesNormalize", .integer = true } };
    const auto copy = [&](const Renamed & each) {
        if (!parameters.has(each.from)) {
            return;
        }
        const float value = parameters.number(each.from, 0.0F);
        scene_->set(made, each.to, each.integer ? scene::Value::integer(value != 0.0F ? 1 : 0) : scene::Value::number(value));
    };
    for (const Renamed & each : kEvery) {
        copy(each);
    }
    grouped(made, parameters);
    if (parameters.has("lightColor")) {
        scene_->set(made, "colour", scene::Value::colour(pointOf(parameters, "lightColor", glm::vec3(1.0F))));
    }
    // UsdLux never normalises a dome
    if (kind != "domeLight") {
        copy(Renamed { .from = "areaNormalize", .to = "normalize", .integer = true });
    }
    if (kind == "domeLight" || kind == "distantLight" || kind == "meshLight") {
        return;
    }
    for (const Renamed & each : kShaping) {
        copy(each);
    }
    if (parameters.has("iesProfile")) {
        scene_->set(made, "iesFile", scene::Value::string(parameters.string("iesProfile")));
    }
}

bool Lights::renderman(const v3d::render::offline::rib::requests::LightSource & request, const ParameterList & parameters, const std::string & name,
    const Sampled & transformation) {
    const auto found = renderManKinds().find(request.shader);
    if (found == renderManKinds().end()) {
        warnings_->once("light " + request.shader, "moya makes no light of the shader \"" + request.shader + "\"");
        unmade_.insert(request.handle);
        return false;
    }
    const std::string & kind = found->second;
    const scene::Handle made = objects_->make(kind, objects_->unique(kind, name));
    const glm::mat4 turned = toUsdLux(kind);
    Sampled placed = transformation;
    for (glm::mat4 & matrix : placed.matrices) {
        matrix = matrix * turned;
    }
    placed.set(scene_, made, "transform", logger_);
    if (kind == "distantLight") {
        scene_->set(made, "intensity", scene::Value::number(kDistantIntensity));
        if (parameters.has("angleExtent")) {
            scene_->set(made, "angle", scene::Value::number(parameters.number("angleExtent", 0.0F)));
        }
    }
    rename(made, kind, parameters);
    if ((kind == "rectLight" || kind == "domeLight") && parameters.has("lightColorMap")) {
        scene_->set(made, "texture", scene::Value::string(parameters.string("lightColorMap")));
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

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the shader, then the light's name
void Lights::glow(const scene::Handle & geometry, const std::string & shader, const std::string & name, const ParameterList & parameters,
    const Sampled & transformation) {
    const scene::Handle made = objects_->make("meshLight", objects_->unique("meshLight", name));
    scene_->set(made, "geometry", scene::Value::handle(geometry));
    transformation.set(scene_, made, "transform", logger_);
    if (shader == "PxrMeshLight") {
        rename(made, "meshLight", parameters);
        // a constant textureColor tints the light's colour
        if (parameters.has("textureColor")) {
            const glm::vec3 colour = pointOf(parameters, "lightColor", glm::vec3(1.0F)) * pointOf(parameters, "textureColor", glm::vec3(1.0F));
            scene_->set(made, "colour", scene::Value::colour(colour));
        }
        return;
    }
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
