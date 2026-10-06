/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "HitShader.h"

#include <string>
#include <vector>

#include "Tracer.h"

namespace v3d::render::offline::trace {

namespace {

typedef v3d::render::offline::sl::runtime::Value Value;

};  // namespace

HitShader::HitShader(Tracer* tracer, const Hit & hit) :
    tracer_(tracer), hit_(hit), placement_(hit.primitive->surface().placement) {
}

bool HitShader::space(const std::string & name, glm::mat4x4* matrix) {
    // a hit's current space is world space, because that is where the scene is
    if (name == "current" || name == "world") {
        *matrix = glm::mat4x4(1.0f);
        return true;
    }
    if (name == "shader") {
        *matrix = placement_;
        return true;
    }
    if (name == "camera" && tracer_->scene() != nullptr) {
        *matrix = tracer_->scene()->view();
        return true;
    }
    // a ray tracer has no screen or raster space: it does not project, but requests a ray
    // through a pixel from the camera, and that matrix belongs to the camera. "object" is the
    // transformation in force at the primitive, and a traced primitive does not carry it
    return false;
}

const std::vector<v3d::render::offline::sl::Placed> & HitShader::shining() const {
    static const std::vector<v3d::render::offline::sl::Placed> none;
    if (hit_.primitive->lights()) {
        return *hit_.primitive->lights();
    }
    return tracer_->scene() == nullptr ? none : tracer_->scene()->lights();
}

unsigned int HitShader::lights() {
    return static_cast<unsigned int>(shining().size());
}

bool HitShader::light(unsigned int index, const Value & surface, Value* direction,
    Value* colour, std::vector<char>* reached, bool* ambient) {
    if (index >= shining().size()) {
        return false;
    }
    const v3d::render::offline::sl::Placed & source = shining()[index];
    Tracer::Run & held = tracer_->run(source.shader);
    held.machine.renderer(this);

    // the point it is lighting is in world space, and the light's own space is where the
    // scene instanced it
    const glm::mat4x4 was = placement_;
    placement_ = source.placement;
    const bool ran = held.globals.shine(source, &held.machine, 1, surface, direction, colour, reached, ambient);
    placement_ = was;
    return ran;
}

bool HitShader::transmission(const Value & from, const Value & to, Value* fraction) {
    if (tracer_->scene() == nullptr || fraction == nullptr) {
        return false;
    }
    fraction->triple(0, tracer_->transmitted(from.triple(0), to.triple(0), hit_.geometric));
    return true;
}

bool HitShader::trace(const Value & origin, const Value & direction, Value* colour) {
    if (tracer_->scene() == nullptr || colour == nullptr) {
        return false;
    }
    colour->triple(0, tracer_->traced(origin.triple(0), direction.triple(0), hit_.geometric));
    return true;
}

const v3d::render::offline::Texture* HitShader::texture(const std::string & name) {
    return tracer_->textures() == nullptr ? nullptr : tracer_->textures()->find(name);
}

glm::vec3 HitShader::shade(glm::vec3* opacity) {
    const Primitive & primitive = *hit_.primitive;
    *opacity = primitive.opacity();
    const v3d::render::offline::sl::Placed & surface = primitive.surface();
    if (!surface.shader) {
        // a primitive built without a shader is drawn in its own colour, which is Cs at the hit
        return primitive.opacity() * hit_.colour;
    }

    Tracer::Run & held = tracer_->run(surface.shader);
    held.machine.renderer(this);
    if (!surface.shader->write(&held.machine, surface.placement)) {
        return primitive.opacity() * hit_.colour;
    }

    v3d::render::offline::sl::Point point;
    point.position = hit_.point;
    point.normal = hit_.normal;
    point.geometric = hit_.geometric;
    point.incident = hit_.incident;
    point.colour = hit_.colour;
    point.opacity = primitive.opacity();
    point.s = hit_.s;
    point.t = hit_.t;
    point.u = hit_.u;
    point.v = hit_.v;
    held.globals.surface(&held.machine, 0, point);
    held.globals.eye(&held.machine, tracer_->scene() == nullptr ? glm::vec3(0.0f) : tracer_->scene()->eye());

    if (!held.machine.run()) {
        // drawn as if it had no shader, so its opacity still applies
        return primitive.opacity() * hit_.colour;
    }
    *opacity = held.globals.opacity(held.machine, 0, *opacity);
    return held.globals.colour(held.machine, 0, hit_.colour);
}

};  // namespace v3d::render::offline::trace
