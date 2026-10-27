/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Properties.h"

#include <api/type/Transform.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <vector>

#include "Camera.h"
#include "Item.h"
#include "Light.h"

#include <glm/common.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

namespace {

const char* const AXES[3] = { "x", "y", "z" };

/**
 * A whole turn, in degrees.
 **/
constexpr float TURN = 360.0F;

std::vector<std::string> placement() {
    std::vector<std::string> paths;
    for (const char* group : { "translation.", "rotation.", "scale." }) {
        for (const char* axis : AXES) {
            paths.push_back(std::string(group) + axis);
        }
    }
    return paths;
}

std::vector<std::string> with(std::vector<std::string> paths, const std::vector<std::string>& more) {
    paths.insert(paths.end(), more.begin(), more.end());
    return paths;
}

/**
 * An angle moved by whole turns to be as close to near as it can.
 **/
float closest(float angle, float near) {
    return angle + (std::round((near - angle) / TURN) * TURN);
}

/**
 * Look up one value, or keep the one given.
 **/
float either(const std::map<std::string, float>& values, const std::string& path, float otherwise) {
    const auto found = values.find(path);
    return found == values.end() ? otherwise : found->second;
}

/**
 * Whether any of the values is in a group, such as "lens.".
 **/
bool any(const std::map<std::string, float>& values, const std::string& group) {
    return std::any_of(values.begin(), values.end(),
        [&group](const auto& value) { return value.first.starts_with(group); });
}

};  // namespace

/**
 **/
const std::vector<std::string>& properties(Item::Kind kind) {
    static const std::vector<std::string> plain = placement();
    static const std::vector<std::string> camera = with(placement(),
        { "lens.fov", "lens.extent", "lens.near", "lens.far" });
    static const std::vector<std::string> light = with(placement(),
        { "emission.intensity", "emission.red", "emission.green", "emission.blue", "emission.cone", "emission.delta" });
    switch (kind) {
        case Item::Kind::Camera:
            return camera;
        case Item::Kind::Light:
            return light;
        case Item::Kind::Mesh:
        case Item::Kind::Group:
        case Item::Kind::Curve:
            break;
    }
    return plain;
}

/**
 **/
bool declares(Item::Kind kind, const std::string& path) {
    const std::vector<std::string>& paths = properties(kind);
    return std::find(paths.begin(), paths.end(), path) != paths.end();
}

/**
 **/
Pose pose(const Item& item) {
    Pose found{ .placement = item.placement() };
    if (item.kind() == Item::Kind::Camera) {
        found.lens = static_cast<const Camera&>(item).lens();
    } else if (item.kind() == Item::Kind::Light) {
        found.emission = static_cast<const Light&>(item).emission();
    }
    return found;
}

/**
 **/
std::map<std::string, float> values(Item::Kind kind, const Pose& pose, const glm::vec3& near) {
    std::map<std::string, float> found;
    const glm::vec3 rotation = eulerNear(pose.placement.rotation, near);
    for (int axis = 0; axis < 3; axis++) {
        found[std::string("translation.") + AXES[axis]] = pose.placement.position[axis];
        found[std::string("rotation.") + AXES[axis]] = rotation[axis];
        found[std::string("scale.") + AXES[axis]] = pose.placement.scale[axis];
    }
    if (kind == Item::Kind::Camera) {
        found["lens.fov"] = pose.lens.fov;
        found["lens.extent"] = pose.lens.extent;
        found["lens.near"] = pose.lens.clipping.x;
        found["lens.far"] = pose.lens.clipping.y;
    } else if (kind == Item::Kind::Light) {
        found["emission.intensity"] = pose.emission.intensity;
        found["emission.red"] = pose.emission.colour.r;
        found["emission.green"] = pose.emission.colour.g;
        found["emission.blue"] = pose.emission.colour.b;
        found["emission.cone"] = pose.emission.cone;
        found["emission.delta"] = pose.emission.delta;
    }
    return found;
}

/**
 **/
std::map<std::string, float> values(const Item& item, const glm::vec3& near) {
    return values(item.kind(), pose(item), near);
}

/**
 **/
Pose posed(Item::Kind kind, Pose pose, const std::map<std::string, float>& values) {
    // each part is set only when a value names it, so a part no value names is not touched
    v3d::type::Transform& placement = pose.placement;
    for (int axis = 0; axis < 3; axis++) {
        placement.position[axis] = either(values, std::string("translation.") + AXES[axis], placement.position[axis]);
        placement.scale[axis] = either(values, std::string("scale.") + AXES[axis], placement.scale[axis]);
    }
    if (any(values, "rotation.")) {
        glm::vec3 rotation = eulerNear(placement.rotation, glm::vec3(0.0F));
        for (int axis = 0; axis < 3; axis++) {
            rotation[axis] = either(values, std::string("rotation.") + AXES[axis], rotation[axis]);
        }
        placement.rotation = fromEuler(rotation);
    }
    if (kind == Item::Kind::Camera) {
        Camera::Lens& lens = pose.lens;
        lens.fov = either(values, "lens.fov", lens.fov);
        lens.extent = either(values, "lens.extent", lens.extent);
        lens.clipping.x = either(values, "lens.near", lens.clipping.x);
        lens.clipping.y = either(values, "lens.far", lens.clipping.y);
    } else if (kind == Item::Kind::Light) {
        Light::Emission& emission = pose.emission;
        emission.intensity = either(values, "emission.intensity", emission.intensity);
        emission.colour.r = either(values, "emission.red", emission.colour.r);
        emission.colour.g = either(values, "emission.green", emission.colour.g);
        emission.colour.b = either(values, "emission.blue", emission.colour.b);
        emission.cone = either(values, "emission.cone", emission.cone);
        emission.delta = either(values, "emission.delta", emission.delta);
    }
    return pose;
}

/**
 **/
glm::quat fromEuler(const glm::vec3& degrees) {
    const glm::vec3 radians = glm::radians(degrees);
    return glm::normalize(glm::angleAxis(radians.z, glm::vec3(0.0F, 0.0F, 1.0F)) *
        glm::angleAxis(radians.y, glm::vec3(0.0F, 1.0F, 0.0F)) *
        glm::angleAxis(radians.x, glm::vec3(1.0F, 0.0F, 0.0F)));
}

/**
 **/
glm::vec3 eulerNear(const glm::quat& rotation, const glm::vec3& near) {
    // the matrix is z times y times x, read by column. The y angle comes from the sine in the
    // third row of the first column, and the other two from the rest of that row and column
    const glm::mat3 matrix = glm::mat3_cast(glm::normalize(rotation));
    const float sine = glm::clamp(-matrix[0][2], -1.0F, 1.0F);
    const float y = std::asin(sine);
    float x = 0.0F;
    float z = 0.0F;
    // close to a quarter turn about y, x and z turn about the same axis, so z is taken as 0
    constexpr float locked = 0.99999F;
    if (std::fabs(sine) < locked) {
        x = std::atan2(matrix[1][2], matrix[2][2]);
        z = std::atan2(matrix[0][1], matrix[0][0]);
    } else {
        x = std::atan2(-matrix[2][1], matrix[1][1]);
    }
    const glm::vec3 first = glm::degrees(glm::vec3(x, y, z));
    // the same rotation is given by half a turn more about x and z, with y mirrored
    const glm::vec3 second(first.x + (TURN / 2.0F), (TURN / 2.0F) - first.y, first.z + (TURN / 2.0F));
    const auto nearest = [&near](const glm::vec3& angles) {
        return glm::vec3(closest(angles.x, near.x), closest(angles.y, near.y), closest(angles.z, near.z));
    };
    const glm::vec3 one = nearest(first);
    const glm::vec3 two = nearest(second);
    const auto distance = [&near](const glm::vec3& angles) {
        const glm::vec3 apart = glm::abs(angles - near);
        return apart.x + apart.y + apart.z;
    };
    return distance(two) < distance(one) ? two : one;
}

};  // namespace v3d::editor
