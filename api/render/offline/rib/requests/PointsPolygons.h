/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <vector>

namespace v3d::render::offline::rib::requests {

/**
 * RiPointsPolygons: the faces of a mesh, as indices into its "P" array.
 **/
struct PointsPolygons {
    std::vector<unsigned int> counts;   /**< how many vertices each polygon has **/
    std::vector<unsigned int> indices;  /**< the vertices of every polygon in turn **/
};

};  // namespace v3d::render::offline::rib::requests
