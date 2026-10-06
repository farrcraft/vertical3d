/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Texture.h>

#include <string>
#include <vector>

#include <glm/mat4x4.hpp>

#include "Value.h"

namespace v3d::render::offline::sl::runtime {

/**
 * What the machine needs from a renderer, and does not hold itself.
 *
 * A shader run has no access to buckets, grids, rays or scenes. What it does need arrives
 * through this interface: the matrix for a named coordinate space, the lights shining on the
 * batch, whether light reaches a point, and a traced ray. moya's grids and a traced hit each
 * implement it.
 *
 * **Every method has a default for a renderer that cannot do it**, so a renderer, or a
 * test's stand-in for one, implements only what it supports. Only the coordinate space is
 * required, because a renderer that cannot say where it is shading has nothing to shade.
 **/
class Renderer {
 public:
    virtual ~Renderer() = default;

    /**
     * The matrix from the shader's current space into the named one, as RenderMan defines it.
     *
     * Every space follows that one direction, so a point in current space times the matrix is
     * the same point in the named space:
     *
     * - "current" is the identity.
     * - "shader" is the inverse of the shader's placement. The placement maps the shader's own
     *   space into current space, as the transformation in force when the scene instanced the
     *   shader running now. While a light runs, it is the light's placement.
     * - "object" is the inverse of the primitive's placement, which maps its object space into
     *   current space.
     * - "world" and "camera" are the scene's world and camera spaces.
     * - "screen" is the projection, with the visible picture over [-1, 1] in x and y.
     * - "raster" is pixels: x to the right and y down from the upper left corner of the
     *   picture.
     * - "NDC" is raster divided by the resolution: x to the right and y down, over [0, 1]
     *   from the upper left corner. Its z is what depth() returns.
     *
     * transform("space", P) applies this matrix. A cast such as `point "space" (x, y, z)` states
     * a value in the named space, so it applies the inverse.
     *
     * moya's grids are shaded in camera space and a traced hit in world space, which is why this is
     * a callback rather than a table the library holds.
     *
     * @param name the space: "current", "object", "shader", "world", "camera", "raster", ...
     * @param matrix where to put it, if the renderer recognises the space
     * @return whether it did; a space the renderer does not recognise leaves the value alone
     **/
    virtual bool space(const std::string & name, glm::mat4x4* matrix) = 0;

    /**
     * How many light sources are shining on the batch being shaded.
     **/
    virtual unsigned int lights();

    /**
     * Runs one light over the batch, for the surface shader's side of the message passing
     * between surface and light shaders.
     *
     * Running a light means running its own program over the same batch. That is the
     * renderer's job rather than the machine's, because the renderer holds the shader
     * instances a scene named.
     *
     * @param index which of lights()
     * @param surface where each point being lit is, which is the light shader's Ps
     * @param direction where L lands: **from the surface point toward the light**
     * @param colour where Cl lands
     * @param reached which points the light gets to at all
     * @param ambient whether the light used neither illuminate nor solar, so that it is left
     *        out of an illuminance loop and summed by ambient()
     * @return whether the renderer ran it; a light it could not run lights nothing
     **/
    virtual bool light(unsigned int index, const Value & surface, Value* direction,
        Value* colour, std::vector<char>* reached, bool* ambient);

    /**
     * How much of the light leaving one point arrives at the other, per component.
     *
     * This is how a shadow is cast. moya computes it with the shared ray tracer, for a grid
     * and for a traced hit alike. A ray tracing extension rather than part of RI 3.03.
     *
     * @return whether the renderer computed it; when it did not, all the light gets through
     **/
    virtual bool transmission(const Value & from, const Value & to, Value* fraction);

    /**
     * The colour a ray from a point in a direction returns. moya traces it with the shared
     * ray tracer, for a grid and for a traced hit alike.
     *
     * @return whether the renderer traced it; when it did not, the result is black and the
     *         machine reports it
     **/
    virtual bool trace(const Value & origin, const Value & direction, Value* colour);

    /**
     * The texture a shader names, which the renderer holds for the frame so that each is
     * read once however many batches use it.
     *
     * @return null when the name cannot be read, or when the renderer holds no textures;
     *         the machine then returns black and reports it
     **/
    virtual const Texture* texture(const std::string & name);
};

};  // namespace v3d::render::offline::sl::runtime
