/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/interchange/Scene.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "Item.h"
#include "Scene.h"

namespace v3d::editor {

/**
 * The items a file's scene becomes, with their hierarchy and their curves, ready to be put into
 * a scene.
 *
 * items, parents and curves run in step. An item's parent is an index into items, always an
 * earlier one, or none for an item at the top.
 **/
struct Import final {
    Items items;
    std::vector<std::optional<std::size_t>> parents;
    std::vector<Curves> curves;
    /**
     * What the file held that the scene cannot, or what could not be made, one line each.
     **/
    std::vector<std::string> notes;
};

/**
 * Make items from an interchange scene.
 *
 * Each node becomes one item: a mesh, a camera or a light when it carries one, in that order,
 * and a group when it carries none. A node that carries more than one becomes the first, with
 * the others as its children. A mesh is built with brep::build(), joining triangles back into
 * quads when the format stores only triangles. Nodes that share a mesh give items that share
 * one geometry. A mesh that cannot be built is noted, and its node becomes a group.
 *
 * A file's camera or light points along -z, and the editor's along +z, so each one is turned
 * half a turn about its own y. Its children and its keys are turned back, so they stay where
 * the file put them.
 *
 * A key at a time in seconds is put on the frame at that time at the rate given. A rotation's
 * keys become Euler angles, each set the one closest to the key before. Linear and step keys
 * keep their interpolation. Cubic spline keys become Bezier keys, with the file's tangents as
 * slopes for a translation or a scale, and automatic slopes for a rotation.
 *
 * @param rate the frames a second of the timeline the keys go on
 **/
Import importScene(const v3d::interchange::Scene& scene, float rate);

};  // namespace v3d::editor
