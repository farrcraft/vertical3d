/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/internal/Glowing.h>

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/accel/Sweep.h>
#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/integrate/media/Crossings.h>
#include <moya/libmoya/kernel/light/Brick.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/volume/Medium.h>
#include <moya/libmoya/kernel/volume/Volume.h>
#include <moya/libmoya/light/internal/EmittingBrick.h>
#include <moya/libmoya/light/internal/Gathered.h>
#include <moya/libmoya/light/internal/Tree.h>
#include <moya/libmoya/light/Shading.h>
#include <moya/libmoya/volume/Grid.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::light::internal {

namespace {

glm::vec3 fromNano(const nanovdb::Vec3f & value) {
    return glm::vec3 { value[0], value[1], value[2] };
}

/** The transformation that takes a grid's index space into its volume's own space. **/
glm::mat4 indexToOwn(const nanovdb::FloatGrid & grid) {
    glm::mat4 out { 1.0F };
    for (int axis = 0; axis < 3; ++axis) {
        nanovdb::Vec3f unit(0.0F);
        unit[axis] = 1.0F;
        out[axis] = glm::vec4(fromNano(grid.indexToWorldDirF(unit)), 0.0F);
    }
    out[3] = glm::vec4(fromNano(grid.indexToWorldF(nanovdb::Vec3f(0.0F))), 1.0F);
    return out;
}

/**
 * Appends the sums of the shares of weights up to and including each, the last held to one, as
 * Lights::drawnFrom() reads them.
 **/
void appendSums(const std::vector<float> & weights, std::vector<float> & into) {
    double total = 0.0;
    for (const float weight : weights) {
        total += weight;
    }
    double sum = 0.0;
    for (const float weight : weights) {
        sum += weight;
        into.push_back(static_cast<float>(sum / total));
    }
    into.back() = 1.0F;
}

/** The luminance of an emission in ACEScg, or zero for one below it. **/
float luminanceOf(const glm::vec3 & emission) {
    return std::fmax(kernel::spectrum::acescgToXyz(emission).y, 0.0F);
}

/** Appends the corner of each active tile of a lower node, each the size of a leaf. **/
void lowerTiles(const nanovdb::NanoLower<float> & node, std::vector<nanovdb::Coord> & out) {
    for (std::uint32_t slot = 0; slot < nanovdb::NanoLower<float>::SIZE; ++slot) {
        if (!node.getChildMask().isOn(slot) && node.getValueMask().isOn(slot)) {
            out.push_back(node.offsetToGlobalCoord(slot));
        }
    }
}

/** Appends the corner of each part the size of a leaf of each active tile of an upper node. **/
void upperTiles(const nanovdb::NanoUpper<float> & node, std::vector<nanovdb::Coord> & out) {
    const auto side = static_cast<std::int32_t>(kernel::light::kBrickSide);
    const auto across = static_cast<std::int32_t>(nanovdb::NanoLower<float>::DIM);
    for (std::uint32_t slot = 0; slot < nanovdb::NanoUpper<float>::SIZE; ++slot) {
        if (node.getChildMask().isOn(slot) || !node.getValueMask().isOn(slot)) {
            continue;
        }
        const nanovdb::Coord corner = node.offsetToGlobalCoord(slot);
        for (std::int32_t z = 0; z < across; z += side) {
            for (std::int32_t y = 0; y < across; y += side) {
                for (std::int32_t x = 0; x < across; x += side) {
                    out.push_back(corner + nanovdb::Coord(x, y, z));
                }
            }
        }
    }
}

/**
 * The lower corner of each brick of a grid. A leaf node is a brick, and so is an active tile of a
 * lower node, which is a leaf's size. An active tile of an upper node is cut into bricks of that
 * size. A root's tiles are left out.
 **/
std::vector<nanovdb::Coord> brickCorners(const nanovdb::FloatGrid & grid) {
    std::vector<nanovdb::Coord> out;
    const nanovdb::NanoTree<float> & tree = grid.tree();
    const nanovdb::NanoLeaf<float> * leaves = tree.getFirstLeaf();
    out.reserve(tree.nodeCount(0));
    for (std::uint32_t i = 0; i < tree.nodeCount(0); ++i) {
        out.push_back(leaves[i].origin());
    }
    const nanovdb::NanoLower<float> * lowers = tree.getFirstLower();
    for (std::uint32_t i = 0; i < tree.nodeCount(1); ++i) {
        lowerTiles(lowers[i], out);
    }
    const nanovdb::NanoUpper<float> * uppers = tree.getFirstUpper();
    for (std::uint32_t i = 0; i < tree.nodeCount(2); ++i) {
        upperTiles(uppers[i], out);
    }
    return out;
}

/** Every brick of a volume's walked grid that emits, as it stands when the shutter opens. **/
std::vector<EmittingBrick> emitting(const geometry::Prototypes & prototypes, const Shading & shading, const kernel::volume::Volume & placed,
    const nanovdb::FloatGrid & grid, const glm::mat4 & toWorld) {
    std::vector<EmittingBrick> out;
    const auto side = static_cast<std::int32_t>(kernel::light::kBrickSide);
    for (const nanovdb::Coord & origin : brickCorners(grid)) {
        EmittingBrick found;
        found.weights.resize(kernel::light::kBrickCells);
        for (std::int32_t z = 0; z < side; ++z) {
            for (std::int32_t y = 0; y < side; ++y) {
                for (std::int32_t x = 0; x < side; ++x) {
                    const nanovdb::Vec3f centre(static_cast<float>(origin[0] + x) + 0.5F, static_cast<float>(origin[1] + y) + 0.5F,
                        static_cast<float>(origin[2] + z) + 0.5F);
                    const kernel::volume::Medium medium = kernel::integrate::media::mediumInside(shading.surfaces, shading.textures, shading.volumes, placed,
                        fromNano(grid.indexToWorldF(centre)), toWorld, prototypes.open);
                    const float weight = luminanceOf(medium.emission);
                    const auto along = static_cast<std::size_t>(side);
                    const auto cell = static_cast<std::size_t>(x) + (along * (static_cast<std::size_t>(y) + (along * static_cast<std::size_t>(z))));
                    found.weights[cell] = weight;
                    found.sum += weight;
                }
            }
        }
        if (found.sum > 0.0F) {
            found.brick.lower = glm::vec3 { static_cast<float>(origin[0]), static_cast<float>(origin[1]), static_cast<float>(origin[2]) };
            found.brick.upper = found.brick.lower + glm::vec3(static_cast<float>(side));
            out.push_back(found);
        }
    }
    std::sort(out.begin(), out.end(), [](const EmittingBrick & left, const EmittingBrick & right) {
        const glm::vec3 & first = left.brick.lower;
        const glm::vec3 & second = right.brick.lower;
        if (first.z != second.z) {
            return first.z < second.z;
        }
        return first.y != second.y ? first.y < second.y : first.x < second.x;
    });
    return out;
}

/** A volume light of no chain, which emits from bricks of its own space along every direction. **/
kernel::light::Light glowingLight() {
    kernel::light::Light out;
    out.id = kernel::geometry::kNoPrimitive;
    out.shape = kernel::light::Shape::Volume;
    out.camera = true;
    out.object = true;
    return out;
}

};  // namespace

bool gridGlow(const geometry::Prototypes & prototypes, const Shading & shading, std::uint32_t volume, Gathered & out, Glowing & made) {
    const kernel::volume::Volume & placed = prototypes.volumes[volume];
    const bool programmed = placed.material < shading.surfaces.materials.size && shading.surfaces.materials[placed.material].mediumProgram.size != 0U;
    if (placed.fieldCount == 0U || !programmed) {
        return false;
    }
    const volume::Grid & held = *prototypes.grids[placed.fields[placed.walked].grid];
    glm::mat4 toWorld { 1.0F };
    if (held.blocks.empty() || !kernel::integrate::media::placedAt(shading.surfaces, placed, prototypes.open, toWorld)) {
        return false;
    }
    // a NanoVDB buffer starts with its grid
    const auto & grid = *reinterpret_cast<const nanovdb::FloatGrid *>(held.blocks.data());
    const std::vector<EmittingBrick> found = emitting(prototypes, shading, placed, grid, toWorld);
    if (found.empty()) {
        return false;
    }
    made = Glowing {};
    made.light = glowingLight();
    made.light.geometry = volume;
    made.light.transform = indexToOwn(grid);
    made.light.bricks = static_cast<std::uint32_t>(out.bricks.size());
    made.light.count = static_cast<std::uint32_t>(found.size());
    made.light.triangles = static_cast<std::uint32_t>(out.triangles.size());
    std::vector<float> sums;
    std::vector<Leaf> leaves;
    for (std::size_t i = 0; i < found.size(); ++i) {
        kernel::light::Brick brick = found[i].brick;
        brick.cells = static_cast<std::uint32_t>(out.cells.size());
        appendSums(found[i].weights, out.cells);
        out.bricks.push_back(brick);
        sums.push_back(found[i].sum);
        Leaf leaf { .bounds = everyDirection(brick.lower, brick.upper), .held = static_cast<std::uint32_t>(i) };
        leaf.bounds.power = found[i].sum;
        leaves.push_back(leaf);
        made.box.grow(brick.lower);
        made.box.grow(brick.upper);
        made.emitted += found[i].sum;
    }
    appendSums(sums, out.triangles);
    std::vector<std::uint32_t> leafOf(found.size(), kernel::geometry::kNoPrimitive);
    made.light.tree = build(leaves, out.nodes, leafOf, 0U);
    for (std::size_t i = 0; i < found.size(); ++i) {
        out.bricks[made.light.bricks + i].leaf = leafOf[i];
    }
    return true;
}

bool enclosedGlow(const geometry::Prototypes & prototypes, std::uint32_t geometry, const glm::vec3 & emission, Gathered & out, Glowing & made) {
    const float luminance = luminanceOf(emission);
    if (!(luminance > 0.0F)) {
        return false;
    }
    const geometry::Geometry & shape = prototypes.geometries[geometry];
    accel::Box box;
    if (shape.form == kernel::geometry::primitive::Form::Triangles) {
        const accel::Sweep sweep = accel::triangleSweep(shape.points, prototypes.points, prototypes.times);
        for (std::uint32_t triangle = 0; triangle < shape.count; ++triangle) {
            box.grow(sweep(triangle, prototypes.open, prototypes.close).swept());
        }
    } else if (shape.form == kernel::geometry::primitive::Form::Quadric) {
        box = accel::quadricSweep(shape.quadric, prototypes.quadrics, prototypes.times, prototypes.open, prototypes.close).swept();
    } else {
        return false;
    }
    const glm::vec3 size = box.upper - box.lower;
    const float volume = size.x * size.y * size.z;
    if (box.empty() || !(volume > 0.0F)) {
        return false;
    }
    made = Glowing {};
    made.light = glowingLight();
    made.light.geometry = kernel::geometry::kNoPrimitive;
    made.light.bricks = static_cast<std::uint32_t>(out.bricks.size());
    made.light.count = 1U;
    made.light.triangles = static_cast<std::uint32_t>(out.triangles.size());
    out.triangles.push_back(1.0F);
    out.bricks.push_back(kernel::light::Brick { .lower = box.lower, .upper = box.upper });
    made.box = box;
    made.emitted = luminance * volume;
    return true;
}

};  // namespace v3d::moya::light::internal
