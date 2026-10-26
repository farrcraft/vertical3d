/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "HitShader.h"

#include <api/render/offline/Texture.h>
#include <api/render/offline/sl/Globals.h>
#include <api/render/offline/sl/Placed.h>
#include <api/render/offline/sl/runtime/Renderer.h>
#include <api/render/offline/sl/runtime/Value.h>
#include <api/type/geometry/Line.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

#include "Hit.h"
#include "Scene.h"
#include "Tracer.h"

namespace v3d::render::offline::trace {

namespace {

typedef v3d::render::offline::sl::runtime::Value Value;

};  // namespace

HitShader::HitShader(Tracer* tracer, const std::vector<const Hit*> & hits, const std::vector<float> & times,
    const std::vector<const Scene::Poses*> & poses) :
    tracer_(tracer), hits_(hits), times_(times), poses_(poses), placement_(hits.front()->primitive->surface().placement) {
}

HitShader::HitShader(Tracer* tracer, const Hit & hit) :
    HitShader(tracer, std::vector<const Hit*>{ &hit }, std::vector<float>{ tracer->time() }, std::vector<const Scene::Poses*>{ &tracer->poses() }) {
}

unsigned int HitShader::lanes() const {
    return lanes_;
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
    if (name == "object") {
        // every hit of the batch shares its object space
        *matrix = glm::inverse(hits_.front()->primitive->object());
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
    return false;
}

const std::vector<v3d::render::offline::sl::Placed> & HitShader::shining() const {
    static const std::vector<v3d::render::offline::sl::Placed> none;
    // every hit of the batch shares its lights
    const Primitive & primitive = *hits_.front()->primitive;
    if (primitive.lights()) {
        return *primitive.lights();
    }
    return tracer_->scene() == nullptr ? none : tracer_->scene()->lights();
}

bool HitShader::differentiates() const {
    const v3d::render::offline::sl::Placed & surface = hits_.front()->primitive->surface();
    if (surface.shader && surface.shader->program().differentiates) {
        return true;
    }
    return std::ranges::any_of(shining(), [](const v3d::render::offline::sl::Placed & light) {
        return light.shader && light.shader->program().differentiates;
    });
}

unsigned int HitShader::lights() {
    return v3d::render::offline::sl::shining(shining());
}

bool HitShader::light(unsigned int index, const Value & surface, const v3d::render::offline::sl::runtime::Cone & gathering,
    Value* direction,
    Value* colour, std::vector<char>* reached, bool* ambient) {
    unsigned int sample = 0;
    const v3d::render::offline::sl::Placed* lit = v3d::render::offline::sl::which(shining(), index, &sample);
    if (lit == nullptr) {
        return false;
    }
    const v3d::render::offline::sl::Placed & source = *lit;
    const unsigned int width = static_cast<unsigned int>(hits_.size()) * lanes_;
    Tracer::Run & held = tracer_->run(source.shader, width);
    held.machine.renderer(this);
    // a hit's current space is world space, where an area light's samples are taken
    std::vector<glm::vec3> from;
    if (source.area) {
        for (unsigned int lane = 0; lane < width; lane++) {
            from.push_back(source.area->point(surface.triple(lane), sample));
        }
    }

    // the shading point is in world space. While the light runs, "shader" space is the space
    // the scene instanced the light in
    const glm::mat4x4 was = placement_;
    placement_ = source.placement;
    const bool ran = held.globals.shine(source, &held.machine, width, surface, gathering, direction, colour,
        reached, ambient, source.area ? &from : nullptr);
    placement_ = was;
    // the illuminance body that runs next for this light reads its output parameters
    lightMachine_ = ran ? &held.machine : nullptr;
    if (ran && source.area) {
        // each sample carries its share of the light
        for (unsigned int lane = 0; lane < width; lane++) {
            colour->triple(lane, colour->triple(lane) / static_cast<float>(source.area->samples()));
        }
    }
    return ran;
}

bool HitShader::transmission(const Value & from, const Value & to, Value* fraction) {
    if (tracer_->scene() == nullptr || fraction == nullptr) {
        return false;
    }
    // only a hit traces. Its neighbours are a pixel away and take its answer, so a shadow is
    // not traced three times
    for (std::size_t hit = 0; hit < hits_.size(); hit++) {
        const unsigned int first = static_cast<unsigned int>(hit) * lanes_;
        const glm::vec3 passed = tracer_->transmitted(
            {.origin = from.triple(first), .direction = to.triple(first) - from.triple(first)},
            hits_[hit]->geometric, *poses_[hit]);
        for (unsigned int lane = first; lane < first + lanes_ && lane < fraction->width(); lane++) {
            fraction->triple(lane, passed);
        }
    }
    return true;
}

bool HitShader::trace(const Value & origin, const Value & direction, Value* colour) {
    if (tracer_->scene() == nullptr || colour == nullptr) {
        return false;
    }
    // one ray for each hit, and the rays of the batch are traced as a batch
    std::vector<v3d::type::geometry::Line> paths;
    std::vector<glm::vec3> geometric;
    paths.reserve(hits_.size());
    geometric.reserve(hits_.size());
    for (std::size_t hit = 0; hit < hits_.size(); hit++) {
        const unsigned int first = static_cast<unsigned int>(hit) * lanes_;
        paths.push_back({.origin = origin.triple(first), .direction = direction.triple(first)});
        geometric.push_back(hits_[hit]->geometric);
    }
    const std::vector<glm::vec3> seen = tracer_->traced(paths, geometric, times_);
    for (std::size_t hit = 0; hit < hits_.size(); hit++) {
        const unsigned int first = static_cast<unsigned int>(hit) * lanes_;
        for (unsigned int lane = first; lane < first + lanes_ && lane < colour->width(); lane++) {
            colour->triple(lane, seen[hit]);
        }
    }
    return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the signature is Renderer::message
bool HitShader::message(const std::string & shader, const std::string & name, Value* value) {
    if (shader == "surface" && surfaceMachine_ != nullptr) {
        return surfaceMachine_->output(name, value);
    }
    if (shader == "lightsource" && lightMachine_ != nullptr) {
        return lightMachine_->output(name, value);
    }
    return false;
}

const v3d::render::offline::Texture* HitShader::texture(const std::string & name) {
    return tracer_->textures() == nullptr ? nullptr : tracer_->textures()->find(name);
}

v3d::render::offline::sl::runtime::Renderer::Shape HitShader::shape() const {
    Shape shaped;
    if (lanes_ != Tracer::STENCIL) {
        return shaped;
    }
    shaped.layout = Shape::Layout::STENCIL;
    shaped.along = static_cast<unsigned int>(hits_.size());
    shaped.steps = &steps_;
    return shaped;
}

glm::vec2 HitShader::neighbours(const Hit & hit, const Scene::Poses & poses, Hit* alongU, Hit* alongV) const {
    *alongU = hit;
    *alongV = hit;
    glm::vec2 steps(0.0F);
    const Scene* scene = tracer_->scene();
    if (scene == nullptr) {
        return steps;
    }
    const float width = scene->footprint(hit.point);
    // how far the surface moves for a small change in each parameter, which turns a pixel's
    // width into a step in that parameter
    constexpr float PROBE = 1.0e-3F;
    Hit probe;
    const glm::vec2 here(hit.u, hit.v);
    Hit* const neighbour[2] = { alongU, alongV };
    for (int axis = 0; axis < 2; axis++) {
        glm::vec2 along(0.0F);
        along[axis] = 1.0F;
        if (!Scene::around(hit, here + (PROBE * along), poses, &probe)) {
            continue;
        }
        const float rate = glm::length(probe.point - hit.point) / PROBE;
        if (rate > 0.0F && Scene::around(hit, here + ((width / rate) * along), poses, neighbour[axis])) {
            steps[axis] = width / rate;
        }
    }
    return steps;
}

void HitShader::shade(std::vector<glm::vec3>* colours, std::vector<glm::vec3>* opacities) {
    const std::size_t count = hits_.size();
    colours->resize(count);
    opacities->resize(count);
    // what a hit is drawn as when it has no shader or its shader fails: its primitive's colour
    // times its opacity, and its opacity
    const auto flat = [this, colours, opacities]() {
        for (std::size_t hit = 0; hit < hits_.size(); hit++) {
            const Primitive & primitive = *hits_[hit]->primitive;
            (*opacities)[hit] = primitive.opacity();
            (*colours)[hit] = primitive.opacity() * hits_[hit]->colour;
        }
    };
    const v3d::render::offline::sl::Placed & surface = hits_.front()->primitive->surface();
    if (!surface.shader) {
        flat();
        return;
    }

    lanes_ = differentiates() ? Tracer::STENCIL : 1U;
    const unsigned int width = static_cast<unsigned int>(count) * lanes_;
    Tracer::Run & held = tracer_->run(surface.shader, width);
    held.machine.renderer(this);
    if (!surface.shader->write(&held.machine, surface.placement)) {
        flat();
        return;
    }

    steps_.assign(count, glm::vec2(0.0F));
    for (std::size_t hit = 0; hit < count; hit++) {
        const Hit & centre = *hits_[hit];
        Hit alongU;
        Hit alongV;
        if (lanes_ == Tracer::STENCIL) {
            steps_[hit] = neighbours(centre, *poses_[hit], &alongU, &alongV);
        }
        const Hit* const stencil[Tracer::STENCIL] = { &centre, &alongU, &alongV };
        for (unsigned int lane = 0; lane < lanes_; lane++) {
            const Hit & at = *stencil[lane];
            v3d::render::offline::sl::Point point;
            point.position = at.point;
            point.normal = at.normal;
            point.geometric = at.geometric;
            // the neighbours are seen along the hit's ray, a pixel to one side of it
            point.incident = centre.incident;
            point.colour = at.colour;
            point.opacity = centre.primitive->opacity();
            point.s = at.s;
            point.t = at.t;
            point.u = at.u;
            point.v = at.v;
            point.du = steps_[hit].x;
            point.dv = steps_[hit].y;
            held.globals.surface(&held.machine, (static_cast<unsigned int>(hit) * lanes_) + lane, point);
        }
    }
    held.globals.eye(&held.machine, tracer_->scene() == nullptr ? glm::vec3(0.0F) : tracer_->scene()->eye());

    surfaceMachine_ = &held.machine;
    lightMachine_ = nullptr;
    const bool ran = held.machine.run();
    surfaceMachine_ = nullptr;
    lightMachine_ = nullptr;
    if (!ran) {
        // drawn as if it had no shader, so its opacity still applies
        flat();
        return;
    }
    for (std::size_t hit = 0; hit < count; hit++) {
        const unsigned int first = static_cast<unsigned int>(hit) * lanes_;
        (*opacities)[hit] = held.globals.opacity(held.machine, first, hits_[hit]->primitive->opacity());
        (*colours)[hit] = held.globals.colour(held.machine, first, hits_[hit]->colour);
    }
}

glm::vec3 HitShader::shade(glm::vec3* opacity) {
    std::vector<glm::vec3> colours;
    std::vector<glm::vec3> opacities;
    shade(&colours, &opacities);
    *opacity = opacities.front();
    return colours.front();
}

};  // namespace v3d::render::offline::trace
