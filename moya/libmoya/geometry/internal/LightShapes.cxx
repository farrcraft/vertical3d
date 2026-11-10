/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/internal/LightShapes.h>

#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Motion.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/primitive/Quadric.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Samples.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::geometry::internal {

namespace {

/** The kind of a light, by its name. **/
std::string kindOf(const scene::Scene & scene, const scene::Handle & handle) {
    return scene.schema().kind(handle.kind).name;
}

/** One sample of values from first, count of them, which holds at every time. **/
kernel::geometry::Sampled still(std::size_t first, std::size_t count) {
    kernel::geometry::Sampled out;
    out.first = static_cast<std::uint32_t>(first);
    out.count = static_cast<std::uint32_t>(count);
    return out;
}

};  // namespace

std::vector<scene::Handle> shapedLights(const scene::Scene & scene) {
    std::vector<scene::Handle> out;
    for (const char * kind : { "rectLight", "diskLight", "sphereLight", "cylinderLight" }) {
        const std::vector<scene::Handle> found = scene.objects(kind);
        out.insert(out.end(), found.begin(), found.end());
    }
    std::sort(out.begin(), out.end(), [](const scene::Handle & left, const scene::Handle & right) { return left.index < right.index; });
    return out;
}

Instance lightInstance(const scene::Scene & scene, const scene::Handle & handle) {
    const bool cylinder = kindOf(scene, handle) == "cylinderLight";
    const glm::mat4 turn = cylinder ? glm::rotate(glm::mat4(1.0F), glm::radians(90.0F), glm::vec3 { 0.0F, 1.0F, 0.0F }) : glm::mat4(1.0F);
    const glm::mat3 turned { turn };
    Instance out;
    out.id = handle.index;
    out.kind = Prototype::Geometry;
    const scene::Samples & transform = scene.samples(handle, "transform");
    out.transform = transform.values.front().matrixAt(0) * turn;
    if (transform.values.size() > 1) {
        out.keys = keys(transform);
        // a key's transformation is T R S, so T R S turn is T (R turn) (turn^T S turn)
        for (kernel::geometry::Key & key : out.keys) {
            key.rotation = key.rotation * glm::quat_cast(turned);
            key.scale = glm::transpose(turned) * key.scale * turned;
        }
    }
    out.visibility = kernel::geometry::kIndirectRay;
    if (scene.value(handle, "camera").integerArray().front() != 0) {
        out.visibility |= kernel::geometry::kCameraRay;
    }
    return out;
}

Geometry lightShape(const scene::Scene & scene, const scene::Handle & handle, Prototypes & prototypes) {
    using kernel::geometry::primitive::Form;
    using kernel::geometry::primitive::Shape;
    const std::string kind = kindOf(scene, handle);
    const auto size = [&](const char * name) {
        return scene.value(handle, name).floatArray().front();
    };
    Geometry shape;
    shape.id = handle.index;
    if (kind == "rectLight") {
        const float x = 0.5F * size("width");
        const float y = 0.5F * size("height");
        // two triangles wound to face -z
        const glm::vec3 corners[] = { { -x, -y, 0.0F }, { -x, y, 0.0F }, { x, y, 0.0F }, { -x, -y, 0.0F }, { x, y, 0.0F }, { x, -y, 0.0F } };
        shape.form = Form::Triangles;
        shape.first = static_cast<std::uint32_t>(prototypes.faces.size());
        shape.count = 2;
        shape.points = still(prototypes.points.size(), std::size(corners));
        prototypes.points.insert(prototypes.points.end(), std::begin(corners), std::end(corners));
        prototypes.faces.insert(prototypes.faces.end(), { 0U, 0U });
        return shape;
    }
    kernel::geometry::primitive::Quadric quadric;
    quadric.radius = size("radius");
    if (kind == "diskLight") {
        quadric.shape = Shape::Disk;
        quadric.height = 0.0F;
    } else if (kind == "sphereLight") {
        quadric.shape = Shape::Sphere;
        quadric.zmin = -quadric.radius;
        quadric.zmax = quadric.radius;
    } else {
        quadric.shape = Shape::Cylinder;
        quadric.zmin = -0.5F * size("length");
        quadric.zmax = 0.5F * size("length");
    }
    shape.form = Form::Quadric;
    shape.count = 1;
    shape.quadric = still(prototypes.quadrics.size(), 1);
    prototypes.quadrics.push_back(quadric);
    return shape;
}

};  // namespace v3d::moya::geometry::internal
