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
#include <vector>

#include <glm/vec3.hpp>

#include "Hit.h"
#include "Scene.h"

namespace v3d::render::offline::trace {

/**
 * Traces rays through a scene and shades what they meet: the ray hider's primary rays, and
 * the shadow and traced rays a shader requests under any of moya's hiders.
 *
 * **Rays are traced and shaded in batches.** see() takes a batch of rays. The hits that share
 * a surface shader, its placement, an object space and a list of lights are shaded by one run
 * of the shader, up to BATCH hits at a time. A shader in that run that calls trace() traces
 * the rays of all its hits as one batch, one depth further down. A ray that continues through
 * a surface it can see through joins the next batch of the same see().
 *
 * Each batch is shaded by a HitShader made for it on the stack, which handles the shader's
 * callbacks. A traced batch gets its own HitShader, so a shader that calls trace() part way
 * through its run resumes with its state unchanged.
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

    /**
     * A ray to look along, and the time its sample was taken, which places the moving
     * primitives it can meet.
     **/
    class Look final {
     public:
        v3d::type::geometry::Ray ray;
        float time = 0.0F;
    };

    /**
     * What each ray of a batch sees, in the order the batch gives them. The result for a ray
     * does not depend on the other rays in the batch.
     **/
    std::vector<Seen> see(const std::vector<Look> & looks);
    /** What one ray sees, at the time set by time(). **/
    Seen see(const v3d::type::geometry::Ray & ray);

    /**
     * The colour of one hit, at the time set by time(): the surface shader's Ci.
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
     * When the sample being shaded was taken, for a caller that shades one point at a time.
     * The moving primitives are placed for it here, once. A batch given to see() carries a
     * time for each of its rays instead.
     **/
    void time(float when);
    float time() const;

    /**
     * transmission() and trace() in world space, for a hit in this scene and for a renderer
     * whose shading point is not a hit, such as moya's grids. Each traces at the time set by
     * time().
     *
     * @param path for transmitted(), from the shading point to the light: its direction ends at
     *        the light. For traced(), where the ray starts and which way it goes.
     * @param geometric the geometric normal of the surface the ray leaves. The ray starts a
     *        small offset along it, so that it does not meet that surface again. Zero starts
     *        the ray at the path's origin
     **/
    glm::vec3 transmitted(const v3d::type::geometry::Line & path, const glm::vec3 & geometric);
    glm::vec3 traced(const v3d::type::geometry::Line & path, const glm::vec3 & geometric);

    /** transmitted() with the moving primitives placed as `poses` places them. **/
    glm::vec3 transmitted(const v3d::type::geometry::Line & path, const glm::vec3 & geometric, const Scene::Poses & poses);

    /**
     * traced() for a batch of rays, each from its own surface at its own time, traced and
     * shaded as one batch. The three lists are the same length.
     **/
    std::vector<glm::vec3> traced(const std::vector<v3d::type::geometry::Line> & paths,
        const std::vector<glm::vec3> & geometric, const std::vector<float> & times);

    const Scene* scene() const;
    /** Where the moving primitives are at the time set by time(). **/
    const Scene::Poses & poses() const;

    /**
     * How many points a hit is shaded as when its shader differentiates: the hit, and the two
     * a step along u and along v from it that its derivatives difference.
     **/
    static constexpr unsigned int STENCIL = 3;
    /**
     * The most hits one run of a shader shades. Past this many, the hits are split into runs
     * of this size, so that a register file stays a modest size.
     **/
    static constexpr unsigned int BATCH = 64;
    v3d::render::offline::Textures* textures() const;

    /**
     * A machine sized for one program and a batch, and that program's globals. Kept between
     * batches, so that a register file is not allocated for every hit.
     *
     * There is one per program per trace depth. A shader that calls trace() is still running
     * when the traced hits are shaded, possibly by the same program. A machine per depth keeps
     * the first run's registers intact.
     **/
    class Run final {
     public:
        v3d::render::offline::sl::runtime::Machine machine;
        const v3d::render::offline::sl::runtime::Program* program = nullptr;
        v3d::render::offline::sl::Globals globals;
        unsigned int batch = 0;
    };

    /**
     * The machine for a program at the depth being traced, prepared for a batch of this many
     * points if it was not already.
     **/
    Run & run(const v3d::render::offline::sl::InstancePtr & shader, unsigned int batch);

 private:
    /**
     * Where the moving primitives are for each ray of a batch, at its time.
     *
     * @param placed where the poses computed for the batch are kept. It must outlive the
     *        pointers returned, which point into it or at the poses for time()
     **/
    std::vector<const Scene::Poses*> poses(const std::vector<Look> & looks, std::vector<Scene::Poses>* placed) const;

    /**
     * Shade a batch of hits, each at its own time and with the moving primitives placed for
     * it, and write each one's Ci and Oi.
     **/
    void shade(const std::vector<Hit> & hits, const std::vector<float> & times,
        const std::vector<const Scene::Poses*> & poses, std::vector<glm::vec3>* colours,
        std::vector<glm::vec3>* opacities);

    const Scene* scene_;
    v3d::render::offline::Textures* textures_;
    float time_ = 0.0F;
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
