/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <vector>

#include "Camera.h"
#include "Channel.h"
#include "Curves.h"
#include "Light.h"
#include "Mesh.h"
#include "Node.h"

/**
 * Scenes read from glTF in no format's terms: nodes, polygon meshes, cameras, lights, curves and
 * keys.
 **/
namespace v3d::interchange {

/**
 * A scene read from a file, in no format's terms. Up is +y.
 **/
struct Scene {
    std::vector<Node> nodes;
    std::vector<Mesh> meshes;
    std::vector<Camera> cameras;
    std::vector<Light> lights;
    std::vector<Curves> curves;
    std::vector<Channel> channels;
};

};  // namespace v3d::interchange
