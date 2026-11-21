/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/internal/Making.h>

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Motion.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/ShapedLight.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/light/internal/Profiles.h>
#include <moya/libmoya/light/internal/Textures.h>
#include <moya/libmoya/light/Planckian.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Samples.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::light::internal {

namespace {

constexpr float kPi = 3.14159265358979323846F;

/** The steps of the sums that measure a sphere's and a cylinder's area under a transformation. **/
constexpr int kHeightSteps = 64;
constexpr int kAngleSteps = 128;

/** A cone of half a turn, in degrees, which takes in every direction. **/
constexpr float kWholeCone = 180.0F;

/** A whole turn, in degrees: the widest a distant light's angle is held to. **/
constexpr float kWholeTurn = 360.0F;

/** The solid angle of the whole sphere, in steradians. **/
constexpr float kWholeSphere = 4.0F * kPi;

/** A sphere's area over the area of the disk it casts: 4 pi r^2 over pi r^2. **/
constexpr float kSphereOverDisk = 4.0F;

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

/** The IES profile a light's iesFile names, with its angle scale and normalisation, or none. **/
std::uint32_t profileOf(const scene::Scene & scene, const scene::Handle & handle, Profiles & profiles) {
    const std::string & path = scene.value(handle, "iesFile").stringArray().front();
    if (path.empty()) {
        return kernel::geometry::kNoPrimitive;
    }
    return profiles.load(path, scene.value(handle, "iesAngleScale").floatArray().front(), scene.value(handle, "iesNormalize").integerArray().front() != 0);
}

/** The mean a light's profile multiplies its emission by, or one with none. **/
float shapingOf(const kernel::light::Light & light, const Profiles & profiles) {
    return light.profile == kernel::geometry::kNoPrimitive ? 1.0F : profiles.mean(light.profile);
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

};  // namespace

float coneFactor(float half) {
    if (!(half > 0.0F)) {
        return 1.0F;
    }
    const float sine = std::sin(half);
    return half > 0.5F * kPi ? kPi * (2.0F - (sine * sine)) : kPi * sine * sine;
}

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
 * Its size is its area in the world as it stands when the shutter opens, weighed by its texture's
 * average where it has one, since a texture shines by its texels.
 **/
Made shapedLight(const scene::Scene & scene, const geometry::Prototypes & prototypes, const geometry::ShapedLight & shaped, const colour::Config & config,
    Textures & textures, Profiles & profiles) {
    const geometry::Instance & instance = prototypes.instances[shaped.instance];
    const std::string & kind = scene.schema().kind(shaped.handle.kind).name;
    const float size = area(scene, shaped.handle, kind, glm::mat3(geometry::transformAt(instance, prototypes.open)));
    Made out { .handle = shaped.handle };
    out.light = withShape(scene, shaped.handle, instance.id, size, config);
    out.light.profile = profileOf(scene, shaped.handle, profiles);
    out.shaping = shapingOf(out.light, profiles);
    out.size = size;
    const std::string & path = kind == "rectLight" ? scene.value(shaped.handle, "texture").stringArray().front() : std::string();
    out.light.image = path.empty() ? kernel::geometry::kNoPrimitive : textures.load(path);
    out.read = out.light.image;
    if (out.light.image != kernel::geometry::kNoPrimitive) {
        out.size *= textures.average(out.light.image);
    }
    return out;
}

/**
 * Its radiant intensity is the sphere's it stands for, L pi r^2, which is L A / 4 for the area A
 * of that sphere in the world. With normalize, it is a quarter of its emission.
 **/
Made pointLight(const scene::Scene & scene, const scene::Handle & handle, float open, const colour::Config & config, Profiles & profiles) {
    const float size = area(scene, handle, "sphereLight", glm::mat3(geometry::transformAt(placement(scene, handle), open)));
    Made out { .handle = handle };
    out.light = emitter(scene, handle, config);
    out.light.id = kernel::geometry::kNoPrimitive;
    out.light.shape = kernel::light::Shape::Point;
    out.light.size = glm::vec2 { 0.0F };
    out.light.scale = normalised(scene, handle) ? out.light.scale / kSphereOverDisk : out.light.scale * size / kSphereOverDisk;
    cone(scene, handle, out.light);
    out.light.profile = profileOf(scene, handle, profiles);
    out.shaping = shapingOf(out.light, profiles);
    out.size = size;
    return out;
}

/** Its half angle is held to a half turn. **/
Made distantLight(const scene::Scene & scene, const scene::Handle & handle, const colour::Config & config) {
    Made out { .handle = handle };
    out.light = emitter(scene, handle, config);
    const float half = std::fmin(0.5F * glm::radians(heldTo(scene, handle, "angle", 0.0F, kWholeTurn)), kPi);
    out.light.id = kernel::geometry::kNoPrimitive;
    out.light.shape = kernel::light::Shape::Distant;
    out.light.size = glm::vec2 { half, 0.0F };
    if (normalised(scene, handle)) {
        out.light.scale /= coneFactor(half);
    }
    return out;
}

/**
 * Its image is read in latitude and longitude, and camera rays see it while its camera is 1. A
 * format of another layout, or a file that says it is a cube map, is reported, and the light emits
 * nothing. Its size is the integral of its image's luminance over the sphere, or the sphere's
 * solid angle with no image.
 **/
Made domeLight(const scene::Scene & scene, const scene::Handle & handle, const colour::Config & config, Textures & textures) {
    Made out { .handle = handle };
    out.light = emitter(scene, handle, config);
    out.light.id = kernel::geometry::kNoPrimitive;
    out.light.shape = kernel::light::Shape::Dome;
    out.light.camera = scene.value(handle, "camera").integerArray().front() != 0;
    out.size = kWholeSphere;
    const std::string & path = scene.value(handle, "texture").stringArray().front();
    if (path.empty()) {
        return out;
    }
    out.read = textures.load(path);
    if (out.read == kernel::geometry::kNoPrimitive) {
        return out;
    }
    const std::string & format = scene.value(handle, "format").stringArray().front();
    if ((format != "automatic" && format != "latlong") || (format == "automatic" && textures.cube(out.read))) {
        const std::string layout = format == "automatic" ? std::string("a cube map") : "the format " + format;
        textures.report("the dome light " + scene.name(handle) + " reads " + layout + ", which moya does not read, so it emits nothing");
        out.light.scale = 0.0F;
        return out;
    }
    out.light.image = out.read;
    out.size = textures.integral(out.read);
    return out;
}

/** Camera rays see it while its camera is 1. **/
Made meshLight(const scene::Scene & scene, const scene::Handle & handle, const colour::Config & config) {
    Made out { .handle = handle };
    out.light = emitter(scene, handle, config);
    out.light.camera = scene.value(handle, "camera").integerArray().front() != 0;
    out.normalise = normalised(scene, handle);
    return out;
}

};  // namespace v3d::moya::light::internal
