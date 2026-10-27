/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Transform.h>

#include <map>
#include <string>
#include <vector>

#include "Camera.h"
#include "Item.h"
#include "Light.h"

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
 * The values of one item that a curve can drive: its placement, and a camera's lens or a
 * light's emission. A kind that has no lens or no emission leaves that one at its default.
 **/
struct Pose {
    v3d::type::Transform placement;
    Camera::Lens lens;
    Light::Emission emission;
};

/**
 * @return the item's values as it holds them
 **/
Pose pose(const Item& item);

/**
 * The value of every property a kind declares, in a pose.
 *
 * @param near the rotation the Euler angles are taken closest to, in degrees. A rotation is a
 *        quaternion, which many sets of angles give
 **/
std::map<std::string, float> values(Item::Kind kind, const Pose& pose, const glm::vec3& near);

/**
 * The value of every property an item declares, as the item holds them.
 **/
std::map<std::string, float> values(const Item& item, const glm::vec3& near);

/**
 * A pose with some of its properties set. A property that is not named keeps its value, and a
 * path the kind does not declare is ignored. A rotation is set whole, from its named angles and
 * the angles closest to zero for the rest.
 **/
Pose posed(Item::Kind kind, Pose pose, const std::map<std::string, float>& values);

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
