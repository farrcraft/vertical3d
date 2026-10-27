/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Transform.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::interchange {

/**
 * A polygon mesh in its node's space: a list of points, and each face as a run of indices into
 * it. counts holds how many corners each face has, and indices holds every face's corners one
 * face after another. A face runs counter-clockwise seen from outside.
 **/
struct Mesh {
    std::string name;
    std::vector<glm::vec3> points;
    std::vector<std::uint32_t> counts;
    std::vector<std::uint32_t> indices;
    /**
     * Whether the format stores triangles only, so that a quad the author made arrives cut in
     * two.
     **/
    bool triangulated = false;
};

/**
 * A camera. It looks along its node's -z axis, with +y up.
 **/
struct Camera {
    enum class Projection : std::uint8_t { Perspective, Orthographic };

    std::string name;
    Projection projection = Projection::Perspective;
    /**
     * A perspective camera's full angle of view from the bottom of the picture to the top, in
     * radians.
     **/
    float yfov = 0.0F;
    /**
     * The picture's width over its height, or zero when the file does not say.
     **/
    float aspect = 0.0F;
    /**
     * Half the width and half the height of what an orthographic camera sees, in its node's
     * units.
     **/
    glm::vec2 magnification{ 0.0F };
    /**
     * The clipping distances along the view direction. far is zero when the camera has no far
     * plane.
     **/
    float near = 0.0F;
    float far = 0.0F;
};

/**
 * A light. A distant or spot light shines along its node's -z axis.
 **/
struct Light {
    enum class Type : std::uint8_t { Point, Distant, Spot };

    std::string name;
    Type type = Type::Point;
    glm::vec3 colour{ 1.0F };
    /**
     * The intensity in the file's own units: candela for a point or spot light in glTF, lux for
     * a distant one, and USD's unitless intensity scaled by its exposure.
     **/
    float intensity = 1.0F;
    /**
     * A spot light's half angles: where its light ends, and where it starts to fade toward
     * that edge, in radians.
     **/
    float outerCone = 0.0F;
    float innerCone = 0.0F;
};

/**
 * One node of the hierarchy. Its placement is in its parent's space, or the scene's when it has
 * no parent. It may carry a mesh, a camera and a light, each by its index in the scene, and a
 * node with none of them only groups its children.
 **/
struct Node {
    std::string name;
    /**
     * Always a node earlier in the list, so a parent is met before its children.
     **/
    std::optional<std::size_t> parent;
    v3d::type::Transform placement;
    std::optional<std::size_t> mesh;
    std::optional<std::size_t> camera;
    std::optional<std::size_t> light;
};

/**
 * The keyed values of one part of one node's placement.
 *
 * A value is a vec4 whatever the part: x, y and z for a translation or a scale, and a
 * quaternion's x, y, z and w for a rotation. Cubic spline keys also carry a tangent on each
 * side, in value per second.
 **/
struct Channel {
    enum class Path : std::uint8_t { Translation, Rotation, Scale };
    enum class Interpolation : std::uint8_t { Step, Linear, CubicSpline };

    std::size_t node = 0;
    Path path = Path::Translation;
    Interpolation interpolation = Interpolation::Linear;
    /**
     * The key times in seconds, rising.
     **/
    std::vector<float> times;
    std::vector<glm::vec4> values;
    std::vector<glm::vec4> inTangents;
    std::vector<glm::vec4> outTangents;
};

/**
 * A scene read from a file, in no format's terms. Up is +y.
 **/
struct Scene {
    std::vector<Node> nodes;
    std::vector<Mesh> meshes;
    std::vector<Camera> cameras;
    std::vector<Light> lights;
    std::vector<Channel> channels;
};

/**
 * What a reader made of a file.
 *
 * scene is empty when the file could not be read, and problem then says why. notes lists what
 * the file held that the scene cannot describe and was left out, such as a skin or a mesh of
 * lines, one line each.
 **/
struct Imported {
    std::optional<Scene> scene;
    std::string problem;
    std::vector<std::string> notes;
};

};  // namespace v3d::interchange
