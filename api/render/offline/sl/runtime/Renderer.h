/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include "Value.h"

namespace v3d::render::offline::sl::runtime {

/**
 * What the machine needs from a renderer, and does not hold itself.
 *
 * A shader run knows nothing about buckets, grids, rays or scenes. What it does need - the
 * matrix for a named coordinate space, the lights shining on the batch, whether light
 * reaches a point, and a ray traced - arrives through this, which moya and talyn each
 * implement.
 *
 * **Every method has an answer for a renderer that cannot do it**, because the two
 * renderers genuinely disagree: talyn traces a shadow ray and moya answers that light gets
 * through until it has a shadow map. Only the coordinate space is required, because a
 * renderer that cannot say where it is shading has nothing to shade.
 **/
class Renderer {
 public:
    virtual ~Renderer() = default;

    /**
     * The matrix from the shader's current space into the named one.
     *
     * moya's current space is camera space and talyn's is world space, which is why this is
     * a callback rather than a table the library holds.
     *
     * @param name the space: "current", "object", "shader", "world", "camera", "raster", ...
     * @param matrix where to put it, if the renderer knows the space
     * @return whether it did; a space a renderer does not know leaves the value alone
     **/
    virtual bool space(const std::string & name, glm::mat4x4* matrix) = 0;

    /**
     * How many light sources are shining on the batch being shaded.
     **/
    virtual unsigned int lights();

    /**
     * What one light does to the batch: the message passing of ADR-0026, from the side the
     * surface shader is on.
     *
     * Running one means running that light's own program over the same batch, which is the
     * renderer's job rather than the machine's - the renderer is what holds the shader
     * instances a scene named.
     *
     * @param index which of lights()
     * @param surface where each point being lit is, which is the light shader's Ps
     * @param direction where L lands: **from the surface point toward the light**
     * @param colour where Cl lands
     * @param reached which points the light gets to at all
     * @param ambient whether the light used neither illuminate nor solar, which is what
     *        keeps it out of an illuminance loop and inside ambient()
     * @return whether the renderer ran it; a light it could not run lights nothing
     **/
    virtual bool light(unsigned int index, const Value & surface, Value* direction,
        Value* colour, std::vector<char>* reached, bool* ambient);

    /**
     * How much of the light leaving one point arrives at the other, per component.
     *
     * This is where a shadow lives, and it is the one thing the two renderers genuinely
     * disagree about: talyn answers by tracing and moya answers that all of it gets through
     * until it has a shadow map. A ray tracing extension rather than RI 3.03.
     *
     * @return whether the renderer answered; one that did not lets all the light through
     **/
    virtual bool transmission(const Value & from, const Value & to, Value* fraction);

    /**
     * What a ray from a point in a direction comes back with - the phase 6 hook.
     *
     * talyn implements it and moya answers with its background. Its existence is what makes
     * phase 6 a question anyone can answer; nothing here decides whether moya's raytracing
     * is talyn.
     *
     * @return whether the renderer traced it; one that did not answers black and says so
     **/
    virtual bool trace(const Value & origin, const Value & direction, Value* colour);
};

};  // namespace v3d::render::offline::sl::runtime
