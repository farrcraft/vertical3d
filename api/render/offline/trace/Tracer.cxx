/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Tracer.h"

#include <api/render/offline/Textures.h>
#include <api/render/offline/sl/Instance.h>
#include <api/type/geometry/Line.h>
#include <api/type/geometry/Ray.h>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include <glm/geometric.hpp>

#include "Hit.h"
#include "HitShader.h"
#include "Primitive.h"
#include "Scene.h"

namespace v3d::render::offline::trace {

namespace {

/**
 * How far along the geometric normal a ray leaving a surface starts.
 *
 * **This is a trap.** A shadow ray that starts exactly on the surface hits the surface it
 * left, and every lit pixel comes out black. The pattern looks like a fault in the normals
 * rather than a numerical one. The offset is along the normal rather than along the ray,
 * because an offset along a ray nearly parallel to the surface stays on the surface.
 **/
const float EPSILON = 1.0e-4F;

/**
 * Whether two primitives can be shaded by one run: the same surface shader in the same
 * placement, the same object space and the same lights. Everything else a hit reads is a
 * point of the batch.
 **/
bool shareable(const Primitive & one, const Primitive & other) {
    if (&one == &other) {
        return true;
    }
    return one.surface().shader == other.surface().shader && one.surface().placement == other.surface().placement &&
        one.object() == other.object() && one.lights() == other.lights();
}

/**
 * Put a surface behind what a ray has seen in front of it: C += (1 - A) Ci and
 * A += (1 - A) Oi, with Ci already premultiplied.
 *
 * @return whether the ray is now opaque, so that nothing behind can show
 **/
bool composite(Tracer::Seen* seen, const glm::vec3 & colour, const glm::vec3 & opacity, float distance) {
    if (!seen->hit) {
        seen->hit = true;
        seen->distance = distance;
    }
    seen->colour += (glm::vec3(1.0F) - seen->opacity) * colour;
    seen->opacity += (glm::vec3(1.0F) - seen->opacity) * opacity;
    return std::min(seen->opacity.r, std::min(seen->opacity.g, seen->opacity.b)) >= 1.0F;
}

};  // namespace

Tracer::Tracer(const Scene* scene, v3d::render::offline::Textures* textures) :
    scene_(scene), textures_(textures) {
    if (scene_ != nullptr) {
        poses_ = scene_->poses(0.0F);
    }
}

const Scene* Tracer::scene() const {
    return scene_;
}

v3d::render::offline::Textures* Tracer::textures() const {
    return textures_;
}

Tracer::Run & Tracer::run(const v3d::render::offline::sl::InstancePtr & shader, unsigned int batch) {
    Run & held = runs_[std::make_pair(depth_, &shader->program())];
    if (held.program == &shader->program() && held.batch == batch) {
        return held;
    }
    held.program = &shader->program();
    held.batch = batch;
    held.machine.prepare(shader->program(), batch);
    held.globals = v3d::render::offline::sl::Globals(shader->program());
    return held;
}

const Scene::Poses & Tracer::poses() const {
    return poses_;
}

void Tracer::time(float when) {
    time_ = when;
    if (scene_ != nullptr) {
        poses_ = scene_->poses(when);
    }
}

float Tracer::time() const {
    return time_;
}

glm::vec3 Tracer::transmitted(const v3d::type::geometry::Line & path, const glm::vec3 & geometric) {
    return transmitted(path, geometric, poses_);
}

glm::vec3 Tracer::transmitted(const v3d::type::geometry::Line & path, const glm::vec3 & geometric, const Scene::Poses & poses) {
    const glm::vec3 & along = path.direction;
    const float span = glm::length(along);
    if (scene_ == nullptr || span <= 0.0F) {
        return glm::vec3(1.0F);
    }

    /*
        The ray starts a small offset from the surface it leaves, because a ray that starts
        exactly on the surface hits it. The offset is along the geometric normal, on the
        side the light is on. A ray leaving the back of a surface would otherwise start
        inside it.
    */
    const float side = glm::dot(geometric, along) < 0.0F ? -1.0F : 1.0F;
    // aimed from where it starts at where the path ends, so its length is the distance from
    // the offset start to the light
    const glm::vec3 start = path.origin + (geometric * (side * EPSILON));
    const glm::vec3 toward = path.origin + along - start;
    const float length = glm::length(toward);
    if (length <= EPSILON) {
        return glm::vec3(1.0F);
    }
    const v3d::type::geometry::Ray ray({.origin = start, .direction = toward / length});

    /*
        Each occluder between the shading point and the light lets through what its Os
        does not stop. Nothing beyond the light counts. Os is read from the primitive, and
        its shader is not run: a shadow ray only tests visibility, and running a shader would
        make every shadow ray a shading ray.
    */
    glm::vec3 through(1.0F);
    float past = 0.0F;
    Hit blocker;
    // the ray stops short of its end by the same offset, because the end can lie on a surface
    // too: an area light's samples lie on its own geometry, which must not shadow it
    const float reach = length - EPSILON;
    while (scene_->nearest(ray, past, &blocker, poses) && blocker.distance < reach) {
        through *= glm::vec3(1.0F) - blocker.primitive->opacity();
        if (std::max(through.r, std::max(through.g, through.b)) <= 0.0F) {
            break;
        }
        past = blocker.distance;
    }
    return through;
}

glm::vec3 Tracer::traced(const v3d::type::geometry::Line & path, const glm::vec3 & geometric) {
    return traced(std::vector<v3d::type::geometry::Line>{ path }, std::vector<glm::vec3>{ geometric },
        std::vector<float>{ time_ }).front();
}

std::vector<glm::vec3> Tracer::traced(const std::vector<v3d::type::geometry::Line> & paths,
    const std::vector<glm::vec3> & geometric, const std::vector<float> & times) {
    std::vector<glm::vec3> colours(paths.size(), glm::vec3(0.0F));
    if (scene_ == nullptr) {
        return colours;
    }
    std::vector<Look> looks;
    std::vector<std::size_t> owners;
    looks.reserve(paths.size());
    owners.reserve(paths.size());
    for (std::size_t i = 0; i < paths.size(); i++) {
        const glm::vec3 & direction = paths[i].direction;
        const float span = glm::length(direction);
        // a ray past the scene's depth returns the background, which bounds the recursion
        if (depth_ >= scene_->traceDepth() || span <= 0.0F) {
            colours[i] = scene_->background();
            continue;
        }
        const float side = glm::dot(geometric[i], direction) < 0.0F ? -1.0F : 1.0F;
        const glm::vec3 start = paths[i].origin + (geometric[i] * (side * EPSILON));
        Look look;
        look.ray = v3d::type::geometry::Ray({.origin = start, .direction = direction / span});
        look.time = i < times.size() ? times[i] : time_;
        looks.push_back(look);
        owners.push_back(i);
    }
    if (looks.empty()) {
        return colours;
    }

    depth_++;
    const std::vector<Seen> seen = see(looks);
    depth_--;
    for (std::size_t i = 0; i < owners.size(); i++) {
        colours[owners[i]] = seen[i].colour;
    }
    return colours;
}

Tracer::Seen Tracer::see(const v3d::type::geometry::Ray & ray) {
    Look look;
    look.ray = ray;
    look.time = time_;
    return see(std::vector<Look>{ look }).front();
}

std::vector<Tracer::Seen> Tracer::see(const std::vector<Look> & looks) {
    std::vector<Seen> seen(looks.size());
    if (scene_ == nullptr) {
        return seen;
    }
    std::vector<Scene::Poses> placed;
    const std::vector<const Scene::Poses*> posed = poses(looks, &placed);

    /*
        Each round finds the next surface along every ray still going, shades them together,
        and puts each behind what its ray has seen. Each search starts past the ray's last hit,
        so its distance only grows. A ray meets each primitive at most twice, so the rounds end.
    */
    std::vector<float> past(looks.size(), 0.0F);
    std::vector<std::size_t> going(looks.size());
    for (std::size_t i = 0; i < looks.size(); i++) {
        going[i] = i;
    }
    std::vector<Hit> hits;
    std::vector<float> times;
    std::vector<const Scene::Poses*> posedHits;
    std::vector<std::size_t> owners;
    std::vector<glm::vec3> colours;
    std::vector<glm::vec3> opacities;
    while (!going.empty()) {
        hits.clear();
        times.clear();
        posedHits.clear();
        owners.clear();
        for (const std::size_t ray : going) {
            Hit hit;
            if (scene_->nearest(looks[ray].ray, past[ray], &hit, *posed[ray])) {
                hits.push_back(hit);
                times.push_back(looks[ray].time);
                posedHits.push_back(posed[ray]);
                owners.push_back(ray);
            } else {
                seen[ray].colour += (glm::vec3(1.0F) - seen[ray].opacity) * scene_->background();
            }
        }
        shade(hits, times, posedHits, &colours, &opacities);

        going.clear();
        for (std::size_t i = 0; i < hits.size(); i++) {
            const std::size_t ray = owners[i];
            if (!composite(&seen[ray], colours[i], opacities[i], hits[i].distance)) {
                past[ray] = hits[i].distance;
                going.push_back(ray);
            }
        }
    }
    return seen;
}

std::vector<const Scene::Poses*> Tracer::poses(const std::vector<Look> & looks, std::vector<Scene::Poses>* placed) const {
    // a scene with nothing moving has no poses to place, and a ray at the same time as the one
    // before it shares that one's poses
    std::vector<const Scene::Poses*> posed(looks.size(), &poses_);
    if (poses_.ahead.empty()) {
        return posed;
    }
    placed->reserve(looks.size());
    for (std::size_t i = 0; i < looks.size(); i++) {
        if (i > 0 && looks[i].time == looks[i - 1].time) {
            posed[i] = posed[i - 1];
        } else if (looks[i].time != time_) {
            placed->push_back(scene_->poses(looks[i].time));
            posed[i] = &placed->back();
        }
    }
    return posed;
}

void Tracer::shade(const std::vector<Hit> & hits, const std::vector<float> & times,
    const std::vector<const Scene::Poses*> & poses, std::vector<glm::vec3>* colours,
    std::vector<glm::vec3>* opacities) {
    colours->assign(hits.size(), glm::vec3(0.0F));
    opacities->assign(hits.size(), glm::vec3(1.0F));

    // the hits that one run can shade, in the order they were found
    std::vector<std::vector<std::size_t>> groups;
    for (std::size_t i = 0; i < hits.size(); i++) {
        const Primitive & primitive = *hits[i].primitive;
        const std::vector<std::vector<std::size_t>>::iterator group = std::ranges::find_if(groups,
            [&](const std::vector<std::size_t> & members) { return shareable(*hits[members.front()].primitive, primitive); });
        if (group == groups.end()) {
            groups.emplace_back(1, i);
        } else {
            group->push_back(i);
        }
    }

    std::vector<const Hit*> batch;
    std::vector<float> when;
    std::vector<const Scene::Poses*> placed;
    std::vector<glm::vec3> shadedColours;
    std::vector<glm::vec3> shadedOpacities;
    for (const std::vector<std::size_t> & members : groups) {
        for (std::size_t first = 0; first < members.size(); first += BATCH) {
            const std::size_t last = std::min(members.size(), first + BATCH);
            batch.clear();
            when.clear();
            placed.clear();
            for (std::size_t member = first; member < last; member++) {
                batch.push_back(&hits[members[member]]);
                when.push_back(times[members[member]]);
                placed.push_back(poses[members[member]]);
            }
            HitShader shader(this, batch, when, placed);
            shader.shade(&shadedColours, &shadedOpacities);
            for (std::size_t member = first; member < last; member++) {
                (*colours)[members[member]] = shadedColours[member - first];
                (*opacities)[members[member]] = shadedOpacities[member - first];
            }
        }
    }
}

glm::vec3 Tracer::shade(const Hit & hit) {
    glm::vec3 opacity(1.0F);
    return shade(hit, &opacity);
}

glm::vec3 Tracer::shade(const Hit & hit, glm::vec3* opacity) {
    if (hit.primitive == nullptr) {
        *opacity = glm::vec3(0.0F);
        return scene_ == nullptr ? glm::vec3(0.0F) : scene_->background();
    }
    HitShader shader(this, hit);
    return shader.shade(opacity);
}

};  // namespace v3d::render::offline::trace
