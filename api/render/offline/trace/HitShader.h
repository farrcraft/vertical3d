/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Texture.h>
#include <api/render/offline/sl/Placed.h>
#include <api/render/offline/sl/runtime/Renderer.h>

#include <string>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Hit.h"

namespace v3d::render::offline::trace {

class Tracer;

/**
 * Runs a surface shader over one hit, which is a batch one point wide, and implements the
 * renderer callbacks the shader makes while it runs.
 *
 * There is no special case and no second path: the hit runs the same program and the same
 * instructions that run over a grid of a hundred in moya, with a mask one bit wide.
 *
 * One is made on the stack for each hit the Tracer shades. A hit that a shader traces into
 * is shaded by another, so nothing this one holds changes under it.
 *
 * **A hit's current space is world space**, because that is where the scene is. moya's
 * grids shade in camera space. The two differ, so the space table is a renderer callback
 * rather than a constant the library holds.
 **/
class HitShader final : public v3d::render::offline::sl::runtime::Renderer {
 public:
    HitShader(Tracer* tracer, const Hit & hit);

    HitShader(const HitShader &) = delete;
    HitShader & operator=(const HitShader &) = delete;

    /**
     * @return the surface shader's Ci, and its Oi through opacity - the primitive's own
     *         colour and opacity when it has no shader or its shader fails
     **/
    glm::vec3 shade(glm::vec3* opacity);

    // the runtime::Renderer callbacks
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
    /** The lights on the hit: its primitive's, or the scene's. **/
    const std::vector<v3d::render::offline::sl::Placed> & shining() const;

    Tracer* tracer_;
    const Hit & hit_;
    /**
     * The placement of the shader being run, which maps its own space into world space: the
     * surface's, and a light's while that light runs.
     **/
    glm::mat4x4 placement_;
};

};  // namespace v3d::render::offline::trace
