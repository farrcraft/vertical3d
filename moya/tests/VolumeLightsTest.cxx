/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Compare.h>
#include <moya/libmoya/film/Estimate.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/Blackbody.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/volume/Coefficients.h>
#include <moya/libmoya/kernel/volume/Homogeneous.h>
#include <moya/libmoya/kernel/volume/Medium.h>
#include <moya/libmoya/light/Planckian.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <openvdb/openvdb.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace film = v3d::moya::film;
namespace integrate = v3d::moya::integrate;
namespace scene = v3d::moya::scene;
namespace spectrum = v3d::moya::kernel::spectrum;
namespace volume = v3d::moya::kernel::volume;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** The side of each picture, in pixels. **/
constexpr std::uint32_t kSide = 8;

/** The wavelengths drawn to give the colour a light at each wavelength makes on average. **/
constexpr int kDraws = 4096;

/** The side of a voxel of the grids the cases write, and how far a box reaches, in voxels. **/
constexpr double kVoxel = 0.05;
constexpr int kReach = 10;

/** A shader node of a definition with inputs. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the node's name, then its definition
scene::Handle node(scene::Scene & scene, const std::string & name, const std::string & definition, const std::vector<std::pair<std::string, scene::Value>> & inputs) {
    const scene::Handle out = scene.create("shader", name);
    scene.set(out, "shader", scene::Value::string(definition));
    for (const auto & [input, value] : inputs) {
        scene.declare(out, scene::Declaration { .name = input, .type = value.type(), .count = static_cast<std::uint32_t>(value.size()), .fallback = value });
        scene.set(out, input, value);
    }
    return out;
}

/** Connects an output to an input, by a connection of a name. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the name, then the input
void connect(scene::Scene & scene, const std::string & name, const scene::Handle & from, const scene::Handle & to, const std::string & input) {
    const scene::Handle out = scene.create("connection", name);
    scene.set(out, "from", scene::Value::handle(from));
    scene.set(out, "to", scene::Value::handle(to));
    scene.set(out, "input", scene::Value::string(input));
}

/** A material of a volume shader whose vdf and edf the nodes given feed. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the vdf, then the edf
scene::Handle volumeMaterial(scene::Scene & scene, const std::string & name, const scene::Handle & vdf, const scene::Handle & edf) {
    const scene::Handle shader = node(scene, name, "ND_volume", {});
    connect(scene, name + " vdf into volume", vdf, shader, "vdf");
    connect(scene, name + " edf into volume", edf, shader, "edf");
    const scene::Handle out = scene.create("material", name);
    scene.set(out, "volume", scene::Value::handle(shader));
    return out;
}

/**
 * A closed sphere of a radius about a centre, filled with a medium that absorbs and emits, the
 * same everywhere inside it.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the absorption, then the emission
void glowingSphere(scene::Scene & scene, float radius, const glm::vec3 & centre, const glm::vec3 & absorption, const glm::vec3 & emission) {
    const scene::Handle vdf = node(scene, "glow vdf", "ND_absorption_vdf", { { "absorption", scene::Value::vector(absorption) } });
    const scene::Handle edf = node(scene, "glow edf", "ND_uniform_edf", { { "color", scene::Value::colour(emission, "ACEScg") } });
    const scene::Handle material = volumeMaterial(scene, "glow", vdf, edf);
    const scene::Handle sphere = scene.create("sphere", "glow");
    scene.set(sphere, "radius", scene::Value::number(radius));
    scene.set(sphere, "zmin", scene::Value::number(-radius));
    scene.set(sphere, "zmax", scene::Value::number(radius));
    const glm::mat4 placed = glm::rotate(moved(centre.x, centre.y, centre.z), glm::radians(90.0F), glm::vec3 { 1.0F, 0.0F, 0.0F });
    scene.set(instanced(scene, "glow", sphere, {}, placed), "material", scene::Value::handle(material));
}

/**
 * A grid of a name and one value, written to a file in the temporary directory, and the file's
 * path. It fills a box of voxels kVoxel a side from -reach to reach on each axis, kReach unless
 * given. A box this size holds active tiles beside its leaves.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the grid's name, then the file's
std::string boxGrid(const std::string & name, float value, const std::string & file, int reach = kReach) {
    openvdb::initialize();
    const openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create(0.0F);
    grid->setName(name);
    grid->setTransform(openvdb::math::Transform::createLinearTransform(kVoxel));
    grid->fill(openvdb::CoordBBox(openvdb::Coord(-reach), openvdb::Coord(reach)), value, true);
    const std::string out = (boost::filesystem::temp_directory_path() / file).string();
    openvdb::io::File written(out);
    written.write({ grid });
    written.close();
    return out;
}

/** A field of a grid of a name in a file. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the object's name, the file, then the grid
scene::Handle field(scene::Scene & scene, const std::string & name, const std::string & path, const std::string & grid) {
    const scene::Handle out = scene.create("openVDBAsset", name);
    scene.set(out, "filePath", scene::Value::string(path));
    scene.set(out, "fieldName", scene::Value::string(grid));
    return out;
}

/**
 * A volume of a density and a temperature, each read from a file, placed at a centre. Its program
 * absorbs the density times a coefficient. It emits blackbody's colour at the temperature, times
 * the density and a brightness.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the files, then the medium
void fire(scene::Scene & scene, const std::string & densityFile, const std::string & temperatureFile, const glm::vec3 & centre, const glm::vec3 & absorption,
    float brightness) {
    const scene::Handle density = node(scene, "density", "ND_geompropvalue_float", { { "geomprop", scene::Value::string("density") } });
    const scene::Handle temperature = node(scene, "temperature", "ND_geompropvalue_float", { { "geomprop", scene::Value::string("temperature") } });
    const scene::Handle absorbed = node(scene, "absorbed", "ND_multiply_vector3FA", { { "in1", scene::Value::vector(absorption) } });
    const scene::Handle hot = node(scene, "hot", "ND_blackbody", {});
    const scene::Handle bright = node(scene, "bright", "ND_multiply_float", { { "in2", scene::Value::number(brightness) } });
    const scene::Handle glowing = node(scene, "glowing", "ND_multiply_color3FA", {});
    const scene::Handle vdf = node(scene, "fire vdf", "ND_absorption_vdf", {});
    const scene::Handle edf = node(scene, "fire edf", "ND_uniform_edf", {});
    connect(scene, "density into absorbed", density, absorbed, "in2");
    connect(scene, "absorbed into vdf", absorbed, vdf, "absorption");
    connect(scene, "temperature into hot", temperature, hot, "temperature");
    connect(scene, "density into bright", density, bright, "in1");
    connect(scene, "hot into glowing", hot, glowing, "in1");
    connect(scene, "bright into glowing", bright, glowing, "in2");
    connect(scene, "glowing into edf", glowing, edf, "color");
    const scene::Handle material = volumeMaterial(scene, "fire", vdf, edf);
    const scene::Handle placed = scene.create("volume", "fire");
    scene.set(placed, "fieldNames", scene::Value::strings({ "density", "temperature" }));
    scene.set(placed, "fields",
        scene::Value::handles({ field(scene, "density field", densityFile, "density"), field(scene, "temperature field", temperatureFile, "temperature") }));
    scene.set(instanced(scene, "fire", placed, {}, moved(centre.x, centre.y, centre.z)), "material", scene::Value::handle(material));
}

/**
 * An orthographic camera five units back along -z looking at the origin, of a count of samples,
 * with no light. Its screen reaches a distance from the axis.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the samples, then the screen's reach
scene::Handle aimed(scene::Scene & scene, std::int32_t samples, float reach) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
    scene.set(options, "samples", scene::Value::integer(samples));
    scene.set(camera, "screen", scene::Value::floats({ -reach, reach, -reach, reach }));
    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
    return options;
}

/** The picture of a scene that build() fills, given the options aimed() makes. **/
template <typename F>
film::Snapshot pictured(std::int32_t samples, float reach, F && build) {
    scene::Scene scene(scene::kinds());
    const scene::Handle options = aimed(scene, samples, reach);
    build(scene, options);
    scene.commit();
    scene.apply();
    CpuDevice device;
    return integrate::render(device, scene, {}, v3d::moya::colour::Config());
}

/** The mean of each component of a picture's beauty, over every pixel. **/
glm::vec3 meanColour(const film::Snapshot & snapshot) {
    glm::dvec3 sum { 0.0 };
    for (std::uint32_t y = 0; y < snapshot.resolution.y; ++y) {
        for (std::uint32_t x = 0; x < snapshot.resolution.x; ++x) {
            sum += glm::dvec3 { snapshot.at(x, y, 0), snapshot.at(x, y, 1), snapshot.at(x, y, 2) };
        }
    }
    return glm::vec3(sum / static_cast<double>(snapshot.resolution.x * snapshot.resolution.y));
}

/**
 * The ACEScg colour a ray sees through a depth of a medium that absorbs and emits the same
 * everywhere along it. At each wavelength it is the emission over the extinction, times one less
 * the transmittance. The film adds it over evenly spread draws of the wavelengths.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the absorption, then the emission
glm::vec3 seenThrough(const glm::vec3 & absorption, const glm::vec3 & emission, float depth) {
    glm::dvec3 sum { 0.0 };
    for (int i = 0; i < kDraws; ++i) {
        const auto wavelengths = spectrum::SampledWavelengths::visible((static_cast<float>(i) + 0.5F) / static_cast<float>(kDraws));
        volume::Medium medium;
        medium.absorption = absorption;
        const spectrum::SampledSpectrum extinction = volume::coefficientsOf(colour::acescgTable(), medium, wavelengths).extinction();
        const spectrum::SampledSpectrum held = spectrum::SampledSpectrum(1.0F) - volume::transmittance(extinction, depth);
        const spectrum::SampledSpectrum light =
            spectrum::illuminant(colour::acescgTable(), emission, wavelengths) * spectrum::safeDivide(held, extinction);
        sum += glm::dvec3(spectrum::acescgFromXyz(spectrum::toXyz(light, wavelengths)));
    }
    return glm::vec3(sum / static_cast<double>(kDraws));
}

/** Whether each component of one colour lies within a share of the other's. **/
bool near(const glm::vec3 & given, const glm::vec3 & expected, float share) {
    for (glm::length_t i = 0; i < 3; ++i) {
        if (!(std::fabs(given[i] - expected[i]) <= share * std::fabs(expected[i]))) {
            return false;
        }
    }
    return true;
}

/**
 * A white disk of a large radius at the origin facing the camera, seen over a square of a half
 * width about the axis, lit by what build() adds and nothing else. A path takes up to two bounces.
 * Gives the mean luminance of the picture under a strategy, and a way of drawing lights.
 **/
template <typename F>
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the strategy, then the picking
film::Estimate litFloor(const char * strategy, const char * picking, std::int32_t samples, F && build) {
    const film::Snapshot snapshot = pictured(samples, 0.25F, [&](scene::Scene & scene, const scene::Handle & options) {
        scene.set(options, "lightSampling", scene::Value::string(strategy));
        scene.set(options, "lightPicking", scene::Value::string(picking));
        scene.set(options, "maxDepth", scene::Value::integer(2));
        const scene::Handle floor = scene.create("disk", "floor");
        scene.set(floor, "radius", scene::Value::number(100.0F));
        scene.declare(floor, scene::Declaration { .name = "Cs",
            .type = scene::Type::Colour,
            .count = scene::Declaration::kAny,
            .fallback = scene::Value::colours({}),
            .interpolation = scene::Interpolation::Constant });
        scene.set(floor, "Cs", scene::Value::colours({ glm::vec3 { 1.0F } }));
        // the disk faces +z in its own space, so it is turned to face the camera
        instanced(scene, "floor", floor, {}, glm::rotate(glm::mat4(1.0F), glm::radians(180.0F), glm::vec3 { 1.0F, 0.0F, 0.0F }));
        build(scene);
    });
    return film::mean(snapshot, { 0, 0 }, { kSide, kSide });
}

// a sphere of a medium that absorbs and emits the same everywhere is seen along its diameter by an
// orthographic camera. It sends back its emission over its extinction, times one less what passes
// along the diameter, at each wavelength. Camera rays find it only by tracking through it
BOOST_AUTO_TEST_CASE(moya_volume_lights_a_glowing_sphere_sends_back_its_closed_form_test) {
    const glm::vec3 absorption { 0.4F, 0.9F, 1.6F };
    const glm::vec3 emission { 0.8F, 0.5F, 0.3F };
    const film::Snapshot snapshot =
        pictured(4096, 0.05F, [&](scene::Scene & scene, const scene::Handle &) { glowingSphere(scene, 1.0F, glm::vec3 { 0.0F }, absorption, emission); });
    const glm::vec3 expected = seenThrough(absorption, emission, 2.0F);
    const glm::vec3 mean = meanColour(snapshot);
    BOOST_TEST(near(mean, expected, 0.02F), mean.x << " " << mean.y << " " << mean.z << " against " << expected.x << " " << expected.y << " " << expected.z);
}

// a box's program absorbs its density, and emits blackbody's colour at its temperature times its
// density. It sends back the closed form of a homogeneous medium of the same along its depth. The
// depth is twice its reach, and a voxel more as a lookup falls away past its last voxel. The
// temperature reaches further than the density, so the colour is the same wherever the density
// is not zero
BOOST_AUTO_TEST_CASE(moya_volume_lights_a_grid_glows_by_its_temperature_test) {
    const glm::vec3 absorption { 0.6F, 0.8F, 1.0F };
    constexpr float kDensity = 1.5F;
    constexpr float kTemperature = 3000.0F;
    constexpr float kBrightness = 2.0F;
    const std::string density = boxGrid("density", kDensity, "v3d_moya_glow_density.vdb");
    const std::string temperature = boxGrid("temperature", kTemperature, "v3d_moya_glow_temperature.vdb", 2 * kReach);
    const film::Snapshot snapshot = pictured(4096, 0.05F, [&](scene::Scene & scene, const scene::Handle &) {
        fire(scene, density, temperature, glm::vec3 { 0.0F }, absorption, kBrightness);
    });
    boost::filesystem::remove(density);
    boost::filesystem::remove(temperature);
    const float depth = static_cast<float>((2.0 * kReach * kVoxel) + kVoxel);
    const glm::vec3 expected = seenThrough(absorption * kDensity, spectrum::blackbody(kTemperature) * kDensity * kBrightness, depth);
    const glm::vec3 mean = meanColour(snapshot);
    BOOST_TEST(near(mean, expected, 0.02F), mean.x << " " << mean.y << " " << mean.z << " against " << expected.x << " " << expected.y << " " << expected.z);
}

// a floor lit by a glowing sphere beside the camera's view alone sends back the same light by
// tracking alone, by light samples alone and by both, drawn by the light tree and by power. A
// light sample draws its point uniformly in the sphere's box, and one outside the sphere emits
// nothing
BOOST_AUTO_TEST_CASE(moya_volume_lights_a_glowing_sphere_lights_alike_by_each_strategy_test) {
    const auto lit = [](const char * strategy, const char * picking, std::int32_t samples) {
        return litFloor(strategy, picking, samples,
            [](scene::Scene & scene) { glowingSphere(scene, 0.5F, glm::vec3 { 1.0F, 0.0F, -1.0F }, glm::vec3 { 1.0F }, glm::vec3 { 4.0F }); });
    };
    const film::Estimate tracked = lit("bsdf", "tree", 2048);
    BOOST_TEST(tracked.value > 0.0F);
    for (const char * strategy : { "light", "mis" }) {
        for (const char * picking : { "tree", "power" }) {
            BOOST_TEST_CONTEXT(strategy << " " << picking) {
                const film::Estimate sampled = lit(strategy, picking, 512);
                BOOST_TEST(film::sigmas(sampled, tracked) < 4.0F, sampled.value << " against " << tracked.value);
            }
        }
    }
}

// a floor lit by a glowing grid beside the camera's view alone sends back the same light by
// tracking alone, by light samples alone and by both, drawn by the light tree and by power. A
// light sample draws a brick, then a cell by what it emits, then a point in it. The box's middle
// is held as active tiles, which are bricks as its leaves are
BOOST_AUTO_TEST_CASE(moya_volume_lights_a_glowing_grid_lights_alike_by_each_strategy_test) {
    const std::string density = boxGrid("density", 1.0F, "v3d_moya_glow_lit_density.vdb");
    const std::string temperature = boxGrid("temperature", 1500.0F, "v3d_moya_glow_lit_temperature.vdb", 2 * kReach);
    const auto lit = [&](const char * strategy, const char * picking, std::int32_t samples) {
        return litFloor(strategy, picking, samples,
            [&](scene::Scene & scene) { fire(scene, density, temperature, glm::vec3 { 1.0F, 0.0F, -1.0F }, glm::vec3 { 1.0F }, 8.0F); });
    };
    const film::Estimate tracked = lit("bsdf", "tree", 2048);
    BOOST_TEST(tracked.value > 0.0F);
    for (const char * strategy : { "light", "mis" }) {
        for (const char * picking : { "tree", "power" }) {
            BOOST_TEST_CONTEXT(strategy << " " << picking) {
                const film::Estimate sampled = lit(strategy, picking, 512);
                BOOST_TEST(film::sigmas(sampled, tracked) < 4.0F, sampled.value << " against " << tracked.value);
            }
        }
    }
    boost::filesystem::remove(density);
    boost::filesystem::remove(temperature);
}

// blackbody's colour at a temperature has a luminance of one, and the colour of a Planckian
// radiator there, which a light's colour temperature sums through the observer at every
// nanometre
BOOST_AUTO_TEST_CASE(moya_volume_lights_blackbody_gives_a_planckian_radiators_colour_test) {
    for (const float kelvin : { 1000.0F, 1800.0F, 3000.0F, 5000.0F, 6500.0F, 12000.0F }) {
        BOOST_TEST_CONTEXT(kelvin) {
            const glm::vec3 colour = spectrum::blackbody(kelvin);
            const glm::vec3 radiator = v3d::moya::light::planckian(kelvin).colour;
            BOOST_TEST(std::fabs(spectrum::acescgToXyz(colour).y - 1.0F) < 1.0e-4F);
            BOOST_TEST(near(colour, radiator, 0.01F), colour.x << " " << colour.y << " " << colour.z << " against " << radiator.x << " " << radiator.y << " " << radiator.z);
        }
    }
}

};  // namespace
};  // namespace v3d::moya::tracing
