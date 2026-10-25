/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <map>
#include <string>
#include <vector>

#include "Item.h"

#include <glm/ext/quaternion_float.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 * The numbers of an item that a curve can drive, each named by a path.
 *
 * Every kind has translation.x, .y and .z, rotation.x, .y and .z, and scale.x, .y and .z. A
 * rotation is three Euler angles in degrees, turned about x first, then y, then z. A camera adds
 * lens.fov, lens.extent, lens.near and lens.far. A light adds emission.intensity, emission.red,
 * emission.green, emission.blue, emission.cone and emission.delta.
 *
 * @return the paths of one kind, in that order
 **/
const std::vector<std::string>& properties(Item::Kind kind);

/**
 * @return whether the kind declares the path
 **/
bool declares(Item::Kind kind, const std::string& path);

/**
 * The value of every property an item declares, as it stands.
 *
 * @param near the rotation the Euler angles are taken closest to, in degrees. An item's
 *        rotation is a quaternion, which many sets of angles give
 **/
std::map<std::string, float> values(const Item& item, const glm::vec3& near);

/**
 * Set some of an item's properties. A property that is not named keeps its value, and a path
 * the item does not declare is ignored. A rotation is set whole, from its named angles and the
 * angles closest to zero for the rest.
 **/
void pose(Item* item, const std::map<std::string, float>& values);

/**
 * The rotation of three Euler angles in degrees, turned about x, then y, then z.
 **/
glm::quat fromEuler(const glm::vec3& degrees);

/**
 * The Euler angles of a rotation, in degrees, of the two sets of angles that give it and every
 * whole turn of each angle, the set closest to near.
 **/
glm::vec3 eulerNear(const glm::quat& rotation, const glm::vec3& near);

};  // namespace v3d::editor
