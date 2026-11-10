/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/Tree.h>
#include <moya/libmoya/light/internal/Tree.h>

#include <cstdint>
#include <vector>

namespace v3d::moya::light::internal {

/**
 * What the lights are gathered into before they are copied to a device: each light with its
 * power, each mesh light's triangle chances, and the first mesh light each instance in the world
 * places. top holds the top tree's leaves, nodes every subtree's nodes, and triangleLeaves the
 * leaf of each triangle of each subtree.
 **/
struct Gathered {
    std::vector<kernel::light::Light> lights;
    std::vector<float> powers;
    std::vector<float> triangles;
    std::vector<std::uint32_t> meshes;
    std::vector<Leaf> top;
    std::vector<kernel::light::TreeNode> nodes;
    std::vector<std::uint32_t> triangleLeaves;
};

};  // namespace v3d::moya::light::internal
