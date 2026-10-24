/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Area.h>

#include "Instance.h"

#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/mat4x4.hpp>

namespace v3d::render::offline::sl {

/**
 * A shader instance and the space it was instanced in.
 *
 * RI defines a shader's own space as the transform that was in force when a scene named
 * it. Its `point "shader" (0, 0, 1)` and every position a scene binds are stated in that
 * space. A surface on a primitive and a light in a scene each need both, so they are kept
 * together.
 **/
class Placed final {
 public:
    InstancePtr shader;
    glm::mat4x4 placement = glm::mat4x4(1.0F);
    /**
     * For an area light, the geometry it shines from, in world space. Null for every other
     * light and for a surface.
     **/
    boost::shared_ptr<const v3d::render::offline::Area> area;
};

/**
 * How many lights a list shines as. A light is one, and an area light is one for each of its
 * samples, so an illuminance loop sees each sample as a light of its own.
 **/
unsigned int shining(const std::vector<Placed> & lights);

/**
 * The light a renderer's light index names, and which of its samples, or null past the end.
 * The index counts as shining() does.
 **/
const Placed* which(const std::vector<Placed> & lights, unsigned int index, unsigned int* sample);

};  // namespace v3d::render::offline::sl
