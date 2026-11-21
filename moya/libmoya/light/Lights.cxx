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
#include <moya/libmoya/geometry/ConvertedMaterials.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/ShapedLight.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/light/Bounds.h>
#include <moya/libmoya/kernel/light/Brick.h>
#include <moya/libmoya/kernel/light/Emission.h>
#include <moya/libmoya/kernel/light/Image.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/kernel/light/Profile.h>
#include <moya/libmoya/kernel/light/link/Links.h>
#include <moya/libmoya/kernel/light/link/LinkStep.h>
#include <moya/libmoya/kernel/light/Tree.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/volume/Volume.h>
#include <moya/libmoya/light/internal/Emitters.h>
#include <moya/libmoya/light/internal/Gathered.h>
#include <moya/libmoya/light/internal/Glowing.h>
#include <moya/libmoya/light/internal/Linking.h>
#include <moya/libmoya/light/internal/Making.h>
#include <moya/libmoya/light/internal/Profiles.h>
#include <moya/libmoya/light/internal/Subtree.h>
#include <moya/libmoya/light/internal/Textures.h>
#include <moya/libmoya/light/internal/Tree.h>
#include <moya/libmoya/light/Shading.h>
#include <moya/libmoya/scene/Change.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::light {

namespace {

constexpr float kPi = 3.14159265358979323846F;

/** The solid angle of the whole sphere, in steradians. **/
constexpr float kWholeSphere = 4.0F * kPi;

/** A sphere's area over the area of the disk it casts: 4 pi r^2 over pi r^2. **/
constexpr float kSphereOverDisk = 4.0F;

/** The kinds of the scene's lights. An update makes one again when its object changes. **/
constexpr std::array<std::string_view, 7> kLightKinds { "rectLight", "diskLight", "sphereLight", "cylinderLight", "distantLight", "domeLight", "meshLight" };

/** The kinds of the objects no light reads but through the reach and the shutter. **/
constexpr std::array<std::string_view, 4> kQuietKinds { "camera", "options", "output", "channel" };

template <typename T>
boost::shared_ptr<device::Buffer> uploaded(device::Device & device, const std::vector<T> & values) {
    const std::size_t bytes = values.size() * sizeof(T);
    auto buffer = device.allocate(bytes);
    device.upload(*buffer, values.data(), bytes);
    return buffer;
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
        case kernel::light::Shape::Volume:
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
            return light.scale * internal::coneFactor(light.size.x) * kPi * reach * reach;
        case kernel::light::Shape::Dome:
            return light.scale * size * kPi * reach * reach;
        case kernel::light::Shape::Rect:
        case kernel::light::Shape::Disk:
        case kernel::light::Shape::Sphere:
        case kernel::light::Shape::Mesh:
            // a triangle that emits from both faces sends as much again from its other
            return light.scale * size * kPi * (light.twoSided ? 2.0F : 1.0F);
        case kernel::light::Shape::Cylinder:
        case kernel::light::Shape::Volume:
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
 * whose points move, or one that emits from both faces, is bounded by every direction.
 **/
internal::Subtree subtreeOf(const geometry::Prototypes & prototypes, const geometry::Geometry & shape, bool twoSided, internal::Gathered & out) {
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
        if (still && !twoSided && leaf.bounds.power > 0.0F) {
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
 * emittingAreas(). A chain that turns, points that move, or triangles that emit from both faces
 * are bounded by every direction.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the chain, then the triangles
kernel::light::Bounds meshBounds(const geometry::Prototypes & prototypes, const std::vector<std::uint32_t> & chain, const geometry::Geometry & shape,
    const internal::Subtree & subtree, const std::vector<glm::vec3> & areas, float power, bool twoSided) {  // NOLINT(bugprone-easily-swappable-parameters)
    accel::Box box { .lower = subtree.bounds.lower, .upper = subtree.bounds.upper };
    bool moves = shape.points.samples > 1U;
    for (auto level = chain.rbegin(); level != chain.rend(); ++level) {
        const geometry::Instance & instance = prototypes.instances[*level];
        box = placedBox(box, instance, prototypes.open, prototypes.close);
        moves = moves || turns(instance);
    }
    kernel::light::Bounds out = internal::everyDirection(box.lower, box.upper);
    out.power = power;
    if (!moves && !twoSided) {
        spreadOf(areas, out);
    }
    return out;
}

/**
 * A mesh light made where a chain of instances places a geometry of triangles, appended to what
 * is gathered with its power. made holds what it emits, whether camera rays see it, and whether
 * it is normalised by its area. It leads into the geometry's subtree, which the first light that
 * places the geometry builds. A chain that places triangles of no area adds nothing. The handle
 * is the mesh light's, or one no scene gave for a material that emits.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - what is read, then what is built
void appendMesh(const geometry::Prototypes & prototypes, kernel::light::Light made, bool normalise, const scene::Handle & handle,
    const std::vector<std::uint32_t> & placed, std::uint32_t at, std::map<std::pair<std::uint32_t, bool>, internal::Subtree> & subtrees, internal::Gathered & out) {
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
    const std::pair<std::uint32_t, bool> key { at, made.twoSided };
    auto subtree = subtrees.find(key);
    if (subtree == subtrees.end()) {
        subtree = subtrees.emplace(key, subtreeOf(prototypes, shape, made.twoSided, out)).first;
    }
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
    if (normalise) {
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
        out.top.push_back(internal::Leaf { .bounds = meshBounds(prototypes, placed, shape, subtree->second, areas, power, made.twoSided),
            .held = static_cast<std::uint32_t>(out.lights.size()) });
    }
    out.lights.push_back(made);
    out.handles.push_back(handle);
    out.powers.push_back(power);
}

/**
 * The scene's mesh lights and its emitting materials, each a light for every chain of instances
 * that places its geometry, appended to what is gathered with their powers. A chain whose
 * geometry a mesh light names makes that light, then the light of its material where its material
 * emits, so the two lie together and add. Each triangle's chance is appended to the triangles,
 * and the first light each instance in the world places to the meshes. Each geometry that emits
 * has one subtree, which each light that places it leads into from the top tree. A geometry with
 * no triangles emits nothing, and a mesh light that names one is reported. named holds the mesh
 * light that makes each geometry emit, by the geometry's index in the scene, and made each light
 * of the scene as it was made, by its index.
 **/
void meshLights(const scene::Scene & scene, const geometry::Prototypes & prototypes, const std::vector<kernel::surface::Material> & materials,
    const std::map<std::uint32_t, scene::Handle> & named, const std::map<std::uint32_t, internal::Made> & made, const boost::shared_ptr<log::Logger> & logger,
    internal::Gathered & out) {
    std::set<std::uint32_t> reported;
    std::map<std::pair<std::uint32_t, bool>, internal::Subtree> subtrees;
    std::vector<std::uint32_t> chain;
    internal::chains(prototypes, prototypes.world.instances, chain, [&](const std::vector<std::uint32_t> & placed, std::uint32_t at) {
        const geometry::Geometry & shape = prototypes.geometries[at];
        const auto naming = named.find(shape.id);
        kernel::light::Light glowing;
        const bool glows =
            internal::materialEmitter(materials, internal::materialOf(prototypes, placed), prototypes.instances[placed.back()].doubleSided, glowing);
        if (naming == named.end() && !glows) {
            return;
        }
        if (shape.form != kernel::geometry::primitive::Form::Triangles) {
            if (naming != named.end() && reported.insert(naming->second.index).second && logger) {
                logger->get()->warn("the mesh light {} names a geometry of no triangles, so it emits nothing", scene.name(naming->second));
            }
            return;
        }
        if (naming != named.end()) {
            const internal::Made & light = made.at(naming->second.index);
            appendMesh(prototypes, light.light, light.normalise, naming->second, placed, at, subtrees, out);
        }
        if (glows) {
            appendMesh(prototypes, glowing, false, scene::Handle {}, placed, at, subtrees, out);
        }
    });
}

/**
 * The scene's volume lights, appended to what is gathered with their powers. A volume whose program
 * emits makes one, and each chain of instances that places a closed surface makes one where the
 * surface's material encloses a medium that emits. Each is an object, which camera rays see, in no
 * group. Its power is four pi times what it emits over its own space, times the volume the world
 * gives a unit of that space as the shutter opens. Its box in the top tree holds its bricks where
 * its chain places them over the shutter, and it emits along every direction. A volume whose chain
 * the prototypes do not hold makes none.
 **/
void volumeLights(const geometry::Prototypes & prototypes, const Shading & shading, internal::Gathered & out) {
    std::map<std::uint32_t, std::uint32_t> byId;
    for (std::size_t i = 0; i < prototypes.instances.size(); ++i) {
        byId.emplace(prototypes.instances[i].id, static_cast<std::uint32_t>(i));
    }
    const auto add = [&](internal::Glowing made, const std::vector<std::uint32_t> & placed) {
        glm::mat4 toWorld { 1.0F };
        for (const std::uint32_t member : placed) {
            toWorld = toWorld * geometry::transformAt(prototypes.instances[member], prototypes.open);
        }
        accel::Box box = accel::moveBox(made.box, made.light.transform);
        for (auto level = placed.rbegin(); level != placed.rend(); ++level) {
            box = placedBox(box, prototypes.instances[*level], prototypes.open, prototypes.close);
        }
        made.light.depth = static_cast<std::uint32_t>(placed.size());
        for (std::size_t level = 0; level < placed.size(); ++level) {
            made.light.chain[level] = prototypes.instances[placed[level]].id;
        }
        const float stretch = std::fabs(glm::determinant(glm::mat3(toWorld * made.light.transform)));
        internal::Leaf leaf { .bounds = internal::everyDirection(box.lower, box.upper), .held = static_cast<std::uint32_t>(out.lights.size()) };
        leaf.bounds.power = kWholeSphere * made.emitted * stretch;
        out.top.push_back(leaf);
        out.lights.push_back(made.light);
        out.handles.push_back(scene::Handle {});
        out.powers.push_back(leaf.bounds.power);
        return leaf.held;
    };
    out.glowing.assign(prototypes.volumes.size(), kernel::geometry::kNoPrimitive);
    for (std::uint32_t volume = 0; volume < prototypes.volumes.size(); ++volume) {
        const kernel::volume::Volume & placed = prototypes.volumes[volume];
        std::vector<std::uint32_t> chain;
        for (std::uint32_t level = 0; level < placed.depth && level < kernel::geometry::kInstanceDepth; ++level) {
            const auto found = byId.find(placed.chain[level]);
            if (found != byId.end()) {
                chain.push_back(found->second);
            }
        }
        internal::Glowing made;
        if (chain.size() == placed.depth && internal::gridGlow(prototypes, shading, volume, out, made)) {
            out.glowing[volume] = add(made, chain);
        }
    }
    std::vector<std::uint32_t> chain;
    internal::chains(prototypes, prototypes.world.instances, chain, [&](const std::vector<std::uint32_t> & placed, std::uint32_t at) {
        const std::uint32_t material = internal::materialOf(prototypes, placed);
        if (material == kernel::surface::kNoMaterial || material >= shading.surfaces.materials.size || !shading.surfaces.materials[material].encloses()) {
            return;
        }
        internal::Glowing made;
        if (internal::enclosedGlow(prototypes, at, shading.surfaces.materials[material].medium.emission, out, made)) {
            out.enclosing.push_back(add(made, placed));
        }
    });
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

/**
 * The names of the light groups of the lights the scene's objects made, sorted and each once,
 * with each light given its group's place among them as its label.
 **/
std::vector<std::string> labelled(const scene::Scene & scene, const std::vector<scene::Handle> & handles, std::vector<kernel::light::Light> & lights) {
    std::vector<std::string> groups(lights.size());
    std::set<std::string> names;
    for (std::size_t i = 0; i < lights.size(); ++i) {
        if (handles[i].valid()) {
            groups[i] = scene.value(handles[i], "lightGroup").stringArray().front();
        }
        if (!groups[i].empty()) {
            names.insert(groups[i]);
        }
    }
    const std::vector<std::string> out(names.begin(), names.end());
    for (std::size_t i = 0; i < lights.size(); ++i) {
        if (!groups[i].empty()) {
            lights[i].label = static_cast<std::uint32_t>(std::lower_bound(out.begin(), out.end(), groups[i]) - out.begin());
        }
    }
    return out;
}

/**
 * The place of each light by the index in the scene of the instance that places its shape, over
 * indices up to most, or none for an index no light's shape has.
 **/
std::vector<std::uint32_t> placesById(const std::vector<kernel::light::Light> & lights, std::uint32_t most) {
    std::vector<std::uint32_t> out(most, kernel::geometry::kNoPrimitive);
    for (std::size_t i = 0; i < lights.size(); ++i) {
        if (lights[i].id != kernel::geometry::kNoPrimitive) {
            out[lights[i].id] = static_cast<std::uint32_t>(i);
        }
    }
    return out;
}

/** Whether a kind is among some. **/
template <std::size_t N>
bool among(const std::array<std::string_view, N> & kinds, const std::string & kind) {
    return std::ranges::find(kinds, kind) != kinds.end();
}

/** The scene's objects of a kind, in the order of their indices. **/
std::vector<scene::Handle> sorted(std::vector<scene::Handle> handles) {
    std::sort(handles.begin(), handles.end(), [](const scene::Handle & left, const scene::Handle & right) { return left.index < right.index; });
    return handles;
}

/** The scene's distant lights and its sphere lights of no shape, in the order of their indices. **/
std::vector<scene::Handle> unshaped(const scene::Scene & scene) {
    std::vector<scene::Handle> out = scene.objects("distantLight");
    for (const scene::Handle & handle : scene.objects("sphereLight")) {
        if (!geometry::shaped(scene, handle)) {
            out.push_back(handle);
        }
    }
    return sorted(out);
}

/** Whether a light of the scene lights only the instances it names. **/
bool linked(const scene::Scene & scene) {
    return std::ranges::any_of(kLightKinds, [&](std::string_view kind) {
        return std::ranges::any_of(scene.objects(std::string(kind)), [&](const scene::Handle & handle) {
            return scene.value(handle, "includeRoot").integerArray().front() == 0;
        });
    });
}

};  // namespace

Lights::Lights(device::Device & device, const scene::Scene & scene, const geometry::Prototypes & prototypes, const colour::Config & config,
    const Shading & shading, float reach, const boost::shared_ptr<log::Logger> & logger)
    : Lights(device, config, logger) {
    update(scene, prototypes, shading, reach);
}

Lights::Lights(device::Device & device, const colour::Config & config, const boost::shared_ptr<log::Logger> & logger)
    : device_(device), config_(config), logger_(logger), read_(config, logger), shapes_(logger) {
}

void Lights::update(const scene::Scene & scene, const geometry::Prototypes & prototypes, const Shading & shading, float reach) {
    madeLights_.clear();
    const bool linking = linked(scene);
    const bool volumes = !prototypes.volumes.empty();
    // a light with a shape, and a point, is measured as the shutter opens
    const bool shutter = prototypes.open != open_ || prototypes.close != close_;
    bool stale = !built_ || shutter || reach != reach_;
    if (built_ && shutter) {
        made_.clear();
    }
    if (built_) {
        for (const scene::Change & change : scene.changes(version_)) {
            const std::string & kind = scene.schema().kind(change.handle.kind).name;
            if (among(kLightKinds, kind)) {
                made_.erase(change.handle.index);
                stale = true;
            } else if ((linking || linked_ || volumes || volumes_) && !among(kQuietKinds, kind)) {
                stale = true;
            }
        }
    }

    stale = make(scene, prototypes) || stale;

    const std::vector<kernel::surface::Material> materials = geometry::ConvertedMaterials::of(prototypes, config_).materials;
    const std::map<std::uint32_t, scene::Handle> named = internal::meshEmitters(scene);
    std::vector<std::uint8_t> emitting = internal::emitting(prototypes, materials, named, shading);
    stale = stale || emitting != emitting_;
    if (stale) {
        place(scene, prototypes, shading, reach, materials, named);
    }
    builtTree_ = stale;
    emitting_ = std::move(emitting);
    version_ = scene.version();
    built_ = true;
    reach_ = reach;
    open_ = prototypes.open;
    close_ = prototypes.close;
    linked_ = linking;
    volumes_ = volumes;
}

bool Lights::make(const scene::Scene & scene, const geometry::Prototypes & prototypes) {
    std::map<std::uint32_t, internal::Made> made;
    const auto keep = [&](const scene::Handle & handle, const auto & build) {
        const auto held = made_.find(handle.index);
        if (held != made_.end() && held->second.handle == handle) {
            made.emplace(handle.index, held->second);
            return;
        }
        made.emplace(handle.index, build());
        madeLights_.push_back(handle.index);
    };
    for (const geometry::ShapedLight & shaped : prototypes.lights) {
        keep(shaped.handle, [&] { return internal::shapedLight(scene, prototypes, shaped, config_, read_, shapes_); });
    }
    for (const scene::Handle & handle : unshaped(scene)) {
        keep(handle, [&] {
            return scene.schema().kind(handle.kind).name == "sphereLight" ? internal::pointLight(scene, handle, prototypes.open, config_, shapes_)
                                                                           : internal::distantLight(scene, handle, config_);
        });
    }
    for (const scene::Handle & handle : sorted(scene.objects("domeLight"))) {
        keep(handle, [&] { return internal::domeLight(scene, handle, config_, read_); });
    }
    for (const scene::Handle & handle : sorted(scene.objects("meshLight"))) {
        keep(handle, [&] { return internal::meshLight(scene, handle, config_); });
    }
    const bool changed = !madeLights_.empty() || made.size() != made_.size();
    made_ = std::move(made);
    return changed;
}

void Lights::place(const scene::Scene & scene, const geometry::Prototypes & prototypes, const Shading & shading, float reach,
    const std::vector<kernel::surface::Material> & materials, const std::map<std::uint32_t, scene::Handle> & named) {
    internal::Gathered all;
    std::vector<kernel::light::Light> & lights = all.lights;
    std::vector<kernel::geometry::Key> keys;
    std::vector<std::uint32_t> distant;
    std::vector<std::uint32_t> domes;
    // the images and the profiles the lights read, in order, with the places of the lights
    std::vector<internal::Textures::Use> imageUses;
    std::vector<std::uint32_t> imaged;
    std::vector<std::uint32_t> profileUses;
    std::vector<std::uint32_t> profiled;
    const auto add = [&](const internal::Made & made, const geometry::Instance & placed) {
        kernel::light::Light light = made.light;
        light.transform = placed.transform;
        light.keys = static_cast<std::uint32_t>(keys.size());
        light.keyCount = static_cast<std::uint32_t>(placed.keys.size());
        keys.insert(keys.end(), placed.keys.begin(), placed.keys.end());
        const float power = std::fmax(0.0F, powerOf(light, made.size, reach) * kernel::spectrum::acescgToXyz(light.colour).y * made.shaping);
        const auto place = static_cast<std::uint32_t>(lights.size());
        if (light.shape == kernel::light::Shape::Distant || light.shape == kernel::light::Shape::Dome) {
            all.infinite.push_back(place);
        } else {
            all.top.push_back(internal::Leaf { .bounds = boundsOf(light, placed, power, prototypes.open, prototypes.close), .held = place });
        }
        if (made.read != kernel::geometry::kNoPrimitive) {
            // a dome draws its texels only when it emits by its image
            imageUses.push_back(internal::Textures::Use { .image = made.read, .drawn = light.shape == kernel::light::Shape::Dome && light.image == made.read });
            imaged.push_back(place);
        }
        if (light.profile != kernel::geometry::kNoPrimitive) {
            profileUses.push_back(light.profile);
            profiled.push_back(place);
        }
        lights.push_back(light);
        all.powers.push_back(power);
        all.handles.push_back(made.handle);
    };
    std::uint32_t most = 0;
    for (const geometry::ShapedLight & shaped : prototypes.lights) {
        const geometry::Instance & instance = prototypes.instances[shaped.instance];
        add(made_.at(shaped.handle.index), instance);
        most = std::max(most, instance.id + 1U);
    }
    for (const scene::Handle & handle : unshaped(scene)) {
        const internal::Made & made = made_.at(handle.index);
        if (made.light.shape == kernel::light::Shape::Distant && made.light.size.x > 0.0F) {
            distant.push_back(static_cast<std::uint32_t>(lights.size()));
        }
        add(made, internal::placement(scene, handle));
    }
    for (const scene::Handle & handle : sorted(scene.objects("domeLight"))) {
        domes.push_back(static_cast<std::uint32_t>(lights.size()));
        add(made_.at(handle.index), internal::placement(scene, handle));
    }
    // each light reads its image and its profile at its place among those the lights read
    const internal::Textures::Packed images = read_.pack(imageUses);
    for (std::size_t use = 0; use < imaged.size(); ++use) {
        kernel::light::Light & light = lights[imaged[use]];
        light.image = light.image == kernel::geometry::kNoPrimitive ? light.image : images.places[use];
    }
    const internal::Profiles::Packed profiles = shapes_.pack(profileUses);
    for (std::size_t use = 0; use < profiled.size(); ++use) {
        lights[profiled[use]].profile = static_cast<std::uint32_t>(use);
    }
    meshLights(scene, prototypes, materials, named, made_, logger_, all);
    volumeLights(prototypes, shading, all);
    labels_ = labelled(scene, all.handles, lights);
    const internal::Linking links = internal::linking(scene, prototypes, all.handles, lights);
    treesBySet(links, all);
    const std::vector<std::uint32_t> byId = placesById(lights, most);
    lights_ = uploaded(device_, lights);
    keys_ = uploaded(device_, keys);
    distant_ = uploaded(device_, distant);
    byId_ = uploaded(device_, byId);
    cdf_ = uploaded(device_, cumulative(all.powers));
    triangles_ = uploaded(device_, all.triangles);
    meshes_ = uploaded(device_, all.meshes);
    images_ = uploaded(device_, images.images);
    texels_ = uploaded(device_, images.texels);
    sums_ = uploaded(device_, images.sums);
    domes_ = uploaded(device_, domes);
    nodes_ = uploaded(device_, all.nodes);
    roots_ = uploaded(device_, all.roots);
    leaves_ = uploaded(device_, all.leaves);
    triangleLeaves_ = uploaded(device_, all.triangleLeaves);
    infinite_ = uploaded(device_, all.afar);
    infiniteFirst_ = uploaded(device_, all.afarFirst);
    bricks_ = uploaded(device_, all.bricks);
    cells_ = uploaded(device_, all.cells);
    glowing_ = uploaded(device_, all.glowing);
    enclosing_ = uploaded(device_, all.enclosing);
    linkFirst_ = uploaded(device_, links.first);
    linkSteps_ = uploaded(device_, links.steps);
    lit_ = uploaded(device_, links.lit);
    casters_ = uploaded(device_, links.casters);
    profiles_ = uploaded(device_, profiles.profiles);
    photometry_ = uploaded(device_, profiles.photometry);
    count_ = static_cast<std::uint32_t>(lights.size());
}

const std::vector<std::string> & Lights::labels() const {
    return labels_;
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
        .bricks = device::view<kernel::light::Brick>(*bricks_),
        .cells = device::view<float>(*cells_),
        .glowing = device::view<std::uint32_t>(*glowing_),
        .enclosing = device::view<std::uint32_t>(*enclosing_),
        .links = kernel::light::link::Links { .first = device::view<std::uint32_t>(*linkFirst_),
            .steps = device::view<kernel::light::link::LinkStep>(*linkSteps_),
            .lit = device::view<std::uint8_t>(*lit_),
            .count = count_,
            .casters = device::view<std::uint32_t>(*casters_) },
        .profiles = device::view<kernel::light::Profile>(*profiles_),
        .photometry = device::view<float>(*photometry_) };
}

};  // namespace v3d::moya::light
