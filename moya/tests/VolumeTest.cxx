/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/rib/Reader.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Cache.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/volume/Cell.h>
#include <moya/libmoya/kernel/volume/Coefficients.h>
#include <moya/libmoya/kernel/volume/Field.h>
#include <moya/libmoya/kernel/volume/Grid.h>
#include <moya/libmoya/kernel/volume/Homogeneous.h>
#include <moya/libmoya/kernel/volume/Medium.h>
#include <moya/libmoya/kernel/volume/Volume.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

// MSVC reports code NanoVDB's tools cannot reach once it inlines them, which it does not count
// as a warning in an external header, so the warning is switched off around them.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
#include <nanovdb/NanoVDB.h>
#include <nanovdb/tools/CreateNanoGrid.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <openvdb/openvdb.h>
#include <openvdb/tools/LevelSetSphere.h>
#include <openvdb/tools/Interpolation.h>
#include <openvdb/tools/LevelSetUtil.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/common.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace film = v3d::moya::film;
namespace geometry = v3d::moya::geometry;
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
constexpr double kVoxel = 0.02;
constexpr int kReach = 50;

/**
 * A grid named density over a box of voxels from -kReach to kReach on each axis, of one value,
 * with voxels kVoxel a side. A lookup reads it whole from -1 to 1 on each axis, and falls away to
 * nothing over the voxel beyond.
 **/
openvdb::FloatGrid::Ptr box(float value) {
    openvdb::initialize();
    openvdb::FloatGrid::Ptr out = openvdb::FloatGrid::create(0.0F);
    out->setName("density");
    out->setTransform(openvdb::math::Transform::createLinearTransform(kVoxel));
    out->fill(openvdb::CoordBBox(openvdb::Coord(-kReach), openvdb::Coord(kReach)), value, true);
    return out;
}

/** A grid written to a file of a name in the temporary directory, and the file's path. **/
std::string written(const openvdb::GridBase::Ptr & grid, const std::string & name) {
    const std::string out = (boost::filesystem::temp_directory_path() / name).string();
    openvdb::io::File file(out);
    file.write({ grid });
    file.close();
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

/** A volume that reads one field as its density. **/
scene::Handle smoke(scene::Scene & scene, const std::string & name, const scene::Handle & density) {
    const scene::Handle out = scene.create("volume", name);
    scene.set(out, "fieldNames", scene::Value::strings({ "density" }));
    scene.set(out, "fields", scene::Value::handles({ density }));
    return out;
}

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

/**
 * A material of a volume that absorbs its density times a coefficient in each channel, through a
 * program that reads the density as a primvar.
 **/
scene::Handle absorbing(scene::Scene & scene, const std::string & name, const glm::vec3 & absorption) {
    const scene::Handle density = node(scene, name + " density", "ND_geompropvalue_float", { { "geomprop", scene::Value::string("density") } });
    const scene::Handle scaled = node(scene, name + " scaled", "ND_multiply_vector3FA", { { "in1", scene::Value::vector(absorption) } });
    const scene::Handle vdf = node(scene, name + " vdf", "ND_anisotropic_vdf", {});
    const scene::Handle shader = node(scene, name, "ND_volume", {});
    connect(scene, name + " density into scaled", density, scaled, "in2");
    connect(scene, name + " scaled into vdf", scaled, vdf, "absorption");
    connect(scene, name + " vdf into volume", vdf, shader, "vdf");
    const scene::Handle out = scene.create("material", name);
    scene.set(out, "volume", scene::Value::handle(shader));
    return out;
}

/**
 * An orthographic camera five units back along -z looking at the origin, of a count of samples,
 * under a white dome light. Its screen reaches a distance from the axis, a twentieth of a unit
 * unless given, so every ray runs along z within that distance of the axis.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the samples, then the screen's reach
scene::Handle aimed(scene::Scene & scene, std::int32_t samples, float reach = 0.05F) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(options, "resolution", scene::Value::integers({ kSide, kSide }));
    scene.set(options, "samples", scene::Value::integer(samples));
    scene.set(camera, "screen", scene::Value::floats({ -reach, reach, -reach, reach }));
    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
    scene.create("domeLight", "sky");
    return options;
}

/** The picture of a scene that build() fills, given the options aimed() makes. **/
template <typename F>
film::Snapshot pictured(std::int32_t samples, F && build, float reach = 0.05F) {
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
 * The ACEScg colour a white light makes on average after it passes a share at each wavelength,
 * as the film adds it, over evenly spread draws of the wavelengths.
 **/
template <typename F>
glm::vec3 passedOf(F && share) {
    glm::dvec3 sum { 0.0 };
    for (int i = 0; i < kDraws; ++i) {
        const auto wavelengths = spectrum::SampledWavelengths::visible((static_cast<float>(i) + 0.5F) / static_cast<float>(kDraws));
        const spectrum::SampledSpectrum light = share(wavelengths) * spectrum::illuminant(colour::acescgTable(), glm::vec3 { 1.0F }, wavelengths);
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

/** Whether two snapshots hold the same floats, bit for bit. **/
bool sameBytes(const film::Snapshot & one, const film::Snapshot & other) {
    return one.values.size() == other.values.size() && std::memcmp(one.values.data(), other.values.data(), one.values.size() * sizeof(float)) == 0;
}

// the cell a voxel lies in bounds every lookup whose floor lies in it, though a lookup near its far
// side reads the cells beyond. A grid of scattered leaves and tiles of random values, between them
// and beyond them, is read at points in and around every node
BOOST_AUTO_TEST_CASE(moya_volume_a_cell_bounds_every_lookup_inside_it_test) {
    openvdb::initialize();
    const openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create(0.0F);
    std::uint32_t seed = 7U;
    const auto place = [&seed] { return (static_cast<int>(next(seed) * 601.0F) - 300) / 4; };
    openvdb::FloatGrid::Accessor access = grid->getAccessor();
    for (int i = 0; i < 4000; ++i) {
        const openvdb::Coord voxel(place(), place(), place());
        access.setValue(voxel, next(seed) * 4.0F);
    }
    grid->fill(openvdb::CoordBBox(openvdb::Coord(128), openvdb::Coord(255)), 2.5F, true);
    grid->fill(openvdb::CoordBBox(openvdb::Coord(-64, 0, 0), openvdb::Coord(-57, 7, 7)), 9.0F, true);
    // a voxel alone in its upper node, beside one that holds nothing
    access.setValue(openvdb::Coord(0, 2 * volume::kUpperSide, 0), 7.0F);
    const nanovdb::GridHandle<nanovdb::HostBuffer> handle = nanovdb::tools::createNanoGrid(*grid);
    const nanovdb::FloatGrid & nano = *handle.grid<float>();
    const auto point = [&seed] { return -90.0F + (next(seed) * 390.0F); };
    const auto edge = [&seed] { return static_cast<float>(2 * volume::kUpperSide) - (next(seed) * 1.5F); };
    int sides[3] {};
    for (int i = 0; i < 200000; ++i) {
        glm::vec3 at { point(), point(), point() };
        if (i % 10 == 0) {
            at = glm::vec3 { at.x / 100.0F, edge(), at.z / 100.0F };
        }
        const volume::Cell cell = volume::cellAt(nano, glm::ivec3(glm::floor(at)));
        const float read = volume::valueAt(nano, at);
        BOOST_TEST_REQUIRE(read <= cell.most, "a lookup of " << read << " at " << at.x << " " << at.y << " " << at.z << " in a cell of most " << cell.most);
        if (cell.side == volume::kLeafSide) {
            ++sides[0];
        } else {
            ++sides[cell.side == volume::kLowerSide ? 1 : 2];
        }
    }
    BOOST_TEST(sides[0] > 0);
    BOOST_TEST(sides[1] > 0);
    BOOST_TEST(sides[2] > 0);
}

// a box of a constant density whose program absorbs the density times a colour lets through Beer
// and Lambert's share along its depth at each wavelength, as a homogeneous medium of that
// absorption does. The depth is two units, and half a voxel more on each side as a lookup falls
// away beyond the last voxel
BOOST_AUTO_TEST_CASE(moya_volume_a_constant_grid_absorbs_as_a_homogeneous_medium_test) {
    const glm::vec3 absorption { 0.2F, 0.6F, 1.2F };
    const std::string path = written(box(1.5F), "v3d_moya_volume_constant.vdb");
    const film::Snapshot snapshot = pictured(4096, [&](scene::Scene & scene, const scene::Handle &) {
        const scene::Handle placed = instanced(scene, "smoke", smoke(scene, "smoke", field(scene, "density", path, "density")), {}, glm::mat4(1.0F));
        scene.set(placed, "material", scene::Value::handle(absorbing(scene, "smoke", absorption)));
    });
    boost::filesystem::remove(path);
    const float depth = 2.0F + static_cast<float>(kVoxel);
    const glm::vec3 expected = passedOf([&](const spectrum::SampledWavelengths & wavelengths) {
        volume::Medium medium;
        medium.absorption = absorption * 1.5F;
        return volume::transmittance(volume::coefficientsOf(colour::acescgTable(), medium, wavelengths).extinction(), depth);
    });
    const glm::vec3 mean = meanColour(snapshot);
    BOOST_TEST(near(mean, expected, 0.02F), mean.x << " " << mean.y << " " << mean.z << " against " << expected.x << " " << expected.y << " " << expected.z);
}

// the same box and program, read from RIB as RenderMan's OpenVDB volume and a network of a VDF's
// Bxdf under a dome light, lets through the same share
BOOST_AUTO_TEST_CASE(moya_volume_a_fog_read_from_rib_absorbs_as_its_closed_form_test) {
    const std::string path = boost::filesystem::path(written(box(1.5F), "v3d_moya_volume_rib.vdb")).generic_string();
    const std::string requests = "Format 8 8 1\nPixelSamples 64 64\nProjection \"orthographic\"\nScreenWindow -0.05 0.05 -0.05 0.05\nTranslate 0 0 5\n"
        "WorldBegin\n"
        "Light \"PxrDomeLight\" \"sky\"\n"
        "Pattern \"ND_geompropvalue_float\" \"density\" \"string geomprop\" [\"density\"]\n"
        "Pattern \"ND_multiply_vector3FA\" \"scaled\" \"vector in1\" [0.2 0.6 1.2] \"reference float in2\" [\"density:out\"]\n"
        "Bxdf \"ND_anisotropic_vdf\" \"fog\" \"reference vector absorption\" [\"scaled:out\"]\n"
        "Volume \"blobbydso:impl_openvdb\" [-1 1 -1 1 -1 1] [0 0 0] \"constant string[2] blobbydso:stringargs\" [\"" + path + "\" \"density:fogvolume\"]\n"
        "WorldEnd\n";
    scene::Scene scene(scene::kinds());
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    {
        v3d::moya::ri::SceneClient client(scene, logger);
        client.frame([&scene]() {
            scene.apply();
        });
        std::istringstream stream(requests);
        v3d::rib::Reader reader(logger);
        BOOST_TEST_REQUIRE(reader.read(stream, &client));
        BOOST_TEST(reader.unrecognised().empty());
    }
    CpuDevice device;
    const film::Snapshot snapshot = integrate::render(device, scene, {}, v3d::moya::colour::Config());
    boost::filesystem::remove(path);
    const float depth = 2.0F + static_cast<float>(kVoxel);
    const glm::vec3 expected = passedOf([&](const spectrum::SampledWavelengths & wavelengths) {
        volume::Medium medium;
        medium.absorption = glm::vec3 { 0.2F, 0.6F, 1.2F } * 1.5F;
        return volume::transmittance(volume::coefficientsOf(colour::acescgTable(), medium, wavelengths).extinction(), depth);
    });
    const glm::vec3 mean = meanColour(snapshot);
    BOOST_TEST(near(mean, expected, 0.02F), mean.x << " " << mean.y << " " << mean.z << " against " << expected.x << " " << expected.y << " " << expected.z);
}

// two volumes over one box, one absorbing three times as strongly as the other, each take that
// share of the collisions of the camera rays that cross them. Cryptomatte then covers the
// stronger by three quarters of the picture's alpha and the weaker by a quarter
BOOST_AUTO_TEST_CASE(moya_volume_a_collision_names_a_volume_by_its_share_of_the_extinction_test) {
    const std::string path = written(box(1.0F), "v3d_moya_volume_shares.vdb");
    const film::Snapshot snapshot = pictured(1024, [&](scene::Scene & scene, const scene::Handle &) {
        const scene::Handle density = field(scene, "density", path, "density");
        scene.set(instanced(scene, "strong", smoke(scene, "strong", density), {}, glm::mat4(1.0F)), "material",
            scene::Value::handle(absorbing(scene, "strong", glm::vec3 { 0.6F })));
        scene.set(instanced(scene, "weak", smoke(scene, "weak", density), {}, glm::mat4(1.0F)), "material",
            scene::Value::handle(absorbing(scene, "weak", glm::vec3 { 0.2F })));
        const scene::Handle layer = scene.create("channel", "CryptoObject");
        scene.set(layer, "sourceType", scene::Value::string("raw"));
        scene.set(layer, "sourceName", scene::Value::string("crypto_object"));
        scene.set(layer, "ranks", scene::Value::integer(2));
        const scene::Handle out = scene.create("output", "picture");
        scene.set(out, "file", scene::Value::string("v3d_moya_volume_shares.exr"));
        scene.set(out, "channels", scene::Value::handles({ layer }));
    });
    boost::filesystem::remove(path);
    const std::uint32_t offset = snapshot.offset("CryptoObject00");
    double alpha = 0.0;
    double strong = 0.0;
    double weak = 0.0;
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            alpha += static_cast<double>(snapshot.at(x, y, 3U));
            strong += static_cast<double>(snapshot.at(x, y, offset + 1U));
            weak += static_cast<double>(snapshot.at(x, y, offset + 3U));
        }
    }
    BOOST_TEST(alpha > 0.5 * kSide * kSide);
    BOOST_TEST(strong / alpha == 0.75, boost::test_tools::tolerance(0.02));
    BOOST_TEST(weak / alpha == 0.25, boost::test_tools::tolerance(0.06));
}

// a box of a volume of no material scatters its density, and absorbs nothing, so inside a white
// dome it returns the dome's light however many times a path scatters in it. A light sample and
// a phase sample weigh each other through the null collisions between
BOOST_AUTO_TEST_CASE(moya_volume_a_density_that_only_scatters_returns_a_furnaces_light_test) {
    const std::string path = written(box(2.0F), "v3d_moya_volume_furnace.vdb");
    const auto furnace = [&](bool smoked) {
        return pictured(256, [&](scene::Scene & scene, const scene::Handle & options) {
            scene.set(options, "maxDepth", scene::Value::integer(1024));
            scene.set(options, "volumeDepth", scene::Value::integer(1024));
            if (smoked) {
                instanced(scene, "smoke", smoke(scene, "smoke", field(scene, "density", path, "density")), {}, glm::mat4(1.0F));
            }
        });
    };
    const glm::vec3 bare = meanColour(furnace(false));
    const glm::vec3 smoked = meanColour(furnace(true));
    boost::filesystem::remove(path);
    BOOST_TEST(near(smoked, bare, 0.02F), smoked.x << " " << smoked.y << " " << smoked.z << " against " << bare.x << " " << bare.y << " " << bare.z);
}

// a sphere of fog that OpenVDB makes from a level set holds one inside, and falls away to nothing
// at its surface over the level set's narrow band. Along its diameter it lets through e to the
// minus twice its radius, less the width of the band, times the extinction a density of one
// scatters by at each wavelength. A path that scatters ends, so the camera sees the dome through
// it
BOOST_AUTO_TEST_CASE(moya_volume_a_sphere_of_fog_lets_through_its_closed_form_test) {
    constexpr float kRadius = 1.0F;
    constexpr float kHalfWidth = 3.0F;
    openvdb::initialize();
    const openvdb::FloatGrid::Ptr fog =
        openvdb::tools::createLevelSetSphere<openvdb::FloatGrid>(kRadius, openvdb::Vec3f(0.0F), static_cast<float>(kVoxel), kHalfWidth);
    openvdb::tools::sdfToFogVolume(*fog);
    fog->setName("fog");
    const std::string path = written(fog, "v3d_moya_volume_sphere.vdb");
    const film::Snapshot snapshot = pictured(4096, [&](scene::Scene & scene, const scene::Handle & options) {
        scene.set(options, "volumeDepth", scene::Value::integer(0));
        instanced(scene, "fog", smoke(scene, "fog", field(scene, "density", path, "fog")), {}, glm::mat4(1.0F));
    });
    boost::filesystem::remove(path);
    const float depth = (2.0F * kRadius) - (kHalfWidth * static_cast<float>(kVoxel));
    const glm::vec3 expected = passedOf([&](const spectrum::SampledWavelengths & wavelengths) {
        volume::Medium medium;
        medium.lobes[0].scattering = glm::vec3 { 1.0F };
        return volume::transmittance(volume::coefficientsOf(colour::acescgTable(), medium, wavelengths).extinction(), depth);
    });
    const glm::vec3 mean = meanColour(snapshot);
    BOOST_TEST(near(mean, expected, 0.02F), mean.x << " " << mean.y << " " << mean.z << " against " << expected.x << " " << expected.y << " " << expected.z);
}

// a puff of smoke that Blender simulated and wrote through its own OpenVDB cache renders as the
// density its grid holds, placed by the grid's own transform. Along the camera's axis, which runs
// up through the middle of the puff, the picture lets through e to the minus the density that
// OpenVDB's own trilinear sampler gives along that line
BOOST_AUTO_TEST_CASE(moya_volume_a_grid_blender_wrote_renders_its_density_test) {
    const std::string path = "data/blender_smoke.vdb";
    openvdb::initialize();
    openvdb::io::File file(path);
    file.open();
    const openvdb::FloatGrid::Ptr density = openvdb::gridPtrCast<openvdb::FloatGrid>(file.readGrid("density"));
    file.close();
    BOOST_TEST_REQUIRE(density != nullptr);
    const openvdb::CoordBBox active = density->evalActiveVoxelBoundingBox();
    const openvdb::Vec3d centre = density->indexToWorld(active.getCenter());
    const openvdb::Vec3d lowest = density->indexToWorld(active.min() - openvdb::Coord(1));
    const openvdb::Vec3d highest = density->indexToWorld(active.max() + openvdb::Coord(1));
    const openvdb::tools::GridSampler<openvdb::FloatGrid, openvdb::tools::BoxSampler> sampler(*density);
    constexpr int kSteps = 20000;
    double depth = 0.0;
    for (int i = 0; i < kSteps; ++i) {
        const double z = lowest.z() + ((highest.z() - lowest.z()) * (static_cast<double>(i) + 0.5) / kSteps);
        depth += sampler.wsSample(openvdb::Vec3d(centre.x(), centre.y(), z)) * (highest.z() - lowest.z()) / kSteps;
    }
    BOOST_TEST_REQUIRE(depth > 0.1);
    const film::Snapshot snapshot = pictured(4096, [&](scene::Scene & scene, const scene::Handle & options) {
        scene.set(options, "volumeDepth", scene::Value::integer(0));
        const glm::mat4 placed = moved(-static_cast<float>(centre.x()), -static_cast<float>(centre.y()), 0.0F);
        instanced(scene, "smoke", smoke(scene, "smoke", field(scene, "density", path, "density")), {}, placed);
    }, 1.0e-4F);
    const glm::vec3 expected = passedOf([&](const spectrum::SampledWavelengths & wavelengths) {
        volume::Medium medium;
        medium.lobes[0].scattering = glm::vec3 { 1.0F };
        return volume::transmittance(volume::coefficientsOf(colour::acescgTable(), medium, wavelengths).extinction(), static_cast<float>(depth));
    });
    const glm::vec3 mean = meanColour(snapshot);
    BOOST_TEST(near(mean, expected, 0.02F), mean.x << " " << mean.y << " " << mean.z << " against " << expected.x << " " << expected.y << " " << expected.z
        << " over a depth of " << depth);
}

// a field whose file cannot be read is reported, and its volume holds nothing, so the picture is
// the bare dome's to the byte
BOOST_AUTO_TEST_CASE(moya_volume_a_grid_that_cannot_be_read_holds_nothing_test) {
    const std::string missing = (boost::filesystem::temp_directory_path() / "v3d_moya_volume_missing.vdb").string();
    boost::filesystem::remove(missing);
    const auto picture = [&](bool smoked) {
        return pictured(16, [&](scene::Scene & scene, const scene::Handle &) {
            if (smoked) {
                instanced(scene, "smoke", smoke(scene, "smoke", field(scene, "lost", missing, "density")), {}, glm::mat4(1.0F));
            }
        });
    };
    BOOST_TEST(sameBytes(picture(true), picture(false)));
    scene::Scene scene(scene::kinds());
    aimed(scene, 1);
    instanced(scene, "smoke", smoke(scene, "smoke", field(scene, "lost", missing, "density")), {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    BOOST_TEST(prototypes.volumes.empty());
    BOOST_TEST(reported(prototypes, "field lost cannot be read"));
    BOOST_TEST(reported(prototypes, "volume smoke holds nothing"));
}

// a volume in a group placed twice is placed through both chains, each ending in the volume's
// own instance, and reads its density alone when it has no material. Its bounds are its grid's,
// one voxel past its active voxels
BOOST_AUTO_TEST_CASE(moya_volume_a_volume_is_placed_through_every_chain_test) {
    const std::string path = written(box(1.0F), "v3d_moya_volume_chains.vdb");
    scene::Scene scene(scene::kinds());
    aimed(scene, 1);
    const scene::Handle group = scene.create("group", "cloud");
    const scene::Handle inner = instanced(scene, "inner", smoke(scene, "smoke", field(scene, "density", path, "density")), group, glm::mat4(1.0F));
    const scene::Handle left = instanced(scene, "left", group, {}, moved(-2.0F, 0.0F, 0.0F));
    const scene::Handle right = instanced(scene, "right", group, {}, moved(2.0F, 0.0F, 0.0F));
    scene.commit();
    scene.apply();
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    boost::filesystem::remove(path);
    BOOST_TEST_REQUIRE(prototypes.volumes.size() == 2U);
    BOOST_TEST(prototypes.grids.size() == 1U);
    std::vector<std::uint32_t> outer;
    for (const volume::Volume & placed : prototypes.volumes) {
        BOOST_TEST(placed.depth == 2U);
        BOOST_TEST(placed.chain[1] == inner.index);
        outer.push_back(placed.chain[0]);
        BOOST_TEST(placed.fieldCount == 1U);
        BOOST_TEST(placed.fields[0].name == volume::kUnread);
        const float edge = static_cast<float>((kReach + 1) * kVoxel);
        BOOST_TEST(placed.upper.x == edge, boost::test_tools::tolerance(1.0e-5F));
        BOOST_TEST(placed.lower.z == -edge, boost::test_tools::tolerance(1.0e-5F));
    }
    BOOST_TEST(((outer[0] == left.index && outer[1] == right.index) || (outer[0] == right.index && outer[1] == left.index)));
}

// a gather through a cache reads a field's grid once, and places it again from the cache until
// the field changes. A forgotten field reads its grid again, and its neighbour does not
BOOST_AUTO_TEST_CASE(moya_volume_a_cache_reads_only_a_changed_fields_grid_test) {
    const std::string first = written(box(1.0F), "v3d_moya_volume_first.vdb");
    const std::string second = written(box(2.0F), "v3d_moya_volume_second.vdb");
    scene::Scene scene(scene::kinds());
    aimed(scene, 1);
    const scene::Handle one = field(scene, "one", first, "density");
    const scene::Handle other = field(scene, "other", second, "density");
    instanced(scene, "one", smoke(scene, "one", one), {}, glm::mat4(1.0F));
    instanced(scene, "other", smoke(scene, "other", other), {}, moved(3.0F, 0.0F, 0.0F));
    scene.commit();
    scene.apply();
    geometry::Cache cache;
    const geometry::Prototypes made = geometry::gather(scene, {}, cache);
    const geometry::Prototypes kept = geometry::gather(scene, {}, cache);
    cache.forget(one.index);
    const geometry::Prototypes changed = geometry::gather(scene, {}, cache);
    boost::filesystem::remove(first);
    boost::filesystem::remove(second);
    BOOST_TEST_REQUIRE(made.grids.size() == 2U);
    BOOST_TEST_REQUIRE(kept.grids.size() == 2U);
    BOOST_TEST_REQUIRE(changed.grids.size() == 2U);
    BOOST_TEST((made.grids[0] == kept.grids[0] && made.grids[1] == kept.grids[1]));
    BOOST_TEST((changed.grids[0] != made.grids[0]));
    BOOST_TEST((changed.grids[1] == made.grids[1]));
}

};  // namespace
};  // namespace v3d::moya::tracing
