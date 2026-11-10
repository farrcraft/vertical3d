/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/Lights.h>

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/ShapedLight.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/light/Planckian.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::light {

namespace {

constexpr float kPi = 3.14159265358979323846F;

/** The steps of the sums that measure a sphere's and a cylinder's area under a transformation. **/
constexpr int kHeightSteps = 64;
constexpr int kAngleSteps = 128;

/** A cone of half a turn, in degrees, which takes in every direction. **/
constexpr float kWholeCone = 180.0F;

template <typename T>
boost::shared_ptr<device::Buffer> uploaded(device::Device & device, const std::vector<T> & values) {
    const std::size_t bytes = values.size() * sizeof(T);
    auto buffer = device.allocate(bytes);
    device.upload(*buffer, values.data(), bytes);
    return buffer;
}

/**
 * How a linear map scales the area of a surface where its unit normal is a direction: the length
 * of the normal through the map's cofactor matrix.
 **/
float stretch(const glm::mat3 & cofactor, const glm::vec3 & normal) {
    return glm::length(cofactor * normal);
}

/**
 * The area in the world of a light's shape, in its prototype's space, under a transformation. A
 * rect's and a disk's are exact. A sphere's and a cylinder's are summed over their normals, which
 * is exact under a turn and a scale of the same size on every axis.
 **/
float area(const scene::Scene & scene, const scene::Handle & handle, const std::string & kind, const glm::mat3 & toWorld) {
    const glm::mat3 cofactor { glm::cross(toWorld[1], toWorld[2]), glm::cross(toWorld[2], toWorld[0]), glm::cross(toWorld[0], toWorld[1]) };
    const auto size = [&](const char * name) {
        return scene.value(handle, name).floatArray().front();
    };
    const glm::vec3 up { 0.0F, 0.0F, 1.0F };
    if (kind == "rectLight") {
        return size("width") * size("height") * stretch(cofactor, up);
    }
    const float radius = size("radius");
    if (kind == "diskLight") {
        return kPi * radius * radius * stretch(cofactor, up);
    }
    const float turn = 2.0F * kPi / static_cast<float>(kAngleSteps);
    float sum = 0.0F;
    if (kind == "sphereLight") {
        // heights spread evenly over [-1, 1] cover the sphere in bands of equal area
        const float band = 2.0F / static_cast<float>(kHeightSteps);
        for (int height = 0; height < kHeightSteps; ++height) {
            const float z = -1.0F + ((static_cast<float>(height) + 0.5F) * band);
            const float across = std::sqrt(1.0F - (z * z));
            for (int angle = 0; angle < kAngleSteps; ++angle) {
                const float phi = (static_cast<float>(angle) + 0.5F) * turn;
                sum += stretch(cofactor, glm::vec3 { across * std::cos(phi), across * std::sin(phi), z });
            }
        }
        return radius * radius * sum * band * turn;
    }
    for (int angle = 0; angle < kAngleSteps; ++angle) {
        const float phi = (static_cast<float>(angle) + 0.5F) * turn;
        sum += stretch(cofactor, glm::vec3 { std::cos(phi), std::sin(phi), 0.0F });
    }
    return radius * size("length") * sum * turn;
}

/** A float parameter held between a least and a most. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the least, then the most
float heldTo(const scene::Scene & scene, const scene::Handle & handle, const char * name, float least, float most) {
    return std::clamp(scene.value(handle, name).floatArray().front(), least, most);
}

};  // namespace

Lights::Lights(device::Device & device, const scene::Scene & scene, const geometry::Prototypes & prototypes, const colour::Config & config) {
    std::vector<kernel::light::Light> lights;
    std::uint32_t most = 0;
    for (const geometry::ShapedLight & shaped : prototypes.lights) {
        const scene::Handle & handle = shaped.handle;
        const geometry::Instance & instance = prototypes.instances[shaped.instance];
        const std::string & kind = scene.schema().kind(handle.kind).name;
        const auto number = [&](const char * name) {
            return scene.value(handle, name).floatArray().front();
        };
        kernel::light::Light out;
        out.id = instance.id;
        const scene::Value & colour = scene.value(handle, "colour");
        const std::vector<float> & floats = colour.floatArray();
        out.colour = config.input(colour.space()).apply(glm::vec3 { floats[0], floats[1], floats[2] });
        out.scale = number("intensity") * std::exp2(number("exposure"));
        if (scene.value(handle, "normalize").integerArray().front() != 0) {
            const float size = area(scene, handle, kind, glm::mat3(geometry::transformAt(instance, prototypes.open)));
            out.scale = size > 0.0F ? out.scale / size : out.scale;
        }
        if (scene.value(handle, "enableColorTemperature").integerArray().front() != 0) {
            const Planckian radiator = planckian(number("colorTemperature"));
            out.temperature = number("colorTemperature");
            out.planckScale = radiator.scale;
            out.planckColour = radiator.colour;
        }
        // a cylinder light's shape is a quadric about z, turned onto x, so the light's -z is its +x
        out.axis = kind == "cylinderLight" ? glm::vec3 { 1.0F, 0.0F, 0.0F } : glm::vec3 { 0.0F, 0.0F, -1.0F };
        out.oneSided = kind == "rectLight" || kind == "diskLight" ? 1U : 0U;
        out.coneAngle = glm::radians(heldTo(scene, handle, "coneAngle", 0.0F, kWholeCone));
        out.coneSoftness = heldTo(scene, handle, "coneSoftness", 0.0F, 1.0F);
        lights.push_back(out);
        most = out.id + 1U > most ? out.id + 1U : most;
    }
    std::vector<std::uint32_t> byId(most, kernel::geometry::kNoPrimitive);
    for (std::size_t i = 0; i < lights.size(); ++i) {
        byId[lights[i].id] = static_cast<std::uint32_t>(i);
    }
    lights_ = uploaded(device, lights);
    byId_ = uploaded(device, byId);
}

kernel::light::Lights Lights::view() const {
    return kernel::light::Lights { .lights = device::view<kernel::light::Light>(*lights_), .byId = device::view<std::uint32_t>(*byId_) };
}

};  // namespace v3d::moya::light
