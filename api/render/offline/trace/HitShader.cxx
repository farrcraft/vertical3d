/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "HitShader.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>

namespace v3d::render::offline::trace {

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

HitShader::HitShader(const Scene* scene, v3d::render::offline::Textures* textures) :
    scene_(scene), textures_(textures) {
}

HitShader::Run & HitShader::run(const v3d::render::offline::sl::InstancePtr & shader) {
    Run & held = runs_[std::make_pair(depth_, &shader->program())];
    if (held.program == &shader->program()) {
        return held;
    }
    held.program = &shader->program();
    held.machine.renderer(this);
    // a batch of one, which is the whole point: a hit is not a special case of the
    // model, it is a batch one wide
    held.machine.prepare(shader->program(), 1);
    return held;
}

bool HitShader::space(const std::string & name, glm::mat4x4* matrix) {
    // a hit is in world space, because that is where the scene is. moya's grids are in
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
        *matrix = scene_->view();
        return true;
    }
    // a raytracer has no screen or raster space to speak of: it does not project, it
    // asks a camera for a ray through a pixel and the matrix that would do it is the
    // camera's own business
    return false;
}

const std::vector<v3d::render::offline::sl::Placed> & HitShader::shining() const {
    static const std::vector<v3d::render::offline::sl::Placed> none;
    if (hit_ != nullptr && hit_->primitive != nullptr && hit_->primitive->lights()) {
        return *hit_->primitive->lights();
    }
    return scene_ == nullptr ? none : scene_->lights();
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
    Run & held = run(source.shader);
    const Program & program = source.shader->program();

    // a light's parameters are stated in the space the scene instanced it in, and the
    // point it is lighting is in world space
    const glm::mat4x4 was = placement_;
    placement_ = source.placement;
    source.shader->write(&held.machine, source.placement);

    put(&held.machine, program.symbol("Ps"), surface.triple(0));
    put(&held.machine, program.symbol("P"),
        v3d::render::offline::sl::ptransform(source.placement, glm::vec3(0.0f)));
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
    *ambient = source.shader->ambient();
    return true;
}

bool HitShader::transmission(const Value & from, const Value & to, Value* fraction) {
    if (scene_ == nullptr || fraction == nullptr) {
        return false;
    }
    fraction->triple(0, transmitted(from.triple(0), to.triple(0),
        hit_ == nullptr ? glm::vec3(0.0f) : hit_->geometric));
    return true;
}

glm::vec3 HitShader::transmitted(const glm::vec3 & from, const glm::vec3 & to, const glm::vec3 & geometric) {
    const glm::vec3 along = to - from;
    const float span = glm::length(along);
    if (scene_ == nullptr || span <= 0.0f) {
        return glm::vec3(1.0f);
    }

    /*
        The ray leaves the surface it is shading, so it starts off it: exactly on it, it
        meets it. The offset is along the geometric normal and toward the light, which is
        the side the light is on - a ray leaving the back of a surface would otherwise
        start inside it.
    */
    const float side = glm::dot(geometric, along) < 0.0f ? -1.0f : 1.0f;
    const v3d::type::geometry::Ray ray(from + geometric * (side * EPSILON), along / span);

    /*
        Everything between here and the light takes its share, and nothing beyond the light
        does. An occluder lets through what its Os does not stop, read off the primitive
        rather than by running its shader: a shadow is a visibility question, and asking
        a shader would make every shadow ray a shading one.
    */
    glm::vec3 through(1.0f);
    float past = 0.0f;
    Hit blocker;
    while (scene_->nearest(ray, past, &blocker, time_) && blocker.distance < span) {
        through *= glm::vec3(1.0f) - blocker.primitive->opacity();
        if (std::max(through.r, std::max(through.g, through.b)) <= 0.0f) {
            break;
        }
        past = blocker.distance;
    }
    return through;
}

const v3d::render::offline::Texture* HitShader::texture(const std::string & name) {
    return textures_ == nullptr ? nullptr : textures_->find(name);
}

void HitShader::time(float when) {
    time_ = when;
}

bool HitShader::trace(const Value & origin, const Value & direction, Value* colour) {
    if (scene_ == nullptr || colour == nullptr) {
        return false;
    }
    colour->triple(0, traced(origin.triple(0), direction.triple(0),
        hit_ == nullptr ? glm::vec3(0.0f) : hit_->geometric));
    return true;
}

glm::vec3 HitShader::traced(const glm::vec3 & origin, const glm::vec3 & direction, const glm::vec3 & geometric) {
    if (scene_ == nullptr) {
        return glm::vec3(0.0f);
    }
    const float span = glm::length(direction);
    // a ray past the scene's depth answers the background, which is what bounds the
    // recursion
    if (depth_ >= scene_->traceDepth() || span <= 0.0f) {
        return scene_->background();
    }
    const float side = glm::dot(geometric, direction) < 0.0f ? -1.0f : 1.0f;
    const glm::vec3 start = origin + geometric * (side * EPSILON);

    // shade() leaves both of these as the traced surface had them, and the shader that
    // traced is still running and reads them again
    const Hit* was = hit_;
    const glm::mat4x4 placed = placement_;
    depth_++;
    const Seen seen = see(v3d::type::geometry::Ray(start, direction / span));
    depth_--;
    hit_ = was;
    placement_ = placed;
    return seen.colour;
}

HitShader::Seen HitShader::see(const v3d::type::geometry::Ray & ray) {
    Seen seen;
    if (scene_ == nullptr) {
        return seen;
    }
    /*
        Each surface goes behind what is in front of it: C += (1 - A) Ci and A += (1 - A) Oi,
        with Ci already premultiplied. The next is looked for past the last, which is what
        ends the walk - the distances only grow, and a ray meets each primitive at most
        twice.
    */
    float past = 0.0f;
    Hit hit;
    while (scene_->nearest(ray, past, &hit, time_)) {
        glm::vec3 opacity(1.0f);
        const glm::vec3 colour = shade(hit, &opacity);
        if (!seen.hit) {
            seen.hit = true;
            seen.distance = hit.distance;
        }
        seen.colour += (glm::vec3(1.0f) - seen.opacity) * colour;
        seen.opacity += (glm::vec3(1.0f) - seen.opacity) * opacity;
        if (std::min(seen.opacity.r, std::min(seen.opacity.g, seen.opacity.b)) >= 1.0f) {
            return seen;
        }
        past = hit.distance;
    }
    seen.colour += (glm::vec3(1.0f) - seen.opacity) * scene_->background();
    return seen;
}

glm::vec3 HitShader::shade(const Hit & hit) {
    glm::vec3 opacity(1.0f);
    return shade(hit, &opacity);
}

glm::vec3 HitShader::shade(const Hit & hit, glm::vec3* opacity) {
    if (hit.primitive == nullptr) {
        *opacity = glm::vec3(0.0f);
        return scene_ == nullptr ? glm::vec3(0.0f) : scene_->background();
    }
    const Primitive & primitive = *hit.primitive;
    *opacity = primitive.opacity();
    const v3d::render::offline::sl::Placed & surface = primitive.surface();
    if (!surface.shader) {
        // a primitive a scene built without a shader is its own colour, which is the
        // picture this renderer drew before there was a language to ask for another
        return primitive.opacity() * primitive.colour();
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
        scene_ == nullptr ? glm::vec3(0.0f) : scene_->eye());
    put(&held.machine, program.symbol("Cs"), primitive.colour());
    put(&held.machine, program.symbol("Os"), primitive.opacity());
    put(&held.machine, program.symbol("Oi"), primitive.opacity());
    put(&held.machine, program.symbol("s"), hit.s);
    put(&held.machine, program.symbol("t"), hit.t);
    put(&held.machine, program.symbol("u"), hit.u);
    put(&held.machine, program.symbol("v"), hit.v);

    if (!held.machine.run(program)) {
        hit_ = nullptr;
        return primitive.colour();
    }
    const int result = program.symbol("Ci");
    const glm::vec3 colour = result < 0 ? primitive.colour() :
        held.machine.value(result).triple(0);
    const int coverage = program.symbol("Oi");
    if (coverage >= 0) {
        *opacity = held.machine.value(coverage).triple(0);
    }
    hit_ = nullptr;
    return colour;
}

};  // namespace v3d::render::offline::trace
