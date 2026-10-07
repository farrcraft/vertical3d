/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Texture.h>
#include <api/render/offline/sl/Globals.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/render/offline/sl/runtime/Renderer.h>
#include <api/render/offline/trace/Tracer.h>

#include <map>
#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include "MicroPolygonGrid.h"
#include "Shading.h"

namespace v3d::moya {

class RenderContext;

/**
 * Runs a surface shader over a micropolygon grid as one batch of shading points.
 *
 * **The shading points are the grid's vertices**, all shaded in one run: a grid of n by n is
 * a batch of n squared.
 *
 * **moya's current space is camera space.** The reyes hider works in camera space, and the
 * space table returns the identity for "current". A traced hit's current space is world space,
 * so the table is a renderer callback rather than a constant.
 *
 * One of these lives for a render rather than for a grid. `prepare` sizes a register file,
 * and a later grid of the same size over the same program reuses it rather than allocating
 * again.
 **/
class GridShader final : public v3d::render::offline::sl::runtime::Renderer {
 public:
    explicit GridShader(RenderContext* context);

    /**
     * Shade every vertex of the grid, leaving `Ci` on each as its colour.
     *
     * `Cs` is read off the vertex before it is written back over, so a primitive's own varying
     * "Cs" reaches the shader.
     */
    void shade(const Shading & shading, MicroPolygonGrid* grid);

    // the callbacks the shading machine calls on its renderer
    bool space(const std::string & name, glm::mat4x4* matrix) override;
    unsigned int lights() override;
    bool light(unsigned int index, const v3d::render::offline::sl::runtime::Value & surface,
        v3d::render::offline::sl::runtime::Value* direction,
        v3d::render::offline::sl::runtime::Value* colour,
        std::vector<char>* reached, bool* ambient) override;
    const v3d::render::offline::Texture* texture(const std::string & name) override;
    /**
     * Both trace through the context's traced scene, in world space, at the shutter's open. A
     * grid is shaded once for all of its samples, so it has no single time of its own.
     **/
    bool transmission(const v3d::render::offline::sl::runtime::Value & from,
        const v3d::render::offline::sl::runtime::Value & to,
        v3d::render::offline::sl::runtime::Value* fraction) override;
    bool trace(const v3d::render::offline::sl::runtime::Value & origin,
        const v3d::render::offline::sl::runtime::Value & direction,
        v3d::render::offline::sl::runtime::Value* colour) override;

 private:
    /**
     * A machine sized for one program and one batch. A light's program is run once per
     * grid and a surface's once per grid, so both are worth keeping between grids.
     */
    class Run final {
     public:
        v3d::render::offline::sl::runtime::Machine machine;
        const v3d::render::offline::sl::runtime::Program* program = nullptr;
        v3d::render::offline::sl::Globals globals;
        unsigned int batch = 0;
    };

    /**
     * The machine for a program at this batch, prepared if it was not already. Keyed on
     * the program rather than on the shader instance, since two instances of one shader
     * differ only in what is written into the register file.
     */
    Run & run(const v3d::render::offline::sl::InstancePtr & shader, unsigned int batch);

    RenderContext* context_;
    /** What is being shaded, for the space table and for the lights. **/
    const Shading* shading_ = nullptr;
    /**
     * The shader being run's own space into camera space: the surface's, and a light's while
     * that light runs, so a light's `point "shader" (0, 0, 0)` lands where the scene placed it.
     **/
    glm::mat4x4 placement_ = glm::mat4x4(1.0f);
    unsigned int batch_ = 1;
    std::map<const v3d::render::offline::sl::runtime::Program*, Run> runs_;
    /** What casts the rays, into the context's traced scene. **/
    v3d::render::offline::trace::Tracer tracer_;
    /** Camera space to world space, and each shading point's Ng in world space. **/
    glm::mat4x4 toWorld_ = glm::mat4x4(1.0f);
    std::vector<glm::vec3> planes_;
};

};  // namespace v3d::moya
