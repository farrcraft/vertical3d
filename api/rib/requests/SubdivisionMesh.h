/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>
#include <vector>

namespace v3d::rib::requests {

/**
 * RiSubdivisionMesh: a mesh's faces, as PointsPolygons gives them, refined by a scheme, with tags
 * that mark creases, corners, holes and the boundary rule.
 *
 * Each tag takes a count of integers and of floats from the arrays after it, in order, and RIB
 * after RI 3.2 adds a count of strings. arguments holds those counts, two or three per tag.
 **/
struct SubdivisionMesh {
    std::string scheme;
    std::vector<unsigned int> counts;   /**< how many vertices each face has **/
    std::vector<unsigned int> indices;  /**< the vertices of every face in turn **/
    std::vector<std::string> tags;
    std::vector<unsigned int> arguments;
    std::vector<unsigned int> integers;
    std::vector<float> floats;
    std::vector<std::string> strings;
};

};  // namespace v3d::rib::requests
