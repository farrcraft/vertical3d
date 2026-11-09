/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/bench/Scenes.h>

#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
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

};  // namespace

std::vector<Fixed> scenes(float scale) {
    std::vector<Fixed> out { smallTriangles(scale), largeTriangles(), curves(scale), instances(scale), motion(scale) };
    for (const Fixed & fixed : out) {
        fixed.scene->commit();
        fixed.scene->apply();
    }
    return out;
}

};  // namespace v3d::moya::bench
