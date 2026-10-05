/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Textures.h>
#include <api/render/offline/sl/Globals.h>
#include <api/render/offline/sl/Instance.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/type/geometry/Ray.h>

#include <map>
#include <utility>

#include <glm/vec3.hpp>

#include "Hit.h"
#include "Scene.h"

namespace v3d::render::offline::trace {

/**
 * Traces rays through a scene and shades what they meet: the ray hider's primary rays, and
 * the shadow and traced rays a shader requests under either of moya's hiders.
 *
 * Each hit is shaded by a HitShader made for it on the stack, which handles the shader's
 * callbacks. A traced ray shades its hit with another HitShader, so a shader that traces
 * part way through its run is left as it was.
 *
 * **One shader run per pixel is slow.** Batching the hits of a scanline that share a shader
 * would make it faster.
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
     * A ray going on through a surface is not a traced ray, so it does not count against
     * the scene's trace depth, and a stack of panes of any depth is composited.
     **/
    class Seen final {
     public:
        /** Premultiplied, as Ci is, with the background already behind it. **/
        glm::vec3 colour = glm::vec3(0.0f);
        glm::vec3 opacity = glm::vec3(0.0f);
        /** Whether the ray hit anything, and the distance to the first hit. **/
        bool hit = false;
        float distance = 0.0f;
    };
    Seen see(const v3d::type::geometry::Ray & ray);

    /**
     * The colour of one hit: the surface shader's Ci.
     *
     * A primitive with no shader is drawn in its own flat colour.
     */
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
     * @param geometric the plane of the surface the ray leaves, which it is started off so
     *        that it does not meet that surface again; zero starts it where it is
     **/
    glm::vec3 transmitted(const glm::vec3 & from, const glm::vec3 & to, const glm::vec3 & geometric);
    glm::vec3 traced(const glm::vec3 & origin, const glm::vec3 & direction, const glm::vec3 & geometric);

    const Scene* scene() const;
    v3d::render::offline::Textures* textures() const;

    /**
     * A machine sized for one program and a batch of one, and that program's globals. Kept
     * between hits, because a render runs one per pixel and sizing a register file per pixel
     * would be the costliest allocation in the render.
     *
     * There is one per program per trace depth. A surface tracing into another with the
     * same shader is still running its machine when the other starts, and sharing one
     * would overwrite the registers the first is part way through.
     */
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
     * A ray a shader traced may hit a surface whose shader traces again, and nothing in
     * the language stops that recursing for ever; the scene's trace depth limits it.
     */
    unsigned int depth_ = 0;
    std::map<std::pair<unsigned int, const v3d::render::offline::sl::runtime::Program*>, Run> runs_;
};

};  // namespace v3d::render::offline::trace
