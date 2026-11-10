/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/Lights.h>

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Motion.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/ShapedLight.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/light/Planckian.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Samples.h>
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
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::light {

namespace {

constexpr float kPi = 3.14159265358979323846F;

/** The steps of the sums that measure a sphere's and a cylinder's area under a transformation. **/
constexpr int kHeightSteps = 64;
constexpr int kAngleSteps = 128;

/** A cone of half a turn, in degrees, which takes in every direction. **/
constexpr float kWholeCone = 180.0F;

/** A whole turn, in degrees: the widest a distant light's angle is held to. **/
constexpr float kWholeTurn = 360.0F;

/** A sphere's area over the area of the disk it casts: 4 pi r^2 over pi r^2. **/
constexpr float kSphereOverDisk = 4.0F;

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

/** The kernel's shape of a light of a kind. **/
kernel::light::Shape shapeOf(const std::string & kind) {
    if (kind == "diskLight") {
        return kernel::light::Shape::Disk;
    }
    if (kind == "sphereLight") {
        return kernel::light::Shape::Sphere;
    }
    return kind == "cylinderLight" ? kernel::light::Shape::Cylinder : kernel::light::Shape::Rect;
}

/** Whether a light's emission is divided by its size. **/
bool normalised(const scene::Scene & scene, const scene::Handle & handle) {
    return scene.value(handle, "normalize").integerArray().front() != 0;
}

/**
 * A distant light's size, as UsdLux measures it from the half angle of its cone: pi sin^2 for a
 * cone up to a quarter turn, and pi (2 - sin^2) for a wider one. Its radiance times its size is
 * the irradiance of a plane that faces it. A light of no angle gives that irradiance itself, so
 * its size is one.
 **/
float coneFactor(float half) {
    if (!(half > 0.0F)) {
        return 1.0F;
    }
    const float sine = std::sin(half);
    return half > 0.5F * kPi ? kPi * (2.0F - (sine * sine)) : kPi * sine * sine;
}

/**
 * What every light emits, before its size: its colour in ACEScg, its intensity times two to its
 * exposure, and its colour temperature.
 **/
kernel::light::Light emitter(const scene::Scene & scene, const scene::Handle & handle, const colour::Config & config) {
    const auto number = [&](const char * name) {
        return scene.value(handle, name).floatArray().front();
    };
    kernel::light::Light out;
    const scene::Value & colour = scene.value(handle, "colour");
    const std::vector<float> & floats = colour.floatArray();
    out.colour = config.input(colour.space()).apply(glm::vec3 { floats[0], floats[1], floats[2] });
    out.scale = number("intensity") * std::exp2(number("exposure"));
    if (scene.value(handle, "enableColorTemperature").integerArray().front() != 0) {
        const Planckian radiator = planckian(number("colorTemperature"));
        out.temperature = number("colorTemperature");
        out.planckScale = radiator.scale;
        out.planckColour = radiator.colour;
    }
    return out;
}

/** A light's cone, its angle held to [0, 180] degrees and its softness to [0, 1]. **/
void cone(const scene::Scene & scene, const scene::Handle & handle, kernel::light::Light & out) {
    out.coneAngle = glm::radians(heldTo(scene, handle, "coneAngle", 0.0F, kWholeCone));
    out.coneSoftness = heldTo(scene, handle, "coneSoftness", 0.0F, 1.0F);
}

/** Where a light with no shape stands over the shutter, as an instance would place it. **/
geometry::Instance placement(const scene::Scene & scene, const scene::Handle & handle) {
    const scene::Samples & transform = scene.samples(handle, "transform");
    geometry::Instance out;
    out.transform = transform.values.front().matrixAt(0);
    if (transform.values.size() > 1) {
        out.keys = geometry::keys(transform);
    }
    return out;
}

/**
 * A light with a shape, from its instance's index in the scene and its area in the world as it
 * stands when the shutter opens.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the instance, then the area
kernel::light::Light withShape(const scene::Scene & scene, const scene::Handle & handle, std::uint32_t id, float size, const colour::Config & config) {
    const std::string & kind = scene.schema().kind(handle.kind).name;
    const auto number = [&](const char * name) {
        return scene.value(handle, name).floatArray().front();
    };
    kernel::light::Light out = emitter(scene, handle, config);
    out.id = id;
    if (normalised(scene, handle) && size > 0.0F) {
        out.scale /= size;
    }
    // a cylinder light's shape is a quadric about z, turned onto x, so the light's -z is its +x
    out.axis = kind == "cylinderLight" ? glm::vec3 { 1.0F, 0.0F, 0.0F } : glm::vec3 { 0.0F, 0.0F, -1.0F };
    out.shape = shapeOf(kind);
    out.size = kind == "rectLight" ? glm::vec2 { number("width"), number("height") } : glm::vec2 { number("radius"), 0.0F };
    if (kind == "cylinderLight") {
        out.size.y = number("length");
    }
    cone(scene, handle, out);
    return out;
}

/**
 * A sphere light that is a point, from the area in the world of the sphere it stands for. Its
 * radiant intensity is the sphere's, L pi r^2, which is L A / 4. With normalize, it is a quarter
 * of its emission.
 **/
kernel::light::Light point(const scene::Scene & scene, const scene::Handle & handle, float size, const colour::Config & config) {
    kernel::light::Light out = emitter(scene, handle, config);
    out.id = kernel::geometry::kNoPrimitive;
    out.shape = kernel::light::Shape::Point;
    out.size = glm::vec2 { 0.0F };
    out.scale = normalised(scene, handle) ? out.scale / kSphereOverDisk : out.scale * size / kSphereOverDisk;
    cone(scene, handle, out);
    return out;
}

/** A distant light, its half angle held to a half turn. **/
kernel::light::Light distantOf(const scene::Scene & scene, const scene::Handle & handle, const colour::Config & config) {
    kernel::light::Light out = emitter(scene, handle, config);
    const float half = std::fmin(0.5F * glm::radians(heldTo(scene, handle, "angle", 0.0F, kWholeTurn)), kPi);
    out.id = kernel::geometry::kNoPrimitive;
    out.shape = kernel::light::Shape::Distant;
    out.size = glm::vec2 { half, 0.0F };
    if (normalised(scene, handle)) {
        out.scale /= coneFactor(half);
    }
    return out;
}

/**
 * A light's power before its luminance, which leaves its cone out. A light with a shape has its
 * radiance times its area times pi. A point has its intensity times four pi. A distant light has
 * the irradiance it gives a plane that faces it, times the area of a disk of the world's reach.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the area, then the reach
float powerOf(const kernel::light::Light & light, float size, float reach) {
    switch (light.shape) {
        case kernel::light::Shape::Point:
            return light.scale * kSphereOverDisk * kPi;
        case kernel::light::Shape::Distant:
            return light.scale * coneFactor(light.size.x) * kPi * reach * reach;
        case kernel::light::Shape::Rect:
        case kernel::light::Shape::Disk:
        case kernel::light::Shape::Sphere:
        case kernel::light::Shape::Cylinder:
            break;
    }
    return light.scale * size * kPi;
}

/**
 * The sum of the shares of the total up to and including each of a list of powers, the last held
 * to one. Empty when the powers add up to nothing.
 **/
std::vector<float> cumulative(const std::vector<float> & powers) {
    double total = 0.0;
    for (const float power : powers) {
        total += power;
    }
    std::vector<float> out;
    if (!(total > 0.0)) {
        return out;
    }
    double sum = 0.0;
    for (const float power : powers) {
        sum += power;
        out.push_back(static_cast<float>(sum / total));
    }
    out.back() = 1.0F;
    return out;
}

};  // namespace

Lights::Lights(device::Device & device, const scene::Scene & scene, const geometry::Prototypes & prototypes, const colour::Config & config,
    float reach) {
    std::vector<kernel::light::Light> lights;
    std::vector<kernel::geometry::Key> keys;
    std::vector<float> powers;
    std::vector<std::uint32_t> distant;
    const auto add = [&](kernel::light::Light light, const geometry::Instance & placed, float size) {
        light.transform = placed.transform;
        light.keys = static_cast<std::uint32_t>(keys.size());
        light.keyCount = static_cast<std::uint32_t>(placed.keys.size());
        keys.insert(keys.end(), placed.keys.begin(), placed.keys.end());
        lights.push_back(light);
        powers.push_back(std::fmax(0.0F, powerOf(light, size, reach) * kernel::spectrum::acescgToXyz(light.colour).y));
    };
    std::uint32_t most = 0;
    for (const geometry::ShapedLight & shaped : prototypes.lights) {
        const geometry::Instance & instance = prototypes.instances[shaped.instance];
        const std::string & kind = scene.schema().kind(shaped.handle.kind).name;
        const float size = area(scene, shaped.handle, kind, glm::mat3(geometry::transformAt(instance, prototypes.open)));
        add(withShape(scene, shaped.handle, instance.id, size, config), instance, size);
        most = std::max(most, instance.id + 1U);
    }
    std::vector<scene::Handle> unshaped = scene.objects("distantLight");
    for (const scene::Handle & handle : scene.objects("sphereLight")) {
        if (!geometry::shaped(scene, handle)) {
            unshaped.push_back(handle);
        }
    }
    std::sort(unshaped.begin(), unshaped.end(), [](const scene::Handle & left, const scene::Handle & right) { return left.index < right.index; });
    for (const scene::Handle & handle : unshaped) {
        const geometry::Instance placed = placement(scene, handle);
        if (scene.schema().kind(handle.kind).name == "sphereLight") {
            const float size = area(scene, handle, "sphereLight", glm::mat3(geometry::transformAt(placed, prototypes.open)));
            add(point(scene, handle, size, config), placed, size);
            continue;
        }
        const kernel::light::Light sun = distantOf(scene, handle, config);
        if (sun.size.x > 0.0F) {
            distant.push_back(static_cast<std::uint32_t>(lights.size()));
        }
        add(sun, placed, 0.0F);
    }
    std::vector<std::uint32_t> byId(most, kernel::geometry::kNoPrimitive);
    for (std::size_t i = 0; i < lights.size(); ++i) {
        if (lights[i].id != kernel::geometry::kNoPrimitive) {
            byId[lights[i].id] = static_cast<std::uint32_t>(i);
        }
    }
    lights_ = uploaded(device, lights);
    keys_ = uploaded(device, keys);
    distant_ = uploaded(device, distant);
    byId_ = uploaded(device, byId);
    cdf_ = uploaded(device, cumulative(powers));
}

kernel::light::Lights Lights::view() const {
    return kernel::light::Lights { .lights = device::view<kernel::light::Light>(*lights_),
        .keys = device::view<kernel::geometry::Key>(*keys_),
        .distant = device::view<std::uint32_t>(*distant_),
        .byId = device::view<std::uint32_t>(*byId_),
        .cdf = device::view<float>(*cdf_) };
}

};  // namespace v3d::moya::light
