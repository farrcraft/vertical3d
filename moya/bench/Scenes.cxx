/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/bench/Scenes.h>

#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/Exr.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <openvdb/openvdb.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <glm/common.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::bench {

namespace {

/** The seed every scene's random values are drawn from, so each run builds the same scenes. **/
constexpr std::uint32_t kSeed = 20261109;

/** How many waves a ripple runs across x and across y, over the grid from -1 to 1. **/
constexpr float kWavesAcross = 9.0F;
constexpr float kWavesUp = 7.0F;

/** How high a ground ripples, and how high a patch does in its own space. **/
constexpr float kGroundRipple = 0.05F;
constexpr float kPatchRipple = 0.5F;

/** The cells to a side of each grid: small triangles, the instanced patch, and the moving ones. **/
constexpr std::int32_t kSmallCells = 724;
constexpr std::int32_t kPatchCells = 100;
constexpr std::int32_t kGroundCells = 256;
constexpr std::int32_t kMovingCells = 32;

/** The large triangles: how many, and how far they reach above and below z = 0. **/
constexpr std::int32_t kLargeTriangles = 32;
constexpr float kLargeReach = 0.5F;

/**
 * The curves: how many at full scale and at least, their vertices, how far a tip leans and how
 * high it stands, and their width.
 **/
constexpr float kCurves = 32768.0F;
constexpr std::int32_t kFewestCurves = 64;
constexpr std::int32_t kCurveVertices = 6;
constexpr float kLean = 0.05F;
constexpr float kCurveHeight = 0.3F;
constexpr float kCurveWidth = 0.003F;

/** How many tiles to a side the instanced patches make, and how many moving patches. **/
constexpr std::int32_t kTileSide = 16;
constexpr std::int32_t kMovingSide = 8;

/** How far the moving ground rises over the shutter, and how high the moving patches pass. **/
constexpr float kRise = 0.1F;
constexpr float kPassHeight = 0.2F;

/** A moving patch's size against the cell it crosses from. **/
constexpr float kMovingSize = 0.4F;

/** How far each scene's box reaches below and above z = 0, beyond what it holds. **/
constexpr float kRippledBox = 0.1F;
constexpr float kCurvesBox = 0.4F;
constexpr float kMovingBox = 0.3F;

/**
 * The lit room: the camera's field of view and height above the floor, the walls' height, the
 * size of the rect and disk lights, and the sphere light's radius and height.
 **/
constexpr float kRoomFov = 60.0F;
constexpr float kRoomEye = 2.6F;
constexpr float kWallHeight = 0.8F;
constexpr float kLightSize = 0.4F;
constexpr float kSphereRadius = 0.1F;
constexpr float kSphereHeight = 0.4F;

/** A half turn, in degrees, which turns the camera to look down. **/
constexpr float kHalfTurn = 180.0F;

/** Where the rect, disk and sphere lights hang over the floor, as x and y. **/
constexpr float kLightsAt[3][2] = { { 0.4F, 0.4F }, { -0.4F, 0.5F }, { -0.5F, -0.3F } };

/** How many times the textured room's image runs across its floor along x and along y. **/
constexpr float kTextureRepeats = 4.0F;

/**
 * The textured room's image. Its texels to a side, in a full run and in a short pass. Its tiles to
 * a side and their two colours. How many rings shade a tile, and the darkest a ring makes it.
 **/
constexpr std::uint32_t kImageSide = 1024;
constexpr std::uint32_t kQuickImageSide = 64;
constexpr float kImageTiles = 8.0F;
constexpr glm::vec3 kImageLight { 0.8F, 0.7F, 0.5F };
constexpr glm::vec3 kImageDark { 0.25F, 0.3F, 0.4F };
constexpr float kImageRings = 12.0F;
constexpr float kImageShade = 0.6F;

/** The roughness of the textured room's coat. **/
constexpr float kCoatRoughness = 0.1F;

/**
 * The fog room's grid: its voxels along the floor's side, in a full run and in a short pass. Its
 * density at the floor, the height over which it falls to 1/e, how much it varies across the
 * floor and in how many waves. The coefficients its density scatters and absorbs by.
 **/
constexpr std::int32_t kFogCells = 128;
constexpr std::int32_t kQuickFogCells = 8;
constexpr float kFogDensity = 4.0F;
constexpr float kFogFalloff = 0.25F;
constexpr float kFogVariation = 0.5F;
constexpr float kFogWaves = 5.0F;
constexpr float kFogScattering = 0.9F;
constexpr float kFogAbsorption = 0.1F;

/**
 * The subsurface room's statue: the radius of each ball from the bottom, its subsurface colour,
 * and its subsurface radius, the mean free path.
 **/
constexpr float kStatueRadii[] = { 0.3F, 0.2F, 0.12F };
constexpr glm::vec3 kStatueColour { 0.9F, 0.85F, 0.8F };
constexpr float kStatueRadius = 0.1F;

/**
 * The hair lock: how many strands at a scale of 1 and at least, the vertices of each, the band
 * they hang from, how far they fall and sway, and their width. The camera's field of view, how
 * far it stands from the lock and its height. The key light's and the dome's intensity, and the
 * hair's absorption.
 **/
constexpr float kStrands = 16384.0F;
constexpr std::int32_t kFewestStrands = 64;
constexpr std::int32_t kStrandVertices = 8;
constexpr float kBandWide = 0.15F;
constexpr float kBandDeep = 0.05F;
constexpr float kBandHeight = 0.9F;
constexpr float kFall = 0.85F;
constexpr float kSway = 0.12F;
constexpr float kStrandWidth = 0.004F;
constexpr float kLockFov = 40.0F;
constexpr float kLockDistance = 2.5F;
constexpr float kLockEye = 0.45F;
constexpr float kKeyIntensity = 2.0F;
constexpr float kLockDome = 0.2F;
constexpr glm::vec3 kHairAbsorption { 0.4F, 0.7F, 1.4F };

/** A quarter turn, in degrees. **/
constexpr float kQuarterTurn = 90.0F;

/** How many cells a side of a grid has at a scale, at least two. **/
std::int32_t cellsAt(std::int32_t full, float scale) {
    return std::max(2, static_cast<std::int32_t>(std::lround(static_cast<float>(full) * std::sqrt(scale))));  // checked: full and scale are the benchmark's own, small and finite
}

/** A height that ripples over x and y, of an amplitude. **/
float ripple(float x, float y, float amplitude) {
    return amplitude * std::sin(kWavesAcross * x) * std::cos(kWavesUp * y);
}

/**
 * A mesh of a grid of quads over x and y from -1 to 1, cells to a side, rippled to an amplitude.
 * It has a sample for each of a list of rises, raised by it, at times spread over the shutter.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cells to a side, then the ripple
scene::Handle grid(scene::Scene & scene, const std::string & name, std::int32_t cells, float amplitude, const std::vector<float> & rises) {
    const scene::Handle out = scene.create("mesh", name);
    const std::int32_t side = cells + 1;
    std::vector<std::int32_t> indices;
    indices.reserve(static_cast<std::size_t>(cells) * static_cast<std::size_t>(cells) * 4U);
    for (std::int32_t row = 0; row < cells; ++row) {
        for (std::int32_t column = 0; column < cells; ++column) {
            const std::int32_t first = (row * side) + column;
            indices.insert(indices.end(), { first, first + 1, first + side + 1, first + side });
        }
    }
    std::vector<scene::Value> samples;
    for (const float rise : rises) {
        std::vector<glm::vec3> points;
        points.reserve(static_cast<std::size_t>(side) * static_cast<std::size_t>(side));
        for (std::int32_t row = 0; row < side; ++row) {
            for (std::int32_t column = 0; column < side; ++column) {
                const float x = -1.0F + (2.0F * static_cast<float>(column) / static_cast<float>(cells));
                const float y = -1.0F + (2.0F * static_cast<float>(row) / static_cast<float>(cells));
                points.emplace_back(x, y, ripple(x, y, amplitude) + rise);
            }
        }
        samples.push_back(scene::Value::points(points));
    }
    if (samples.size() == 1) {
        scene.set(out, "P", samples.front());
    } else {
        std::vector<float> times;
        times.reserve(samples.size());
        for (std::size_t sample = 0; sample < samples.size(); ++sample) {
            times.push_back(static_cast<float>(sample) / static_cast<float>(samples.size() - 1));
        }
        scene.set(out, "P", times, samples);
    }
    scene.set(out, "vertices", scene::Value::integers(std::vector<std::int32_t>(static_cast<std::size_t>(cells) * static_cast<std::size_t>(cells), 4)));
    scene.set(out, "indices", scene::Value::integers(indices));
    return out;
}

/** An instance of a prototype in a group, or in the world, at its placements over the shutter. **/
void place(scene::Scene & scene, const std::string & name, const scene::Handle & prototype, const scene::Handle & group,
    const std::vector<glm::mat4> & placements) {
    const scene::Handle instance = scene.create("instance", name);
    scene.set(instance, "prototype", scene::Value::handle(prototype));
    if (group.valid()) {
        scene.set(instance, "group", scene::Value::handle(group));
    }
    if (placements.size() == 1) {
        scene.set(instance, "transform", scene::Value::matrix(placements.front()));
        return;
    }
    std::vector<float> times;
    std::vector<scene::Value> values;
    for (std::size_t sample = 0; sample < placements.size(); ++sample) {
        times.push_back(static_cast<float>(sample) / static_cast<float>(placements.size() - 1));
        values.push_back(scene::Value::matrix(placements[sample]));
    }
    scene.set(instance, "transform", times, values);
}

/** A placement that scales by a factor and then moves by an offset. **/
glm::mat4 placed(const glm::vec3 & offset, float factor) {
    return glm::scale(glm::translate(glm::mat4(1.0F), offset), glm::vec3 { factor });
}

/** The middle of a cell of a grid of cells to a side over x and y from -1 to 1, at a height. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cell across, then up, as x and y run
glm::vec3 middleOf(std::int32_t column, std::int32_t row, std::int32_t side, float height) {
    const float size = 2.0F / static_cast<float>(side);
    return glm::vec3 { -1.0F + (size * (static_cast<float>(column) + 0.5F)), -1.0F + (size * (static_cast<float>(row) + 0.5F)), height };
}

/** A camera whose shutter opens at 0 and closes at 1, named by the scene's options. **/
void shutter(scene::Scene & scene) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(camera, "shutter", scene::Value::floats({ 0.0F, 1.0F }));
}

/** A scene of a name, empty, over x and y from -1 to 1 and z from low to high. **/
Fixed empty(const std::string & name, float low, float high) {
    return Fixed { .name = name, .scene = boost::make_shared<scene::Scene>(scene::kinds()), .low = { -1.0F, -1.0F, low }, .high = { 1.0F, 1.0F, high } };
}

Fixed smallTriangles(float scale) {
    Fixed out = empty("small triangles", -kRippledBox, kRippledBox);
    place(*out.scene, "grid", grid(*out.scene, "grid", cellsAt(kSmallCells, scale), kGroundRipple, { 0.0F }), {}, { glm::mat4(1.0F) });
    return out;
}

Fixed largeTriangles() {
    Fixed out = empty("large triangles", -kLargeReach, kLargeReach);
    // NOLINTNEXTLINE(bugprone-random-generator-seed) - every run builds the same scene
    std::mt19937 random(kSeed);
    std::uniform_real_distribution<float> across(-1.0F, 1.0F);
    std::uniform_real_distribution<float> up(-kLargeReach, kLargeReach);
    std::vector<glm::vec3> points;
    std::vector<std::int32_t> indices;
    for (std::int32_t corner = 0; corner < kLargeTriangles * 3; ++corner) {
        points.emplace_back(across(random), across(random), up(random));
        indices.push_back(corner);
    }
    const scene::Handle mesh = out.scene->create("mesh", "triangles");
    out.scene->set(mesh, "P", scene::Value::points(points));
    out.scene->set(mesh, "vertices", scene::Value::integers(std::vector<std::int32_t>(kLargeTriangles, 3)));
    out.scene->set(mesh, "indices", scene::Value::integers(indices));
    place(*out.scene, "triangles", mesh, {}, { glm::mat4(1.0F) });
    return out;
}

Fixed curves(float scale) {
    Fixed out = empty("curves", 0.0F, kCurvesBox);
    // NOLINTNEXTLINE(bugprone-random-generator-seed) - every run builds the same scene
    std::mt19937 random(kSeed);
    std::uniform_real_distribution<float> across(-1.0F, 1.0F);
    std::uniform_real_distribution<float> lean(-kLean, kLean);
    const auto count = std::max(kFewestCurves, static_cast<std::int32_t>(kCurves * scale));
    std::vector<glm::vec3> points;
    for (std::int32_t curve = 0; curve < count; ++curve) {
        const glm::vec3 root { across(random), across(random), 0.0F };
        const glm::vec3 tip { lean(random), lean(random), kCurveHeight };
        for (std::int32_t vertex = 0; vertex < kCurveVertices; ++vertex) {
            const float along = static_cast<float>(vertex) / static_cast<float>(kCurveVertices - 1);
            points.push_back(root + (tip * glm::vec3 { along * along, along * along, along }));
        }
    }
    const scene::Handle hair = out.scene->create("curves", "grass");
    out.scene->set(hair, "P", scene::Value::points(points));
    out.scene->set(hair, "vertices", scene::Value::integers(std::vector<std::int32_t>(static_cast<std::size_t>(count), kCurveVertices)));
    out.scene->set(hair, "type", scene::Value::string("cubic"));
    out.scene->set(hair, "basis", scene::Value::string("b-spline"));
    out.scene->set(hair, "constantWidth", scene::Value::number(kCurveWidth));
    out.scene->set(hair, "profile", scene::Value::string("round"));
    place(*out.scene, "grass", hair, {}, { glm::mat4(1.0F) });
    return out;
}

Fixed instances(float scale) {
    Fixed out = empty("instances", -kRippledBox, kRippledBox);
    scene::Scene & made = *out.scene;
    const scene::Handle patch = grid(made, "patch", cellsAt(kPatchCells, scale), kPatchRipple, { 0.0F });
    // a tile of four patches, each a quarter of it
    const scene::Handle tile = made.create("group", "tile");
    for (std::int32_t quarter = 0; quarter < 4; ++quarter) {
        place(made, "patch " + std::to_string(quarter), patch, tile, { placed(middleOf(quarter % 2, quarter / 2, 2, 0.0F), 0.5F) });
    }
    for (std::int32_t row = 0; row < kTileSide; ++row) {
        for (std::int32_t column = 0; column < kTileSide; ++column) {
            const float half = 1.0F / static_cast<float>(kTileSide);
            place(made, "tile " + std::to_string(row) + " " + std::to_string(column), tile, {}, { placed(middleOf(column, row, kTileSide, 0.0F), half) });
        }
    }
    return out;
}

Fixed motion(float scale) {
    Fixed out = empty("motion", -kRippledBox, kMovingBox);
    scene::Scene & made = *out.scene;
    shutter(made);
    place(made, "ground", grid(made, "ground", cellsAt(kGroundCells, scale), kGroundRipple, { 0.0F, kRise }), {}, { glm::mat4(1.0F) });
    // patches that each cross to the next cell over the shutter
    const scene::Handle patch = grid(made, "patch", cellsAt(kMovingCells, scale), kPatchRipple, { 0.0F });
    const float size = 2.0F / static_cast<float>(kMovingSide);
    for (std::int32_t row = 0; row < kMovingSide; ++row) {
        for (std::int32_t column = 0; column < kMovingSide; ++column) {
            const glm::vec3 from = middleOf(column, row, kMovingSide, kPassHeight);
            const glm::vec3 to = from + glm::vec3 { size, 0.0F, 0.0F };
            const float factor = size * kMovingSize;
            place(made, "patch " + std::to_string(row) + " " + std::to_string(column), patch, {}, { placed(from, factor), placed(to, factor) });
        }
    }
    return out;
}

/**
 * The lit room, uncommitted: its floor, walls, lights and camera, with no material. The floor's
 * st runs over it kTextureRepeats times along x and along y.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the scale, then the side and samples
boost::shared_ptr<scene::Scene> room(float scale, std::uint32_t side, std::uint32_t samples) {
    const boost::shared_ptr<scene::Scene> out = boost::make_shared<scene::Scene>(scene::kinds());
    scene::Scene & made = *out;
    const scene::Handle options = made.create("options", "options");
    const scene::Handle camera = made.create("camera", "camera");
    made.set(options, "camera", scene::Value::handle(camera));
    made.set(options, "resolution", scene::Value::integers({ static_cast<std::int32_t>(side), static_cast<std::int32_t>(side) }));
    made.set(options, "samples", scene::Value::integer(static_cast<std::int32_t>(samples)));
    made.set(camera, "fov", scene::Value::number(kRoomFov));
    // the camera looks along its +z, so it is turned half a turn to look down
    made.set(camera, "transform",
        scene::Value::matrix(glm::rotate(glm::translate(glm::mat4(1.0F), glm::vec3 { 0.0F, 0.0F, kRoomEye }), glm::radians(kHalfTurn), glm::vec3 { 1.0F, 0.0F, 0.0F })));

    const std::int32_t cells = cellsAt(kSmallCells, scale);
    const scene::Handle floor = grid(made, "floor", cells, kGroundRipple, { 0.0F });
    std::vector<float> st;
    for (std::int32_t row = 0; row <= cells; ++row) {
        for (std::int32_t column = 0; column <= cells; ++column) {
            st.push_back(kTextureRepeats * static_cast<float>(column) / static_cast<float>(cells));
            st.push_back(kTextureRepeats * static_cast<float>(row) / static_cast<float>(cells));
        }
    }
    const scene::Value spread = scene::Value::floats(st);
    made.declare(floor, scene::Declaration { .name = "st", .type = spread.type(), .count = scene::Declaration::kAny, .width = 2, .fallback = spread,
                            .interpolation = scene::Interpolation::Vertex });
    made.set(floor, "st", spread);
    place(made, "floor", floor, {}, { glm::mat4(1.0F) });
    // four walls of a quad each, from the floor to kWallHeight around its edges
    const std::vector<glm::vec3> corners { { -1.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } };
    std::vector<glm::vec3> points;
    std::vector<std::int32_t> indices;
    for (std::size_t wall = 0; wall < corners.size(); ++wall) {
        const glm::vec3 & from = corners[wall];
        const glm::vec3 & to = corners[(wall + 1U) % corners.size()];
        const auto first = static_cast<std::int32_t>(points.size());
        points.insert(points.end(), { from, to, to + glm::vec3 { 0.0F, 0.0F, kWallHeight }, from + glm::vec3 { 0.0F, 0.0F, kWallHeight } });
        indices.insert(indices.end(), { first, first + 1, first + 2, first + 3 });
    }
    const scene::Handle walls = made.create("mesh", "walls");
    made.set(walls, "P", scene::Value::points(points));
    made.set(walls, "vertices", scene::Value::integers(std::vector<std::int32_t>(corners.size(), 4)));
    made.set(walls, "indices", scene::Value::integers(indices));
    place(made, "walls", walls, {}, { glm::mat4(1.0F) });

    made.create("domeLight", "sky");
    // a rect light and a disk light face down, along their -z, and a sphere light hangs low
    const scene::Handle rect = made.create("rectLight", "rect");
    made.set(rect, "transform", scene::Value::matrix(placed({ kLightsAt[0][0], kLightsAt[0][1], kWallHeight }, kLightSize)));
    const scene::Handle disk = made.create("diskLight", "disk");
    made.set(disk, "transform", scene::Value::matrix(placed({ kLightsAt[1][0], kLightsAt[1][1], kWallHeight }, kLightSize)));
    const scene::Handle sphere = made.create("sphereLight", "sphere");
    made.set(sphere, "radius", scene::Value::number(kSphereRadius));
    made.set(sphere, "transform", scene::Value::matrix(placed({ kLightsAt[2][0], kLightsAt[2][1], kSphereHeight }, 1.0F)));
    return out;
}

/** A shader node of language materialx, of a node definition and its constant inputs. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the node's name, then its definition
scene::Handle node(scene::Scene & scene, const std::string & name, const std::string & definition, const std::vector<std::pair<std::string, scene::Value>> & inputs) {
    const scene::Handle out = scene.create("shader", name);
    scene.set(out, "shader", scene::Value::string(definition));
    scene.set(out, "language", scene::Value::string("materialx"));
    for (const auto & [input, value] : inputs) {
        scene.declare(out, scene::Declaration { .name = input, .type = value.type(), .count = static_cast<std::uint32_t>(value.size()), .fallback = value });
        scene.set(out, input, value);
    }
    return out;
}

/** A connection, by a name of its own, from a node's only output to an input of another. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the connection's name, then the input
void connect(scene::Scene & scene, const std::string & name, const scene::Handle & from, const scene::Handle & to, const std::string & input) {
    const scene::Handle out = scene.create("connection", name);
    scene.set(out, "from", scene::Value::handle(from));
    scene.set(out, "to", scene::Value::handle(to));
    scene.set(out, "input", scene::Value::string(input));
}

/**
 * Writes the textured room's image: tiles of two colours, kImageTiles to a side, each shaded by a
 * ring that rises and falls across it. Its red runs from a fifth to four fifths, and is the
 * floor's roughness.
 **/
void writeImage(const std::string & path, std::uint32_t side) {
    film::Snapshot image;
    image.resolution = glm::uvec2 { side };
    image.stride = 4;
    image.values.reserve(static_cast<std::size_t>(side) * side * 4U);
    for (std::uint32_t y = 0; y < side; ++y) {
        for (std::uint32_t x = 0; x < side; ++x) {
            const float u = static_cast<float>(x) / static_cast<float>(side);
            const float v = static_cast<float>(y) / static_cast<float>(side);
            const auto tileU = static_cast<std::uint32_t>(u * kImageTiles);
            const auto tileV = static_cast<std::uint32_t>(v * kImageTiles);
            const glm::vec3 & colour = ((tileU + tileV) % 2U) == 0U ? kImageLight : kImageDark;
            const float ring = std::sin(kImageRings * glm::length(glm::fract(glm::vec2 { u, v } * kImageTiles) - 0.5F));
            const glm::vec3 shaded = colour * (kImageShade + ((1.0F - kImageShade) * ring * ring));
            image.values.insert(image.values.end(), { shaded.x, shaded.y, shaded.z, 1.0F });
        }
    }
    output::writeExr(path, image, { { .name = "", .outputs = { std::string(output::kBeauty) } } }, {});
}

/**
 * Writes the fog room's grid, cells voxels to the floor's side: a density that falls to 1/e over
 * kFogFalloff above the floor, and varies across it by kFogVariation in waves. Its index space
 * starts at the room's corner, -1 along x and y and 0 along z, and runs up to the walls' height.
 **/
void writeFog(const std::string & path, std::int32_t cells) {
    openvdb::initialize();
    const double voxel = 2.0 / static_cast<double>(cells);
    const openvdb::FloatGrid::Ptr fog = openvdb::FloatGrid::create(0.0F);
    fog->setName("density");
    const openvdb::math::Transform::Ptr transform = openvdb::math::Transform::createLinearTransform(voxel);
    transform->postTranslate(openvdb::Vec3d(-1.0, -1.0, 0.0));
    fog->setTransform(transform);
    const auto high = static_cast<std::int32_t>(std::ceil(static_cast<double>(kWallHeight) / voxel));  // checked: the benchmark's own, small and finite
    openvdb::FloatGrid::Accessor accessor = fog->getAccessor();
    for (std::int32_t z = 0; z < high; ++z) {
        for (std::int32_t y = 0; y < cells; ++y) {
            for (std::int32_t x = 0; x < cells; ++x) {
                const openvdb::Vec3d at = fog->indexToWorld(openvdb::Vec3d(x + 0.5, y + 0.5, z + 0.5));
                const auto across = static_cast<float>(at.x());
                const auto along = static_cast<float>(at.y());
                const float varied = 1.0F + (kFogVariation * std::sin(kFogWaves * across) * std::cos(kFogWaves * along));
                accessor.setValue(openvdb::Coord(x, y, z), kFogDensity * varied * std::exp(-static_cast<float>(at.z()) / kFogFalloff));
            }
        }
    }
    openvdb::io::File file(path);
    file.write({ fog });
    file.close();
}

/** A closed ball of a radius, placed at a centre, with a material. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the ball's name, then its material
void ball(scene::Scene & scene, const std::string & name, float radius, const glm::vec3 & centre, const scene::Handle & material) {
    const scene::Handle sphere = scene.create("sphere", name);
    scene.set(sphere, "radius", scene::Value::number(radius));
    scene.set(sphere, "zmin", scene::Value::number(-radius));
    scene.set(sphere, "zmax", scene::Value::number(radius));
    const scene::Handle instance = scene.create("instance", name);
    scene.set(instance, "prototype", scene::Value::handle(sphere));
    scene.set(instance, "transform", scene::Value::matrix(glm::translate(glm::mat4(1.0F), centre)));
    scene.set(instance, "material", scene::Value::handle(material));
}

};  // namespace

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the scale, then the side and samples
boost::shared_ptr<scene::Scene> litRoom(float scale, std::uint32_t side, std::uint32_t samples) {
    const boost::shared_ptr<scene::Scene> out = room(scale, side, samples);
    out->commit();
    out->apply();
    return out;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the scale, then the side and samples
boost::shared_ptr<scene::Scene> texturedRoom(float scale, std::uint32_t side, std::uint32_t samples, const std::string & image) {
    writeImage(image, scale < 1.0F ? kQuickImageSide : kImageSide);
    const boost::shared_ptr<scene::Scene> out = room(scale, side, samples);
    scene::Scene & made = *out;
    const scene::Handle colour = node(made, "colour", "ND_image_color3", { { "file", scene::Value::string(image) } });
    const scene::Handle rough = node(made, "rough", "ND_image_float", { { "file", scene::Value::string(image) } });
    const scene::Handle surface = node(made, "surface", "ND_open_pbr_surface_surfaceshader",
        { { "coat_weight", scene::Value::number(1.0F) }, { "coat_roughness", scene::Value::number(kCoatRoughness) } });
    connect(made, "colour to base_color", colour, surface, "base_color");
    connect(made, "rough to specular_roughness", rough, surface, "specular_roughness");
    const scene::Handle material = made.create("material", "textured");
    made.set(material, "surface", scene::Value::handle(surface));
    // the room's instances are listed once it is applied
    made.commit();
    made.apply();
    for (const scene::Handle & instance : made.objects("instance")) {
        made.set(instance, "material", scene::Value::handle(material));
    }
    made.commit();
    made.apply();
    return out;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the scale, then the side and samples
boost::shared_ptr<scene::Scene> outputRoom(float scale, std::uint32_t side, std::uint32_t samples) {
    const boost::shared_ptr<scene::Scene> out = room(scale, side, samples);
    scene::Scene & made = *out;
    // the room's lights are found once it is committed
    made.commit();
    made.apply();
    made.set(*made.find("rectLight", "rect"), "lightGroup", scene::Value::string("key"));
    made.set(*made.find("diskLight", "disk"), "lightGroup", scene::Value::string("fill"));
    std::vector<scene::Handle> channels;
    const std::pair<const char *, const char *> expressions[] = { { "diffuse", "C<.D>.*" }, { "glossy", "C<.G>.*" }, { "specular", "C<.S>.*" },
        { "emission", "C[LO]" }, { "caustic", "C.*<TG>.*L" }, { "key", "C.*<L.'key'>" }, { "fill", "C.*<L.'fill'>" } };
    for (const auto & [name, expression] : expressions) {
        const scene::Handle channel = made.create("channel", name);
        made.set(channel, "sourceType", scene::Value::string("lpe"));
        made.set(channel, "sourceName", scene::Value::string(expression));
        channels.push_back(channel);
    }
    for (const auto & [name, source] : { std::pair { "CryptoObject", "crypto_object" }, std::pair { "CryptoMaterial", "crypto_material" } }) {
        const scene::Handle channel = made.create("channel", name);
        made.set(channel, "sourceName", scene::Value::string(source));
        channels.push_back(channel);
    }
    const scene::Handle picture = made.create("output", "picture");
    made.set(picture, "file", scene::Value::string("outputs.exr"));
    made.set(picture, "channels", scene::Value::handles(channels));
    const scene::Handle deep = made.create("output", "deep");
    made.set(deep, "file", scene::Value::string("outputs-deep.exr"));
    made.set(deep, "format", scene::Value::string("deepexr"));
    made.commit();
    made.apply();
    return out;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the scale, then the side and samples
boost::shared_ptr<scene::Scene> fogRoom(float scale, std::uint32_t side, std::uint32_t samples, const std::string & grid) {
    writeFog(grid, scale < 1.0F ? kQuickFogCells : kFogCells);
    const boost::shared_ptr<scene::Scene> out = room(scale, side, samples);
    scene::Scene & made = *out;
    const scene::Handle density = node(made, "density", "ND_geompropvalue_float", { { "geomprop", scene::Value::string("density") } });
    const scene::Handle scatters = node(made, "scatters", "ND_multiply_vector3FA", { { "in1", scene::Value::vector(glm::vec3 { kFogScattering }) } });
    const scene::Handle absorbs = node(made, "absorbs", "ND_multiply_vector3FA", { { "in1", scene::Value::vector(glm::vec3 { kFogAbsorption }) } });
    const scene::Handle vdf = node(made, "fog vdf", "ND_anisotropic_vdf", {});
    const scene::Handle shader = node(made, "fog", "ND_volume", {});
    connect(made, "density into scatters", density, scatters, "in2");
    connect(made, "density into absorbs", density, absorbs, "in2");
    connect(made, "scatters into vdf", scatters, vdf, "scattering");
    connect(made, "absorbs into vdf", absorbs, vdf, "absorption");
    connect(made, "vdf into fog", vdf, shader, "vdf");
    const scene::Handle material = made.create("material", "fog");
    made.set(material, "volume", scene::Value::handle(shader));
    const scene::Handle field = made.create("openVDBAsset", "density");
    made.set(field, "filePath", scene::Value::string(grid));
    made.set(field, "fieldName", scene::Value::string("density"));
    const scene::Handle fog = made.create("volume", "fog");
    made.set(fog, "fieldNames", scene::Value::strings({ "density" }));
    made.set(fog, "fields", scene::Value::handles({ field }));
    const scene::Handle instance = made.create("instance", "fog");
    made.set(instance, "prototype", scene::Value::handle(fog));
    made.set(instance, "transform", scene::Value::matrix(glm::mat4(1.0F)));
    made.set(instance, "material", scene::Value::handle(material));
    made.commit();
    made.apply();
    return out;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the scale, then the side and samples
boost::shared_ptr<scene::Scene> subsurfaceRoom(float scale, std::uint32_t side, std::uint32_t samples) {
    const boost::shared_ptr<scene::Scene> out = room(scale, side, samples);
    scene::Scene & made = *out;
    const scene::Handle surface = node(made, "marble", "ND_open_pbr_surface_surfaceshader",
        { { "subsurface_weight", scene::Value::number(1.0F) }, { "subsurface_color", scene::Value::colour(kStatueColour) },
            { "subsurface_radius", scene::Value::number(kStatueRadius) } });
    const scene::Handle material = made.create("material", "marble");
    made.set(material, "surface", scene::Value::handle(surface));
    // the balls stand on one another, from the floor up
    float height = kGroundRipple;
    for (std::size_t i = 0; i < std::size(kStatueRadii); ++i) {
        height += kStatueRadii[i];
        ball(made, "statue " + std::to_string(i), kStatueRadii[i], glm::vec3 { 0.0F, 0.0F, height }, material);
        height += kStatueRadii[i];
    }
    made.commit();
    made.apply();
    return out;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the scale, then the side and samples
boost::shared_ptr<scene::Scene> hairLock(float scale, std::uint32_t side, std::uint32_t samples) {
    const boost::shared_ptr<scene::Scene> out = boost::make_shared<scene::Scene>(scene::kinds());
    scene::Scene & made = *out;
    const scene::Handle options = made.create("options", "options");
    const scene::Handle camera = made.create("camera", "camera");
    made.set(options, "camera", scene::Value::handle(camera));
    made.set(options, "resolution", scene::Value::integers({ static_cast<std::int32_t>(side), static_cast<std::int32_t>(side) }));
    made.set(options, "samples", scene::Value::integer(static_cast<std::int32_t>(samples)));
    made.set(camera, "fov", scene::Value::number(kLockFov));
    // the camera looks along its +z, so a quarter turn about x has it look along -y, with +z up
    made.set(camera, "transform",
        scene::Value::matrix(glm::rotate(glm::translate(glm::mat4(1.0F), glm::vec3 { 0.0F, kLockDistance, kLockEye }), glm::radians(kQuarterTurn),
            glm::vec3 { 1.0F, 0.0F, 0.0F })));
    const scene::Handle sky = made.create("domeLight", "sky");
    made.set(sky, "intensity", scene::Value::number(kLockDome));
    // a distant light of no angle shines down its -z, from above, at an irradiance of its intensity
    const scene::Handle key = made.create("distantLight", "key");
    made.set(key, "angle", scene::Value::number(0.0F));
    made.set(key, "intensity", scene::Value::number(kKeyIntensity));

    // NOLINTNEXTLINE(bugprone-random-generator-seed) - every run builds the same scene
    std::mt19937 random(kSeed);
    std::uniform_real_distribution<float> unit(-1.0F, 1.0F);
    const auto count = std::max(kFewestStrands, static_cast<std::int32_t>(kStrands * scale));
    std::vector<glm::vec3> points;
    for (std::int32_t strand = 0; strand < count; ++strand) {
        const glm::vec3 root { kBandWide * unit(random), kBandDeep * unit(random), kBandHeight };
        const glm::vec3 sway { kSway * unit(random), kSway * unit(random), 0.0F };
        for (std::int32_t vertex = 0; vertex < kStrandVertices; ++vertex) {
            const float along = static_cast<float>(vertex) / static_cast<float>(kStrandVertices - 1);
            points.push_back(root + (sway * along * along) - glm::vec3 { 0.0F, 0.0F, kFall * along });
        }
    }
    const scene::Handle lock = made.create("curves", "lock");
    made.set(lock, "P", scene::Value::points(points));
    made.set(lock, "vertices", scene::Value::integers(std::vector<std::int32_t>(static_cast<std::size_t>(count), kStrandVertices)));
    made.set(lock, "type", scene::Value::string("cubic"));
    made.set(lock, "basis", scene::Value::string("b-spline"));
    made.set(lock, "constantWidth", scene::Value::number(kStrandWidth));
    made.set(lock, "profile", scene::Value::string("round"));
    const scene::Handle fibre = node(made, "hair bsdf", "ND_chiang_hair_bsdf", { { "absorption_coefficient", scene::Value::vector(kHairAbsorption) } });
    const scene::Handle surface = node(made, "hair", "ND_surface", {});
    connect(made, "fibre into hair", fibre, surface, "bsdf");
    const scene::Handle material = made.create("material", "hair");
    made.set(material, "surface", scene::Value::handle(surface));
    const scene::Handle instance = made.create("instance", "lock");
    made.set(instance, "prototype", scene::Value::handle(lock));
    made.set(instance, "transform", scene::Value::matrix(glm::mat4(1.0F)));
    made.set(instance, "material", scene::Value::handle(material));
    made.commit();
    made.apply();
    return out;
}

std::vector<Fixed> scenes(float scale) {
    std::vector<Fixed> out { smallTriangles(scale), largeTriangles(), curves(scale), instances(scale), motion(scale) };
    for (const Fixed & fixed : out) {
        fixed.scene->commit();
        fixed.scene->apply();
    }
    return out;
}

};  // namespace v3d::moya::bench
