/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Tracer.h"

#include <algorithm>
#include <utility>

#include <glm/geometric.hpp>

#include "HitShader.h"

namespace v3d::render::offline::trace {

namespace {

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

};  // namespace

Tracer::Tracer(const Scene* scene, v3d::render::offline::Textures* textures) :
    scene_(scene), textures_(textures) {
    if (scene_ != nullptr) {
        poses_ = scene_->poses(0.0f);
    }
}

const Scene* Tracer::scene() const {
    return scene_;
}

v3d::render::offline::Textures* Tracer::textures() const {
    return textures_;
}

Tracer::Run & Tracer::run(const v3d::render::offline::sl::InstancePtr & shader) {
    Run & held = runs_[std::make_pair(depth_, &shader->program())];
    if (held.program == &shader->program()) {
        return held;
    }
    held.program = &shader->program();
    // a batch of one, which is the whole point: a hit is not a special case of the
    // model, it is a batch one wide
    held.machine.prepare(shader->program(), 1);
    held.globals = v3d::render::offline::sl::Globals(shader->program());
    return held;
}

void Tracer::time(float when) {
    if (scene_ != nullptr) {
        poses_ = scene_->poses(when);
    }
}

glm::vec3 Tracer::transmitted(const glm::vec3 & from, const glm::vec3 & to, const glm::vec3 & geometric) {
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
    while (scene_->nearest(ray, past, &blocker, poses_) && blocker.distance < span) {
        through *= glm::vec3(1.0f) - blocker.primitive->opacity();
        if (std::max(through.r, std::max(through.g, through.b)) <= 0.0f) {
            break;
        }
        past = blocker.distance;
    }
    return through;
}

glm::vec3 Tracer::traced(const glm::vec3 & origin, const glm::vec3 & direction, const glm::vec3 & geometric) {
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

    depth_++;
    const Seen seen = see(v3d::type::geometry::Ray(start, direction / span));
    depth_--;
    return seen.colour;
}

Tracer::Seen Tracer::see(const v3d::type::geometry::Ray & ray) {
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
    while (scene_->nearest(ray, past, &hit, poses_)) {
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

glm::vec3 Tracer::shade(const Hit & hit) {
    glm::vec3 opacity(1.0f);
    return shade(hit, &opacity);
}

glm::vec3 Tracer::shade(const Hit & hit, glm::vec3* opacity) {
    if (hit.primitive == nullptr) {
        *opacity = glm::vec3(0.0f);
        return scene_ == nullptr ? glm::vec3(0.0f) : scene_->background();
    }
    HitShader shader(this, hit);
    return shader.shade(opacity);
}

};  // namespace v3d::render::offline::trace
