/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "HitShader.h"

#include <string>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

namespace v3d::talyn {

namespace {

typedef v3d::render::offline::sl::runtime::Machine Machine;
typedef v3d::render::offline::sl::runtime::Value Value;
typedef v3d::render::offline::sl::runtime::Program Program;

/**
 * How far along the geometric normal a ray leaving a surface starts.
 *
 * **This is the trap of the step.** A shadow ray that starts exactly on the surface hits
 * the surface it left, every lit pixel comes out black, and the pattern it makes looks
 * like a normal fault rather than like a numerical one. The offset is along the normal
 * rather than along the ray, because a ray running nearly parallel to the surface is
 * exactly the case where an offset along it stays on the surface.
 **/
const float EPSILON = 1.0e-4f;

void put(Machine* machine, int reg, const glm::vec3 & value) {
    if (reg >= 0) {
        machine->value(reg).triple(0, value);
    }
}

void put(Machine* machine, int reg, float value) {
    if (reg >= 0) {
        machine->value(reg).number(0, value);
    }
}

};  // namespace

HitShader::HitShader(const Scene* scene) : scene_(scene) {
}

HitShader::Run & HitShader::run(const v3d::render::offline::sl::InstancePtr & shader) {
    Run & held = runs_[&shader->program()];
    if (held.program == &shader->program()) {
        return held;
    }
    held.program = &shader->program();
    held.machine.renderer(this);
    // a batch of one, which is the whole point: talyn's hit is not a special case of the
    // model, it is a batch one wide
    held.machine.prepare(shader->program(), 1);
    return held;
}

bool HitShader::space(const std::string & name, glm::mat4x4* matrix) {
    // talyn works in world space, because that is where its scene is. moya works in
    // camera space, and the two answering "current" differently is why this is a callback
    if (name == "current" || name == "world" || name == "object") {
        *matrix = glm::mat4x4(1.0f);
        return true;
    }
    if (name == "shader") {
        *matrix = placement_;
        return true;
    }
    if (name == "camera" && scene_ != nullptr) {
        *matrix = scene_->camera().view();
        return true;
    }
    // a raytracer has no screen or raster space to speak of: it does not project, it
    // asks a camera for a ray through a pixel and the matrix that would do it is the
    // camera's own business
    return false;
}

unsigned int HitShader::lights() {
    return scene_ == nullptr ? 0u : static_cast<unsigned int>(scene_->lights().size());
}

bool HitShader::light(unsigned int index, const Value & surface, Value* direction,
    Value* colour, std::vector<char>* reached, bool* ambient) {
    if (scene_ == nullptr || index >= scene_->lights().size()) {
        return false;
    }
    const v3d::render::offline::sl::Placed & shining = scene_->lights()[index];
    Run & held = run(shining.shader);
    const Program & program = shining.shader->program();

    // a light's parameters are stated in the space the scene instanced it in, and the
    // point it is lighting is in world space
    const glm::mat4x4 was = placement_;
    placement_ = shining.placement;
    shining.shader->write(&held.machine, shining.placement);

    put(&held.machine, program.symbol("Ps"), surface.triple(0));
    put(&held.machine, program.symbol("P"),
        v3d::render::offline::sl::ptransform(shining.placement, glm::vec3(0.0f)));
    const bool ran = held.machine.run(program);
    placement_ = was;
    if (!ran) {
        return false;
    }

    const int away = program.symbol("L");
    const int tint = program.symbol("Cl");
    direction->triple(0, away < 0 ? glm::vec3(0.0f) : held.machine.value(away).triple(0));
    colour->triple(0, tint < 0 ? glm::vec3(0.0f) : held.machine.value(tint).triple(0));
    *reached = held.machine.lit();
    *ambient = shining.shader->ambient();
    return true;
}

bool HitShader::transmission(const Value & from, const Value & to, Value* fraction) {
    if (scene_ == nullptr || fraction == nullptr) {
        return false;
    }
    const glm::vec3 source = from.triple(0);
    const glm::vec3 target = to.triple(0);
    const glm::vec3 along = target - source;
    const float span = glm::length(along);
    if (span <= 0.0f) {
        fraction->triple(0, glm::vec3(1.0f));
        return true;
    }

    /*
        The ray leaves the surface it is shading, so it starts off it: exactly on it, it
        meets it. The offset is along the geometric normal and toward the light, which is
        the side the light is on - a ray leaving the back of a surface would otherwise
        start inside it.
    */
    glm::vec3 origin = source;
    if (hit_ != nullptr) {
        const float side = glm::dot(hit_->geometric, along) < 0.0f ? -1.0f : 1.0f;
        origin += hit_->geometric * (side * EPSILON);
    }
    const v3d::type::geometry::Ray ray(origin, along / span);

    Hit blocker;
    // anything between here and the light blocks it, and nothing beyond the light does.
    // Opaque only: Os on an occluder is a shading question and this is a visibility one
    const bool blocked = scene_->nearest(ray, 0.0f, &blocker) && blocker.distance < span;
    fraction->triple(0, blocked ? glm::vec3(0.0f) : glm::vec3(1.0f));
    return true;
}

bool HitShader::trace(const Value & origin, const Value & direction, Value* colour) {
    if (scene_ == nullptr || colour == nullptr) {
        return false;
    }
    if (depth_ > 0) {
        // a ray a traced ray traced answers the background, which is what bounds the
        // recursion. A depth a scene can set is phase 5's, with the shaders that use it
        colour->triple(0, scene_->background());
        return true;
    }
    const glm::vec3 along = direction.triple(0);
    const float span = glm::length(along);
    if (span <= 0.0f) {
        colour->triple(0, scene_->background());
        return true;
    }
    glm::vec3 start = origin.triple(0);
    if (hit_ != nullptr) {
        const float side = glm::dot(hit_->geometric, along) < 0.0f ? -1.0f : 1.0f;
        start += hit_->geometric * (side * EPSILON);
    }

    Hit found;
    if (!scene_->nearest(v3d::type::geometry::Ray(start, along / span), 0.0f, &found)) {
        colour->triple(0, scene_->background());
        return true;
    }
    const Hit* was = hit_;
    depth_++;
    colour->triple(0, shade(found));
    depth_--;
    hit_ = was;
    return true;
}

glm::vec3 HitShader::shade(const Hit & hit) {
    if (hit.triangle == nullptr) {
        return scene_ == nullptr ? glm::vec3(0.0f) : scene_->background();
    }
    const v3d::render::offline::sl::Placed & surface = hit.triangle->surface();
    if (!surface.shader) {
        // a triangle a scene built without a shader is its own colour, which is the
        // picture this renderer drew before there was a language to ask for another
        return hit.triangle->colour();
    }

    hit_ = &hit;
    placement_ = surface.placement;
    Run & held = run(surface.shader);
    const Program & program = surface.shader->program();
    surface.shader->write(&held.machine, surface.placement);

    put(&held.machine, program.symbol("P"), hit.point);
    put(&held.machine, program.symbol("N"), hit.normal);
    put(&held.machine, program.symbol("Ng"), hit.geometric);
    put(&held.machine, program.symbol("I"), hit.incident);
    put(&held.machine, program.symbol("E"),
        scene_ == nullptr ? glm::vec3(0.0f) : scene_->camera().profile().eye());
    put(&held.machine, program.symbol("Cs"), hit.triangle->colour());
    put(&held.machine, program.symbol("Os"), hit.triangle->opacity());
    // the barycentric weights stand in for the surface parameters until there is a real
    // parameterisation to read them off
    put(&held.machine, program.symbol("s"), hit.u);
    put(&held.machine, program.symbol("t"), hit.v);
    put(&held.machine, program.symbol("u"), hit.u);
    put(&held.machine, program.symbol("v"), hit.v);

    if (!held.machine.run(program)) {
        hit_ = nullptr;
        return hit.triangle->colour();
    }
    const int result = program.symbol("Ci");
    const glm::vec3 colour = result < 0 ? hit.triangle->colour() :
        held.machine.value(result).triple(0);
    hit_ = nullptr;
    return colour;
}

};  // namespace v3d::talyn
