/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/runtime/Machine.h>
#include <api/render/offline/sl/runtime/Renderer.h>

#include <map>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Scene.h"

namespace v3d::talyn {

/**
 * Runs a surface shader over one hit, which is a batch one point wide.
 *
 * No special case and no second path: the same program and the same instructions that
 * run over a grid of a hundred in moya, with a mask one bit wide. That is the whole
 * reason ADR-0026's execution model is a batch rather than a shading point.
 *
 * **talyn's current space is world space**, because that is where its scene is. moya's is
 * camera space, and the pair is the thing that will confuse a reader - which is why the
 * space table is a renderer callback rather than a constant the library holds.
 *
 * **One shader run per pixel is slow and that is accepted here.** Batching the hits of a
 * scanline that share a shader is the obvious next thing; the 64 by 48 references do not
 * need it and a real image will.
 **/
class HitShader final : public v3d::render::offline::sl::runtime::Renderer {
 public:
    explicit HitShader(const Scene* scene);

    /**
     * The colour of one hit: the surface shader's Ci.
     *
     * A triangle with no shader is its own flat colour, which is what a scene built in
     * code without one asks for.
     */
    glm::vec3 shade(const Hit & hit);

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

 private:
    /**
     * A machine sized for one program and a batch of one. Kept between hits, because a
     * render is one of these per pixel and sizing a register file per pixel is the one
     * allocation that would show.
     */
    class Run final {
     public:
        v3d::render::offline::sl::runtime::Machine machine;
        const v3d::render::offline::sl::runtime::Program* program = nullptr;
    };

    Run & run(const v3d::render::offline::sl::InstancePtr & shader);

    const Scene* scene_;
    /** The hit being shaded, for the space table and for the shadow ray's offset. **/
    const Hit* hit_ = nullptr;
    /** What the shader being run was placed by, which is its own space. **/
    glm::mat4x4 placement_ = glm::mat4x4(1.0f);
    /**
     * How many traced rays deep the run is.
     *
     * A ray a shader traced may hit a surface whose shader traces again, and nothing in
     * the language stops that going round for ever. One level is what this phase needs -
     * no standard shader calls trace() at all - and phase 5 is where a depth a scene can
     * set belongs.
     */
    unsigned int depth_ = 0;
    std::map<const v3d::render::offline::sl::runtime::Program*, Run> runs_;
};

};  // namespace v3d::talyn
