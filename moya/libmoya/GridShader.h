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

#include "MicroPolygonGrid.h"
#include "Shading.h"

namespace v3d::moya {

class RenderContext;

/**
 * Runs a surface shader over a micropolygon grid, which is the batch of ADR-0026 in the
 * renderer it was designed for.
 *
 * **The shading points are the grid's vertices**, every one of them at once - a grid of
 * n by n is a batch of n squared, and one run answers all of it. That is the whole reason
 * the execution model is a batch rather than a shading point.
 *
 * **moya's current space is camera space.** That is what its first pass already works in
 * and what the space table hands the machine for "current". talyn's is world space, which
 * is why the table is a renderer callback rather than a constant.
 *
 * One of these lives for a render rather than for a grid: `prepare` sizes a register file
 * and a grid of the same size over the same program reuses it, which is the difference
 * between shading a thousand grids and allocating a thousand times.
 **/
class GridShader final : public v3d::render::offline::sl::runtime::Renderer {
 public:
    explicit GridShader(RenderContext* context);

    /**
     * Shade every vertex of the grid, leaving `Ci` on each as its colour.
     *
     * `Cs` is read off the vertex before it is written back over, which is what makes a
     * primitive's own varying "Cs" reach the shader.
     */
    void shade(const Shading & shading, MicroPolygonGrid* grid);

    // what the machine asks a renderer for
    bool space(const std::string & name, glm::mat4x4* matrix) override;
    unsigned int lights() override;
    bool light(unsigned int index, const v3d::render::offline::sl::runtime::Value & surface,
        v3d::render::offline::sl::runtime::Value* direction,
        v3d::render::offline::sl::runtime::Value* colour,
        std::vector<char>* reached, bool* ambient) override;

 private:
    /**
     * A machine sized for one program and one batch. A light's program is run once per
     * grid and a surface's once per grid, so both are worth keeping between grids.
     */
    class Run final {
     public:
        v3d::render::offline::sl::runtime::Machine machine;
        const v3d::render::offline::sl::runtime::Program* program = nullptr;
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
    unsigned int batch_ = 1;
    std::map<const v3d::render::offline::sl::runtime::Program*, Run> runs_;
};

};  // namespace v3d::moya
