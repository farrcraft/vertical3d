/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/Lights.h>

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <api/log/Logger.h>
#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Instance.h>
#include <moya/libmoya/geometry/Motion.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/ShapedLight.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/light/Bounds.h>
#include <moya/libmoya/kernel/light/Emission.h>
#include <moya/libmoya/kernel/light/Image.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/kernel/light/Profile.h>
#include <moya/libmoya/kernel/light/link/Links.h>
#include <moya/libmoya/kernel/light/link/LinkStep.h>
#include <moya/libmoya/kernel/light/Tree.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/light/internal/Gathered.h>
#include <moya/libmoya/light/internal/Linking.h>
#include <moya/libmoya/light/internal/Profiles.h>
#include <moya/libmoya/light/internal/Subtree.h>
#include <moya/libmoya/light/internal/Textures.h>
#include <moya/libmoya/light/internal/Tree.h>
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
#include <functional>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

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

/** The solid angle of the whole sphere, in steradians. **/
constexpr float kWholeSphere = 4.0F * kPi;

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

/** A light's box in its prototype's space: its shape's, or a point at its origin. **/
accel::Box localBox(const kernel::light::Light & light) {
    const glm::vec2 size = light.size;
    glm::vec3 half { 0.0F };
    switch (light.shape) {
        case kernel::light::Shape::Rect:
            half = glm::vec3 { 0.5F * size.x, 0.5F * size.y, 0.0F };
            break;
        case kernel::light::Shape::Disk:
            half = glm::vec3 { size.x, size.x, 0.0F };
            break;
        case kernel::light::Shape::Sphere:
            half = glm::vec3 { size.x };
            break;
        case kernel::light::Shape::Cylinder:
            half = glm::vec3 { size.x, size.x, 0.5F * size.y };
            break;
        case kernel::light::Shape::Point:
        case kernel::light::Shape::Distant:
        case kernel::light::Shape::Mesh:
        case kernel::light::Shape::Dome:
            break;
    }
    return accel::Box { .lower = -half, .upper = half };
}

/** A box where an instance places it, over the shutter from open to close. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the shutter's open, then its close
accel::Box placedBox(const accel::Box & box, const geometry::Instance & placed, float open, float close) {
    return placed.moves() ? accel::sweepBox(box, placed.keys, open, close).swept() : accel::moveBox(box, placed.transform);
}

/** Whether an instance turns, scales or shears what it places over the shutter. **/
bool turns(const geometry::Instance & placed) {
    return std::ranges::any_of(placed.keys, [&](const kernel::geometry::Key & key) {
        return key.rotation != placed.keys.front().rotation || key.scale != placed.keys.front().scale;
    });
}

/**
 * A light's bounds in the light tree, where an instance places it over the shutter, with its
 * power. A rect or disk light emits within a quarter turn of its axis, or within its cone where
 * that is narrower. A sphere, a cylinder or a point emits along every direction, or within its
 * cone, the cone's softness its falloff. A light that turns over the shutter is bounded by every
 * direction.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the power, then the shutter
kernel::light::Bounds boundsOf(const kernel::light::Light & light, const geometry::Instance & placed, float power, float open, float close) {
    const accel::Box box = placedBox(localBox(light), placed, open, close);
    kernel::light::Bounds out = internal::everyDirection(box.lower, box.upper);
    out.power = power;
    if (turns(placed)) {
        return out;
    }
    out.axis = kernel::light::axisOf(light, placed.transform);
    const bool coned = light.coneAngle < kernel::light::kQuarterTurn || light.coneSoftness > 0.0F;
    if (light.shape == kernel::light::Shape::Rect || light.shape == kernel::light::Shape::Disk) {
        out.cosSpread = 1.0F;
        out.cosFalloff = std::cos(coned ? std::fmin(light.coneAngle, kernel::light::kQuarterTurn) : kernel::light::kQuarterTurn);
    } else if (coned) {
        out.cosSpread = std::cos(light.coneAngle * (1.0F - light.coneSoftness));
        out.cosFalloff = std::cos(light.coneAngle * light.coneSoftness);
    }
    return out;
}

/** The IES profile a light's iesFile names, with its angle scale and normalisation, or none. **/
std::uint32_t profileOf(const scene::Scene & scene, const scene::Handle & handle, internal::Profiles & profiles) {
    const std::string & path = scene.value(handle, "iesFile").stringArray().front();
    if (path.empty()) {
        return kernel::geometry::kNoPrimitive;
    }
    return profiles.load(path, scene.value(handle, "iesAngleScale").floatArray().front(), scene.value(handle, "iesNormalize").integerArray().front() != 0);
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
 * the irradiance it gives a plane that faces it, times the area of a disk of the world's reach. A
 * dome light has its radiance times the integral of its image's luminance over the sphere, given
 * as its size, times the area of that disk.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the area, then the reach
float powerOf(const kernel::light::Light & light, float size, float reach) {
    switch (light.shape) {
        case kernel::light::Shape::Point:
            return light.scale * kSphereOverDisk * kPi;
        case kernel::light::Shape::Distant:
            return light.scale * coneFactor(light.size.x) * kPi * reach * reach;
        case kernel::light::Shape::Dome:
            return light.scale * size * kPi * reach * reach;
        case kernel::light::Shape::Rect:
        case kernel::light::Shape::Disk:
        case kernel::light::Shape::Sphere:
        case kernel::light::Shape::Cylinder:
        case kernel::light::Shape::Mesh:
            break;
    }
    return light.scale * size * kPi;
}

/**
 * A dome light, whose image is read in latitude and longitude. A format of another layout, or a
 * file that says it is a cube map, is reported, and the light emits nothing.
 **/
kernel::light::Light domeOf(const scene::Scene & scene, const scene::Handle & handle, const colour::Config & config, internal::Textures & textures) {
    kernel::light::Light out = emitter(scene, handle, config);
    out.id = kernel::geometry::kNoPrimitive;
    out.shape = kernel::light::Shape::Dome;
    const std::string & path = scene.value(handle, "texture").stringArray().front();
    if (path.empty()) {
        return out;
    }
    const std::uint32_t image = textures.load(path);
    if (image == kernel::geometry::kNoPrimitive) {
        return out;
    }
    const std::string & format = scene.value(handle, "format").stringArray().front();
    if ((format != "automatic" && format != "latlong") || (format == "automatic" && textures.cube(image))) {
        const std::string layout = format == "automatic" ? std::string("a cube map") : "the format " + format;
        textures.report("the dome light " + scene.name(handle) + " reads " + layout + ", which moya does not read, so it emits nothing");
        out.scale = 0.0F;
        return out;
    }
    textures.drawable(image);
    out.image = image;
    return out;
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

/**
 * The mesh light that makes each geometry of the scene emit, by the geometry's index in the
 * scene: the first, in the order of their indices, of those that name it.
 **/
std::map<std::uint32_t, scene::Handle> emitters(const scene::Scene & scene) {
    std::vector<scene::Handle> meshLights = scene.objects("meshLight");
    std::sort(meshLights.begin(), meshLights.end(), [](const scene::Handle & left, const scene::Handle & right) { return left.index < right.index; });
    std::map<std::uint32_t, scene::Handle> out;
    for (const scene::Handle & handle : meshLights) {
        const std::vector<scene::Handle> & named = scene.value(handle, "geometry").handleArray();
        if (!named.empty() && scene.contains(named.front())) {
            out.emplace(named.front().index, handle);
        }
    }
    return out;
}

/**
 * Calls found with every chain of instances from a group's members that places a geometry, by
 * the instances' places among the prototypes', outermost first, and the geometry's place among
 * the prototypes'. A chain deeper than kernel::geometry::kInstanceDepth places nothing, as a
 * traversal leaves it out.
 **/
void chains(const geometry::Prototypes & prototypes, const std::vector<std::uint32_t> & members, std::vector<std::uint32_t> & chain,
    const std::function<void(const std::vector<std::uint32_t> &, std::uint32_t)> & found) {
    if (chain.size() == kernel::geometry::kInstanceDepth) {
        return;
    }
    for (const std::uint32_t member : members) {
        const geometry::Instance & instance = prototypes.instances[member];
        chain.push_back(member);
        if (instance.kind == geometry::Prototype::Group) {
            chains(prototypes, prototypes.groups[instance.prototype].instances, chain, found);
        } else {
            found(chain, instance.prototype);
        }
        chain.pop_back();
    }
}

/**
 * Each triangle of a geometry where a transformation places it when the shutter opens: a vector
 * along the normal of the side it emits from, as long as its area.
 **/
std::vector<glm::vec3> emittingAreas(const geometry::Prototypes & prototypes, const glm::mat4 & toWorld, const geometry::Geometry & shape) {
    // a transformation that mirrors turns the corners' winding against the normal
    const float side = glm::determinant(glm::mat3(toWorld)) < 0.0F ? -0.5F : 0.5F;
    std::vector<glm::vec3> out;
    for (std::uint32_t triangle = 0; triangle < shape.count; ++triangle) {
        glm::vec3 corners[3];
        for (std::uint32_t corner = 0; corner < 3U; ++corner) {
            const glm::vec3 local =
                kernel::geometry::sampleAt(shape.points, prototypes.points.data(), prototypes.times.data(), (triangle * 3U) + corner, prototypes.open);
            corners[corner] = glm::vec3(toWorld * glm::vec4(local, 1.0F));
        }
        out.push_back(side * glm::cross(corners[1] - corners[0], corners[2] - corners[0]));
    }
    return out;
}

/**
 * The cone of the directions triangles face, each given by emittingAreas(), about the axis of
 * their sum. Left as it was when they sum to nothing.
 **/
void spreadOf(const std::vector<glm::vec3> & areas, kernel::light::Bounds & bounds) {
    glm::vec3 sum { 0.0F };
    for (const glm::vec3 & area : areas) {
        sum += area;
    }
    if (!(glm::dot(sum, sum) > 0.0F)) {
        return;
    }
    bounds.axis = glm::normalize(sum);
    bounds.cosSpread = 1.0F;
    for (const glm::vec3 & area : areas) {
        if (glm::dot(area, area) > 0.0F) {
            bounds.cosSpread = std::fmin(bounds.cosSpread, glm::dot(bounds.axis, glm::normalize(area)));
        }
    }
}

/**
 * A subtree over the triangles of a geometry, in its own space, appended to what is gathered. Each
 * triangle's power is its area when the shutter opens, and its box holds it over the shutter. One
 * whose points move is bounded by every direction.
 **/
internal::Subtree subtreeOf(const geometry::Prototypes & prototypes, const geometry::Geometry & shape, internal::Gathered & out) {
    const accel::Sweep sweep = accel::triangleSweep(shape.points, prototypes.points, prototypes.times);
    const std::vector<glm::vec3> areas = emittingAreas(prototypes, glm::mat4(1.0F), shape);
    const bool still = shape.points.samples < 2U;
    std::vector<internal::Leaf> leaves;
    for (std::uint32_t triangle = 0; triangle < shape.count; ++triangle) {
        accel::Box box;
        if (still) {
            for (std::uint32_t corner = 0; corner < 3U; ++corner) {
                box.grow(kernel::geometry::sampleAt(shape.points, prototypes.points.data(), prototypes.times.data(), (triangle * 3U) + corner, prototypes.open));
            }
        } else {
            box = sweep(triangle, prototypes.open, prototypes.close).swept();
        }
        internal::Leaf leaf { .bounds = internal::everyDirection(box.lower, box.upper), .held = triangle };
        leaf.bounds.power = glm::length(areas[triangle]);
        if (still && leaf.bounds.power > 0.0F) {
            leaf.bounds.axis = glm::normalize(areas[triangle]);
            leaf.bounds.cosSpread = 1.0F;
        }
        leaves.push_back(leaf);
    }
    internal::Subtree made;
    made.leaves = static_cast<std::uint32_t>(out.triangleLeaves.size());
    out.triangleLeaves.resize(out.triangleLeaves.size() + shape.count, kernel::geometry::kNoPrimitive);
    made.root = internal::build(leaves, out.nodes, out.triangleLeaves, made.leaves);
    if (made.root != kernel::geometry::kNoPrimitive) {
        made.bounds = out.nodes[made.root].bounds;
    }
    return made;
}

/**
 * A mesh light's bounds in the top tree, with its power: its subtree's box where its chain places
 * it over the shutter, and the directions its triangles face in the world, each given by
 * emittingAreas(). A chain that turns, or points that move, are bounded by every direction.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the chain, then the triangles
kernel::light::Bounds meshBounds(const geometry::Prototypes & prototypes, const std::vector<std::uint32_t> & chain, const geometry::Geometry & shape,
    const internal::Subtree & subtree, const std::vector<glm::vec3> & areas, float power) {  // NOLINT(bugprone-easily-swappable-parameters)
    accel::Box box { .lower = subtree.bounds.lower, .upper = subtree.bounds.upper };
    bool moves = shape.points.samples > 1U;
    for (auto level = chain.rbegin(); level != chain.rend(); ++level) {
        const geometry::Instance & instance = prototypes.instances[*level];
        box = placedBox(box, instance, prototypes.open, prototypes.close);
        moves = moves || turns(instance);
    }
    kernel::light::Bounds out = internal::everyDirection(box.lower, box.upper);
    out.power = power;
    if (!moves) {
        spreadOf(areas, out);
    }
    return out;
}

/**
 * The mesh light that a mesh light's handle makes where a chain of instances places a geometry of
 * triangles, appended to what is gathered with its power. It leads into the geometry's subtree,
 * which the first light that places the geometry builds. A chain that places triangles of no area
 * adds nothing.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - what is read, then what is built
void appendMesh(const scene::Scene & scene, const geometry::Prototypes & prototypes, const colour::Config & config, const scene::Handle & handle,
    const std::vector<std::uint32_t> & placed, std::uint32_t at, std::map<std::uint32_t, internal::Subtree> & subtrees, internal::Gathered & out) {
    const geometry::Geometry & shape = prototypes.geometries[at];
    glm::mat4 toWorld { 1.0F };
    for (const std::uint32_t member : placed) {
        toWorld = toWorld * geometry::transformAt(prototypes.instances[member], prototypes.open);
    }
    const std::vector<glm::vec3> areas = emittingAreas(prototypes, toWorld, shape);
    std::vector<float> lengths;
    float size = 0.0F;
    for (const glm::vec3 & area : areas) {
        lengths.push_back(glm::length(area));
        size += lengths.back();
    }
    const std::vector<float> sums = cumulative(lengths);
    if (sums.empty()) {
        return;
    }
    auto subtree = subtrees.find(at);
    if (subtree == subtrees.end()) {
        subtree = subtrees.emplace(at, subtreeOf(prototypes, shape, out)).first;
    }
    kernel::light::Light made = emitter(scene, handle, config);
    made.id = kernel::geometry::kNoPrimitive;
    made.shape = kernel::light::Shape::Mesh;
    made.depth = static_cast<std::uint32_t>(placed.size());
    for (std::size_t level = 0; level < placed.size(); ++level) {
        made.chain[level] = prototypes.instances[placed[level]].id;
    }
    made.geometry = at;
    made.triangles = static_cast<std::uint32_t>(out.triangles.size());
    made.count = shape.count;
    made.tree = subtree->second.root;
    made.leaves = subtree->second.leaves;
    made.camera = scene.value(handle, "camera").integerArray().front() != 0;
    if (normalised(scene, handle)) {
        made.scale /= size;
    }
    out.triangles.insert(out.triangles.end(), sums.begin(), sums.end());
    if (out.meshes.size() <= made.chain[0]) {
        out.meshes.resize(made.chain[0] + 1U, kernel::geometry::kNoPrimitive);
    }
    if (out.meshes[made.chain[0]] == kernel::geometry::kNoPrimitive) {
        out.meshes[made.chain[0]] = static_cast<std::uint32_t>(out.lights.size());
    }
    const float power = std::fmax(0.0F, powerOf(made, size, 0.0F) * kernel::spectrum::acescgToXyz(made.colour).y);
    if (made.tree != kernel::geometry::kNoPrimitive) {
        out.top.push_back(internal::Leaf { .bounds = meshBounds(prototypes, placed, shape, subtree->second, areas, power),
            .held = static_cast<std::uint32_t>(out.lights.size()) });
    }
    out.lights.push_back(made);
    out.handles.push_back(handle);
    out.powers.push_back(power);
}

/**
 * The scene's mesh lights, each a light for every chain of instances that places its geometry,
 * appended to what is gathered with their powers. Each triangle's chance is appended to the
 * triangles, and the first light each instance in the world places to the meshes. Each geometry
 * that emits has one subtree, which each light that places it leads into from the top tree. A
 * light whose geometry has no triangles emits nothing, and is reported.
 **/
void meshLights(const scene::Scene & scene, const geometry::Prototypes & prototypes, const colour::Config & config,
    const boost::shared_ptr<log::Logger> & logger, internal::Gathered & out) {
    const std::map<std::uint32_t, scene::Handle> named = emitters(scene);
    if (named.empty()) {
        return;
    }
    std::set<std::uint32_t> reported;
    std::map<std::uint32_t, internal::Subtree> subtrees;
    std::vector<std::uint32_t> chain;
    chains(prototypes, prototypes.world.instances, chain, [&](const std::vector<std::uint32_t> & placed, std::uint32_t at) {
        const geometry::Geometry & shape = prototypes.geometries[at];
        const auto naming = named.find(shape.id);
        if (naming == named.end()) {
            return;
        }
        const scene::Handle & handle = naming->second;
        if (shape.form != kernel::geometry::primitive::Form::Triangles) {
            if (reported.insert(handle.index).second && logger) {
                logger->get()->warn("the mesh light {} names a geometry of no triangles, so it emits nothing", scene.name(handle));
            }
            return;
        }
        appendMesh(scene, prototypes, config, handle, placed, at, subtrees, out);
    });
}

/**
 * The light a shaped light makes, with the size its power is measured by: its area in the world
 * as it stands when the shutter opens, weighed by its texture's average where it has one.
 **/
kernel::light::Light shapedLight(const scene::Scene & scene, const geometry::Prototypes & prototypes, const geometry::ShapedLight & shaped,
    const colour::Config & config, internal::Textures & textures, internal::Profiles & profiles, float & shining) {
    const geometry::Instance & instance = prototypes.instances[shaped.instance];
    const std::string & kind = scene.schema().kind(shaped.handle.kind).name;
    const float size = area(scene, shaped.handle, kind, glm::mat3(geometry::transformAt(instance, prototypes.open)));
    kernel::light::Light made = withShape(scene, shaped.handle, instance.id, size, config);
    made.profile = profileOf(scene, shaped.handle, profiles);
    shining = size;
    const std::string & path = kind == "rectLight" ? scene.value(shaped.handle, "texture").stringArray().front() : std::string();
    made.image = path.empty() ? kernel::geometry::kNoPrimitive : textures.load(path);
    if (made.image != kernel::geometry::kNoPrimitive) {
        // a texture shines by its texels, so it weighs the light by their average
        shining *= textures.average(made.image);
    }
    return made;
}

/**
 * A top tree for each set of linked lights over the lights with a place that light it, and the
 * distant and dome lights that do, appended to what is gathered.
 **/
void treesBySet(const internal::Linking & links, internal::Gathered & all) {
    const auto count = static_cast<std::uint32_t>(all.lights.size());
    all.leaves.assign(static_cast<std::size_t>(links.count) * count, kernel::geometry::kNoPrimitive);
    all.afarFirst.assign(1, 0U);
    for (std::uint32_t set = 0; set < links.count; ++set) {
        std::vector<internal::Leaf> top;
        std::copy_if(all.top.begin(), all.top.end(), std::back_inserter(top), [&](const internal::Leaf & leaf) { return links.lights(set, leaf.held, count); });
        all.roots.push_back(internal::build(top, all.nodes, all.leaves, set * count));
        std::copy_if(all.infinite.begin(), all.infinite.end(), std::back_inserter(all.afar), [&](std::uint32_t place) { return links.lights(set, place, count); });
        all.afarFirst.push_back(static_cast<std::uint32_t>(all.afar.size()));
    }
}

};  // namespace

Lights::Lights(device::Device & device, const scene::Scene & scene, const geometry::Prototypes & prototypes, const colour::Config & config,
    float reach, const boost::shared_ptr<log::Logger> & logger) {
    internal::Gathered all;
    std::vector<kernel::light::Light> & lights = all.lights;
    std::vector<kernel::geometry::Key> keys;
    std::vector<std::uint32_t> distant;
    std::vector<std::uint32_t> domes;
    internal::Textures textures(config, logger);
    internal::Profiles profiles(logger);
    const auto add = [&](kernel::light::Light light, const scene::Handle & handle, const geometry::Instance & placed, float size) {
        light.transform = placed.transform;
        light.keys = static_cast<std::uint32_t>(keys.size());
        light.keyCount = static_cast<std::uint32_t>(placed.keys.size());
        keys.insert(keys.end(), placed.keys.begin(), placed.keys.end());
        const float shaped = light.profile == kernel::geometry::kNoPrimitive ? 1.0F : profiles.mean(light.profile);
        const float power = std::fmax(0.0F, powerOf(light, size, reach) * kernel::spectrum::acescgToXyz(light.colour).y * shaped);
        const auto place = static_cast<std::uint32_t>(lights.size());
        if (light.shape == kernel::light::Shape::Distant || light.shape == kernel::light::Shape::Dome) {
            all.infinite.push_back(place);
        } else {
            all.top.push_back(internal::Leaf { .bounds = boundsOf(light, placed, power, prototypes.open, prototypes.close), .held = place });
        }
        lights.push_back(light);
        all.powers.push_back(power);
        all.handles.push_back(handle);
    };
    std::uint32_t most = 0;
    for (const geometry::ShapedLight & shaped : prototypes.lights) {
        const geometry::Instance & instance = prototypes.instances[shaped.instance];
        float shining = 0.0F;
        const kernel::light::Light made = shapedLight(scene, prototypes, shaped, config, textures, profiles, shining);
        add(made, shaped.handle, instance, shining);
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
            kernel::light::Light bulb = point(scene, handle, size, config);
            bulb.profile = profileOf(scene, handle, profiles);
            add(bulb, handle, placed, size);
            continue;
        }
        const kernel::light::Light sun = distantOf(scene, handle, config);
        if (sun.size.x > 0.0F) {
            distant.push_back(static_cast<std::uint32_t>(lights.size()));
        }
        add(sun, handle, placed, 0.0F);
    }
    std::vector<scene::Handle> skies = scene.objects("domeLight");
    std::sort(skies.begin(), skies.end(), [](const scene::Handle & left, const scene::Handle & right) { return left.index < right.index; });
    for (const scene::Handle & handle : skies) {
        const kernel::light::Light sky = domeOf(scene, handle, config, textures);
        domes.push_back(static_cast<std::uint32_t>(lights.size()));
        add(sky, handle, placement(scene, handle), sky.image == kernel::geometry::kNoPrimitive ? kWholeSphere : textures.integral(sky.image));
    }
    meshLights(scene, prototypes, config, logger, all);
    const internal::Linking links = internal::linking(scene, prototypes, all.handles, lights);
    treesBySet(links, all);
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
    cdf_ = uploaded(device, cumulative(all.powers));
    triangles_ = uploaded(device, all.triangles);
    meshes_ = uploaded(device, all.meshes);
    images_ = uploaded(device, textures.images());
    texels_ = uploaded(device, textures.texels());
    sums_ = uploaded(device, textures.sums());
    domes_ = uploaded(device, domes);
    nodes_ = uploaded(device, all.nodes);
    roots_ = uploaded(device, all.roots);
    leaves_ = uploaded(device, all.leaves);
    triangleLeaves_ = uploaded(device, all.triangleLeaves);
    infinite_ = uploaded(device, all.afar);
    infiniteFirst_ = uploaded(device, all.afarFirst);
    linkFirst_ = uploaded(device, links.first);
    linkSteps_ = uploaded(device, links.steps);
    lit_ = uploaded(device, links.lit);
    casters_ = uploaded(device, links.casters);
    profiles_ = uploaded(device, profiles.profiles());
    photometry_ = uploaded(device, profiles.photometry());
    count_ = static_cast<std::uint32_t>(lights.size());
}

kernel::light::Lights Lights::view() const {
    return kernel::light::Lights { .lights = device::view<kernel::light::Light>(*lights_),
        .keys = device::view<kernel::geometry::Key>(*keys_),
        .distant = device::view<std::uint32_t>(*distant_),
        .byId = device::view<std::uint32_t>(*byId_),
        .cdf = device::view<float>(*cdf_),
        .triangles = device::view<float>(*triangles_),
        .meshes = device::view<std::uint32_t>(*meshes_),
        .images = device::view<kernel::light::Image>(*images_),
        .texels = device::view<glm::vec3>(*texels_),
        .sums = device::view<float>(*sums_),
        .domes = device::view<std::uint32_t>(*domes_),
        .nodes = device::view<kernel::light::TreeNode>(*nodes_),
        .roots = device::view<std::uint32_t>(*roots_),
        .leaves = device::view<std::uint32_t>(*leaves_),
        .triangleLeaves = device::view<std::uint32_t>(*triangleLeaves_),
        .infinite = device::view<std::uint32_t>(*infinite_),
        .infiniteFirst = device::view<std::uint32_t>(*infiniteFirst_),
        .links = kernel::light::link::Links { .first = device::view<std::uint32_t>(*linkFirst_),
            .steps = device::view<kernel::light::link::LinkStep>(*linkSteps_),
            .lit = device::view<std::uint8_t>(*lit_),
            .count = count_,
            .casters = device::view<std::uint32_t>(*casters_) },
        .profiles = device::view<kernel::light::Profile>(*profiles_),
        .photometry = device::view<float>(*photometry_) };
}

};  // namespace v3d::moya::light
