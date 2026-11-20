/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/type/Transform.h>

#include <cstddef>
#include <optional>
#include <string>

namespace v3d::interchange {

/**
 * One node of the hierarchy. Its placement is in its parent's space, or the scene's when it has
 * no parent. It may carry a mesh, a camera, a light or curves, each by its index in the scene,
 * and a node with none of them only groups its children.
 *
 * A node is seen and rendered unless its file says otherwise. A node that is not seen is not
 * rendered either. A node that is seen and not rendered is a guide or a stand-in for something
 * else. What a node's flags hide, they hide beneath it too.
 **/
struct Node {
    std::string name;
    bool visible = true;   /**< whether the node is seen at all **/
    bool renders = true;   /**< whether a render draws it **/
    /**
     * Always a node earlier in the list, so a parent is met before its children.
     **/
    std::optional<std::size_t> parent;
    v3d::type::Transform placement;
    std::optional<std::size_t> mesh;
    std::optional<std::size_t> camera;
    std::optional<std::size_t> light;
    std::optional<std::size_t> curves;
};

};  // namespace v3d::interchange
