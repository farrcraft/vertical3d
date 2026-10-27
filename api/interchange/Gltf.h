/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "Scene.h"

namespace v3d::interchange {

/**
 * Read a glTF 2.0 file, .gltf or .glb, as a scene.
 *
 * The scene read is the one the file names as its default, or its first, or every root node
 * when it has none. Each node becomes a node of the scene, parents first, keeping its name, its
 * placement and its mesh, camera and light (KHR_lights_punctual). A mesh that two nodes use is
 * read once, and both nodes name it. A node placed by a matrix is placed by that matrix taken
 * apart into a translation, a rotation and a scale.
 *
 * A mesh's primitives are joined into one mesh of triangles. Strips and fans are cut into
 * triangles, and a primitive with no indices takes its points in order. Every channel of every
 * animation that moves, turns or scales a node is read, so a file whose animations each move
 * one node plays as one scene.
 *
 * Left out, each with a note: primitives of points or lines, skins and morph targets, and a
 * second channel for a part of a node that an earlier channel already keys.
 *
 * @return the scene, or a problem when the file cannot be read, parsed or validated
 **/
Imported readGltf(const std::string& path);

};  // namespace v3d::interchange
