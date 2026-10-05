/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Texture.h>
#include <api/render/offline/Textures.h>
#include <api/render/offline/sl/Globals.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/render/offline/sl/runtime/Renderer.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Hit.h"
#include "Scene.h"

namespace v3d::render::offline::trace {

/**
 * Runs a surface shader over one hit, which is a batch one point wide: what a ray sees, for
 * the ray hider's primary rays and for traced ones, per ADR-0077.
 *
 * No special case and no second path: the same program and the same instructions that
 * run over a grid of a hundred in moya, with a mask one bit wide. That is the whole
 * reason ADR-0026's execution model is a batch rather than a shading point.
 *
 * **A hit's current space is world space**, because that is where the scene is. moya's
 * grids shade in camera space, and the pair is the thing that will confuse a reader - which
 * is why the space table is a renderer callback rather than a constant the library holds.
 *
 * **One shader run per pixel is slow and that is accepted here.** Batching the hits of a
 * scanline that share a shader is the obvious next thing; the 64 by 48 references do not
 * need it and a real image will.
 **/
class HitShader final : public v3d::render::offline::sl::runtime::Renderer {
 public:
    /**
     * @param textures the images a shader may name, or null for a scene that names none,
     *        which leaves every texture() black
     **/
    explicit HitShader(const Scene* scene, v3d::render::offline::Textures* textures = nullptr);

    /**
     * What a ray sees: every surface along it composited front to back by its Oi, until
     * they are opaque or the ray leaves the scene, and the background behind what is left.
     *
     * A ray going on through a surface is not a traced ray, so it does not count against
     * the scene's trace depth: a stack of panes is as deep as it is.
     **/
    class Seen final {
     public:
        /** Premultiplied, as Ci is, with the background already behind it. **/
        glm::vec3 colour = glm::vec3(0.0f);
        glm::vec3 opacity = glm::vec3(0.0f);
        /** Whether anything was there, and how far away the first of it was. **/
        bool hit = false;
        float distance = 0.0f;
    };
    Seen see(const v3d::type::geometry::Ray & ray);

    /**
     * The colour of one hit: the surface shader's Ci.
     *
     * A primitive with no shader is its own flat colour, which is what a scene built in
     * code without one asks for.
     */
    glm::vec3 shade(const Hit & hit);
    /**
     * And its Oi. A shader that never writes Oi is as opaque as its primitive, and so is
     * a primitive with no shader at all.
     **/
    glm::vec3 shade(const Hit & hit, glm::vec3* opacity);

    /**
     * When the sample being shaded was taken, which is when its shadow and traced rays
     * look at the scene.
     **/
    void time(float when);

    /**
     * transmission() and trace() in world space, for a renderer whose shading point is not
     * a hit of this scene: moya's grids, per ADR-0077.
     *
     * @param geometric the plane of the surface the ray leaves, which it is started off so
     *        that it does not meet that surface again; zero starts it where it is
     **/
    glm::vec3 transmitted(const glm::vec3 & from, const glm::vec3 & to, const glm::vec3 & geometric);
    glm::vec3 traced(const glm::vec3 & origin, const glm::vec3 & direction, const glm::vec3 & geometric);

    // what the machine asks a renderer for
    bool space(const std::string & name, glm::mat4x4* matrix) override;
    unsigned int lights() override;
    bool light(unsigned int index, const v3d::render::offline::sl::runtime::Value & surface,
        v3d::render::offline::sl::runtime::Value* direction,
        v3d::render::offline::sl::runtime::Value* colour,
        std::vector<char>* reached, bool* ambient) override;
    bool transmission(const v3d::render::offline::sl::runtime::Value & from,
        const v3d::render::offline::sl::runtime::Value & to,
        v3d::render::offline::sl::runtime::Value* fraction) override;
    bool trace(const v3d::render::offline::sl::runtime::Value & origin,
        const v3d::render::offline::sl::runtime::Value & direction,
        v3d::render::offline::sl::runtime::Value* colour) override;
    const v3d::render::offline::Texture* texture(const std::string & name) override;

 private:
    /**
     * A machine sized for one program and a batch of one. Kept between hits, because a
     * render is one of these per pixel and sizing a register file per pixel is the one
     * allocation that would show.
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

    Run & run(const v3d::render::offline::sl::InstancePtr & shader);

    /** The lights on the hit being shaded: its primitive's, or the scene's. **/
    const std::vector<v3d::render::offline::sl::Placed> & shining() const;

    const Scene* scene_;
    v3d::render::offline::Textures* textures_;
    float time_ = 0.0f;
    /** The hit being shaded, for the space table and for the shadow ray's offset. **/
    const Hit* hit_ = nullptr;
    /** What the shader being run was placed by, which is its own space. **/
    glm::mat4x4 placement_ = glm::mat4x4(1.0f);
    /**
     * How many traced rays deep the run is.
     *
     * A ray a shader traced may hit a surface whose shader traces again, and nothing in
     * the language stops that going round for ever: the scene's trace depth does.
     */
    unsigned int depth_ = 0;
    std::map<std::pair<unsigned int, const v3d::render::offline::sl::runtime::Program*>, Run> runs_;
};

};  // namespace v3d::render::offline::trace
