/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Textures.h>
#include <api/render/offline/sl/Globals.h>
#include <api/render/offline/sl/Instance.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/type/geometry/Line.h>
#include <api/type/geometry/Ray.h>

#include <map>
#include <utility>

#include <glm/vec3.hpp>

#include "Hit.h"
#include "Scene.h"

namespace v3d::render::offline::trace {

/**
 * Traces rays through a scene and shades what they meet: the ray hider's primary rays, and
 * the shadow and traced rays a shader requests under any of moya's hiders.
 *
 * Each hit is shaded by a HitShader made for it on the stack, which handles the shader's
 * callbacks. A traced ray's hit gets its own HitShader, so a shader that calls trace() part
 * way through its run resumes with its state unchanged.
 **/
class Tracer final {
 public:
    /**
     * @param textures the images a shader may name, or null for a scene that names none,
     *        which leaves every texture() black
     **/
    explicit Tracer(const Scene* scene, v3d::render::offline::Textures* textures = nullptr);

    /**
     * What a ray sees: every surface along it composited front to back by its Oi, until
     * they are opaque or the ray leaves the scene, and the background behind what is left.
     *
     * A ray that continues through a surface is not a traced ray. It does not count against
     * the scene's trace depth, so any number of stacked transparent surfaces is composited.
     **/
    class Seen final {
     public:
        /** Premultiplied, as Ci is, with the background already behind it. **/
        glm::vec3 colour = glm::vec3(0.0F);
        glm::vec3 opacity = glm::vec3(0.0F);
        /** Whether the ray hit anything, and the distance to the first hit. **/
        bool hit = false;
        float distance = 0.0F;
    };
    Seen see(const v3d::type::geometry::Ray & ray);

    /**
     * The colour of one hit: the surface shader's Ci.
     *
     * A primitive with no shader is drawn in its own flat colour.
     **/
    glm::vec3 shade(const Hit & hit);
    /**
     * The Oi of one hit. A shader that never writes Oi is as opaque as its primitive, and so is
     * a primitive with no shader at all.
     **/
    glm::vec3 shade(const Hit & hit, glm::vec3* opacity);

    /**
     * When the sample being shaded was taken, which is the time its shadow and traced rays
     * see the scene at. The moving primitives are placed for it here, once.
     **/
    void time(float when);

    /**
     * transmission() and trace() in world space, for a hit in this scene and for a renderer
     * whose shading point is not a hit, such as moya's grids.
     *
     * @param path for transmitted(), from the shading point to the light: its direction ends at
     *        the light. For traced(), where the ray starts and which way it goes.
     * @param geometric the geometric normal of the surface the ray leaves. The ray starts a
     *        small offset along it, so that it does not meet that surface again. Zero starts
     *        the ray at the path's origin
     **/
    glm::vec3 transmitted(const v3d::type::geometry::Line & path, const glm::vec3 & geometric);
    glm::vec3 traced(const v3d::type::geometry::Line & path, const glm::vec3 & geometric);

    const Scene* scene() const;
    v3d::render::offline::Textures* textures() const;

    /**
     * A machine sized for one program and a batch of one, and that program's globals. Kept
     * between hits, so that a register file is not allocated for every hit.
     *
     * There is one per program per trace depth. A shader that calls trace() is still running
     * when the traced hit is shaded, possibly by the same program. A machine per depth keeps
     * the first run's registers intact.
     **/
    class Run final {
     public:
        v3d::render::offline::sl::runtime::Machine machine;
        const v3d::render::offline::sl::runtime::Program* program = nullptr;
        v3d::render::offline::sl::Globals globals;
    };

    /**
     * The machine for a program at the depth being traced, prepared if it was not already.
     **/
    Run & run(const v3d::render::offline::sl::InstancePtr & shader);

 private:
    const Scene* scene_;
    v3d::render::offline::Textures* textures_;
    Scene::Poses poses_;
    /**
     * How many traced rays deep the run is.
     *
     * A ray a shader traced may hit a surface whose shader traces again. Nothing in the
     * language stops that recursion, so the scene's trace depth limits it.
     **/
    unsigned int depth_ = 0;
    std::map<std::pair<unsigned int, const v3d::render::offline::sl::runtime::Program*>, Run> runs_;
};

};  // namespace v3d::render::offline::trace
