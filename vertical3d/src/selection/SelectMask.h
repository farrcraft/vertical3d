/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Item.h>

#include <cstdint>
#include <string>

namespace v3d::editor {

/**
 * What a click selects.
 *
 * Exactly one is in force at a time. Object, Mesh, Light and Camera select whole items:
 * Object takes every kind, and each of the others takes only its own. Vertex, Edge and Face
 * select the components of meshes. Selection state lives on the item and on each of a mesh's
 * vertices, half edges and faces, and the mask is the only thing stopping two kinds of
 * component being selected at once.
 **/
enum class SelectMask : std::uint8_t {
    Object,
    Vertex,
    Edge,
    Face,
    Mesh,
    Light,
    Camera
};

/**
 * @return whether the mask selects whole items rather than the components of a mesh
 **/
bool selectsItems(SelectMask mask);

/**
 * @return whether the mask selects whole items of this kind
 **/
bool takes(SelectMask mask, Item::Kind kind);

/**
 * @param name one of "object", "vertex", "edge", "face", "mesh", "light" or "camera"
 * @param mask set to the mask named, and untouched if the name is not one of them
 * @return whether the name named a mask
 **/
bool selectMask(const std::string& name, SelectMask* mask);

/**
 * @return the name selectMask() accepts for this mask
 **/
const char* selectMaskName(SelectMask mask);

};  // namespace v3d::editor
