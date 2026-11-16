/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::rib::requests {

/**
 * RenderMan's Pattern and Bxdf: a node of a shading network, of a type, and the handle a later
 * node's reference parameter names it by.
 **/
struct ShadingNode {
    std::string type;    /**< the node's type, such as a MaterialX node definition **/
    std::string handle;  /**< what a reference names this node by **/
};

};  // namespace v3d::rib::requests
