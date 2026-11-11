/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/Tree.h>
#include <moya/libmoya/light/internal/Tree.h>
#include <moya/libmoya/scene/Handle.h>

#include <cstdint>
#include <vector>

namespace v3d::moya::light::internal {

/**
 * What the lights are gathered into before they are copied to a device. It holds each light
 * with its power and the handle it was made from, each mesh light's triangle chances, and the
 * first mesh light each instance in the world places. infinite holds the places of the distant
 * and dome lights. top holds the leaves of the lights with a place, nodes every tree's nodes, and
 * triangleLeaves the leaf of each triangle of each subtree.
 *
 * Each set of linked lights has its tree's root in roots, its lights' leaves in leaves from the
 * set times the count of lights, and its distant and dome lights in afar from afarFirst[set].
 **/
struct Gathered {
    std::vector<kernel::light::Light> lights;
    std::vector<float> powers;
    std::vector<scene::Handle> handles;
    std::vector<float> triangles;
    std::vector<std::uint32_t> meshes;
    std::vector<Leaf> top;
    std::vector<kernel::light::TreeNode> nodes;
    std::vector<std::uint32_t> triangleLeaves;
    std::vector<std::uint32_t> infinite;
    std::vector<std::uint32_t> roots;
    std::vector<std::uint32_t> leaves;
    std::vector<std::uint32_t> afar;
    std::vector<std::uint32_t> afarFirst;
};

};  // namespace v3d::moya::light::internal
