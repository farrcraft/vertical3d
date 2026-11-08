/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/Quadrics.h>

#include <moya/libmoya/kernel/geometry/Shapes.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <string>

#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::geometry {

namespace {

/** A sampled value at a time, of which value(i) gives the i-th sample. **/
template <typename T, typename Value>
T sampledAt(const scene::Samples & samples, float time, const Value & value) {
    if (samples.times.size() < 2 || time <= samples.times.front()) {
        return value(0);
    }
    for (std::size_t sample = 0; sample + 1 < samples.times.size(); ++sample) {
        const float from = samples.times[sample];
        const float to = samples.times[sample + 1];
        if (time < to) {
            const float along = (time - from) / (to - from);
            return value(sample) + ((value(sample + 1) - value(sample)) * along);
        }
    }
    return value(samples.times.size() - 1);
}

};  // namespace

float numberAt(const scene::Samples & samples, float time) {
    return sampledAt<float>(samples, time, [&](std::size_t sample) {
        return samples.values[sample].floatArray().front();
    });
}

glm::vec3 pointOf(const scene::Samples & samples, float time) {
    return sampledAt<glm::vec3>(samples, time, [&](std::size_t sample) {
        return samples.values[sample].triple(0);
    });
}

kernel::geometry::Quadric quadricAt(const scene::Scene & scene, const scene::Handle & handle, float time) {
    const auto number = [&](const char * name) {
        return numberAt(scene.samples(handle, name), time);
    };
    const std::string & kind = scene.schema().kind(handle.kind).name;
    kernel::geometry::Quadric out;
    out.thetamax = glm::radians(number("thetamax"));
    if (kind == "sphere" || kind == "cylinder") {
        out.shape = kind == "sphere" ? kernel::geometry::Shape::Sphere : kernel::geometry::Shape::Cylinder;
        out.radius = number("radius");
        out.zmin = number("zmin");
        out.zmax = number("zmax");
    } else if (kind == "cone" || kind == "disk") {
        out.shape = kind == "cone" ? kernel::geometry::Shape::Cone : kernel::geometry::Shape::Disk;
        out.radius = number("radius");
        out.height = number("height");
    } else if (kind == "paraboloid") {
        out.shape = kernel::geometry::Shape::Paraboloid;
        out.radius = number("rmax");
        out.zmin = number("zmin");
        out.zmax = number("zmax");
    } else if (kind == "hyperboloid") {
        out.shape = kernel::geometry::Shape::Hyperboloid;
        out.point1 = pointOf(scene.samples(handle, "point1"), time);
        out.point2 = pointOf(scene.samples(handle, "point2"), time);
    } else {
        out.shape = kernel::geometry::Shape::Torus;
        out.radius = number("majorRadius");
        out.minorRadius = number("minorRadius");
        out.phimin = glm::radians(number("phimin"));
        out.phimax = glm::radians(number("phimax"));
    }
    return out;
}

};  // namespace v3d::moya::geometry
