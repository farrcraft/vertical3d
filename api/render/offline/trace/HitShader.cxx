/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "HitShader.h"

#include <api/render/offline/Texture.h>
#include <api/render/offline/sl/Globals.h>
#include <api/render/offline/sl/runtime/Renderer.h>
#include <api/render/offline/sl/runtime/Value.h>

#include <string>
#include <vector>

#include <glm/matrix.hpp>

#include "Hit.h"
#include "Scene.h"
#include "Tracer.h"

namespace v3d::render::offline::trace {

namespace {

typedef v3d::render::offline::sl::runtime::Value Value;

};  // namespace

HitShader::HitShader(Tracer* tracer, const Hit & hit) :
    tracer_(tracer), hit_(hit), placement_(hit.primitive->surface().placement) {
}

bool HitShader::space(const std::string & name, glm::mat4x4* matrix) {
    // a hit's current space is world space, because the scene is held in world space
    if (name == "current" || name == "world") {
        *matrix = glm::mat4x4(1.0F);
        return true;
    }
    if (name == "shader") {
        // the placement maps shader space into world space. This matrix maps world space
        // into shader space, so it is the inverse
        *matrix = glm::inverse(placement_);
        return true;
    }
    if (tracer_->scene() == nullptr) {
        return false;
    }
    const Scene & scene = *tracer_->scene();
    if (name == "camera") {
        *matrix = scene.view();
        return true;
    }
    // the projected spaces are the camera's, which builds the ray through each pixel, so a
    // point taken to raster space lands on the pixel whose ray reached it
    if (name == "screen") {
        *matrix = scene.projection().screen * scene.view();
        return true;
    }
    if (name == "raster") {
        *matrix = scene.projection().raster * scene.view();
        return true;
    }
    if (name == "NDC") {
        *matrix = scene.projection().ndc * scene.view();
        return true;
    }
    // "object" space is the transformation in force at the primitive, and a traced primitive
    // does not store it
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

    // the shading point is in world space. While the light runs, "shader" space is the space
    // the scene instanced the light in
    const glm::mat4x4 was = placement_;
    placement_ = source.placement;
    const bool ran = held.globals.shine(source, &held.machine, Tracer::STENCIL, surface, direction, colour, reached,
        ambient);
    placement_ = was;
    return ran;
}

bool HitShader::transmission(const Value & from, const Value & to, Value* fraction) {
    if (tracer_->scene() == nullptr || fraction == nullptr) {
        return false;
    }
    // only the hit traces. Its neighbours are a pixel away and take its answer, so a shadow is
    // not traced three times
    const glm::vec3 passed = tracer_->transmitted({.origin = from.triple(0), .direction = to.triple(0) - from.triple(0)},
        hit_.geometric);
    for (unsigned int point = 0; point < fraction->width(); point++) {
        fraction->triple(point, passed);
    }
    return true;
}

bool HitShader::trace(const Value & origin, const Value & direction, Value* colour) {
    if (tracer_->scene() == nullptr || colour == nullptr) {
        return false;
    }
    const glm::vec3 seen = tracer_->traced({.origin = origin.triple(0), .direction = direction.triple(0)}, hit_.geometric);
    for (unsigned int point = 0; point < colour->width(); point++) {
        colour->triple(point, seen);
    }
    return true;
}

const v3d::render::offline::Texture* HitShader::texture(const std::string & name) {
    return tracer_->textures() == nullptr ? nullptr : tracer_->textures()->find(name);
}

v3d::render::offline::sl::runtime::Renderer::Shape HitShader::shape() const {
    Shape stencil;
    stencil.layout = Shape::Layout::STENCIL;
    stencil.du = du_;
    stencil.dv = dv_;
    return stencil;
}

void HitShader::neighbours(Hit* alongU, Hit* alongV) {
    *alongU = hit_;
    *alongV = hit_;
    du_ = 0.0F;
    dv_ = 0.0F;
    const Scene* scene = tracer_->scene();
    if (scene == nullptr) {
        return;
    }
    const float width = scene->footprint(hit_.point);
    // how far the surface moves for a small change in each parameter, which turns a pixel's
    // width into a step in that parameter
    constexpr float PROBE = 1.0e-3F;
    Hit probe;
    const glm::vec2 here(hit_.u, hit_.v);
    Hit* const neighbour[2] = { alongU, alongV };
    float* const step[2] = { &du_, &dv_ };
    for (int axis = 0; axis < 2; axis++) {
        glm::vec2 along(0.0F);
        along[axis] = 1.0F;
        if (!Scene::around(hit_, here + (PROBE * along), tracer_->poses(), &probe)) {
            continue;
        }
        const float rate = glm::length(probe.point - hit_.point) / PROBE;
        if (rate > 0.0F && Scene::around(hit_, here + ((width / rate) * along), tracer_->poses(), neighbour[axis])) {
            *step[axis] = width / rate;
        }
    }
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

    Hit alongU;
    Hit alongV;
    neighbours(&alongU, &alongV);
    const Hit* const stencil[Tracer::STENCIL] = { &hit_, &alongU, &alongV };
    for (unsigned int lane = 0; lane < Tracer::STENCIL; lane++) {
        const Hit & at = *stencil[lane];
        v3d::render::offline::sl::Point point;
        point.position = at.point;
        point.normal = at.normal;
        point.geometric = at.geometric;
        // the neighbours are seen along the hit's ray, a pixel to one side of it
        point.incident = hit_.incident;
        point.colour = at.colour;
        point.opacity = primitive.opacity();
        point.s = at.s;
        point.t = at.t;
        point.u = at.u;
        point.v = at.v;
        point.du = du_;
        point.dv = dv_;
        held.globals.surface(&held.machine, lane, point);
    }
    held.globals.eye(&held.machine, tracer_->scene() == nullptr ? glm::vec3(0.0F) : tracer_->scene()->eye());

    if (!held.machine.run()) {
        // drawn as if it had no shader, so its opacity still applies
        return primitive.opacity() * hit_.colour;
    }
    *opacity = held.globals.opacity(held.machine, 0, *opacity);
    return held.globals.colour(held.machine, 0, hit_.colour);
}

};  // namespace v3d::render::offline::trace
