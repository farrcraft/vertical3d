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
 * A hit takes no separate path. It runs the same program and the same instructions that moya
 * runs over a grid, with a mask one bit wide.
 *
 * The Tracer makes one on the stack for each hit it shades. A hit reached by the shader's
 * trace() is shaded by its own HitShader, so this one's state is unchanged when trace()
 * returns.
 *
 * **A hit's current space is world space**, because the scene is held in world space. moya's
 * grids shade in camera space. Because the two differ, each renderer answers space() itself,
 * and the library holds no fixed table of spaces.
 **/
class HitShader final : public v3d::render::offline::sl::runtime::Renderer {
 public:
    HitShader(Tracer* tracer, const Hit & hit);

    HitShader(const HitShader &) = delete;
    HitShader & operator=(const HitShader &) = delete;

    /**
     * @return the surface shader's Ci, with its Oi written through opacity. When the
     *         primitive has no shader or the shader fails, the primitive's colour times
     *         its opacity, and its opacity
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
