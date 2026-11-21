/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/Surfaces.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/Lights.h>
#include <moya/libmoya/light/Lights.h>
#include <moya/libmoya/light/Shading.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/volume/Volumes.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfRgba.h>
#include <OpenEXR/ImfRgbaFile.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include "Staged.h"
#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace colour = v3d::moya::colour;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace light = v3d::moya::light;
namespace scene = v3d::moya::scene;
namespace volume = v3d::moya::volume;
typedef v3d::moya::device::CpuDevice CpuDevice;
typedef std::vector<std::uint32_t> Indices;

/** The radius of the sphere that bounds the world, which every update is given. **/
constexpr float kReach = 10.0F;

/**
 * A scene of every kind of light. A rect light reads a texture and an IES profile, and a sphere
 * light has a shape. A point reads another profile. A distant light stands afar, and so does a
 * dome light that reads an image of its own. A mesh light makes a panel emit, a lamp's material
 * emits, and a floor's material does not.
 **/
struct Lit {
    Staged staged { 4 };
    scene::Scene & scene = staged.made;
    scene::Handle camera;
    scene::Handle rect;
    scene::Handle sphere;
    scene::Handle point;
    scene::Handle sun;
    scene::Handle dome;
    scene::Handle glow;
    scene::Handle panel;
    scene::Handle lamp;
    scene::Handle floor;
    scene::Handle glowing;
    scene::Handle matte;

    /** An EXR image of a side, each texel a colour, in the temporary directory. Gives its path. **/
    static std::string imageFile(const std::string & name, int side, const glm::vec3 & colour) {
        const std::string path = (boost::filesystem::temp_directory_path() / ("v3d_lights_kept_" + name + ".exr")).string();
        const Imf::Header header(side, side);
        Imf::RgbaOutputFile file(path.c_str(), header, Imf::WRITE_RGB);
        std::vector<Imf::Rgba> pixels(static_cast<std::size_t>(side) * static_cast<std::size_t>(side), Imf::Rgba(colour.x, colour.y, colour.z, 1.0F));
        file.setFrameBuffer(pixels.data(), 1, static_cast<std::size_t>(side));
        file.writePixels(side);
        return path;
    }

    /**
     * An IES file of type C photometry over two vertical angles, of a candela, in the temporary
     * directory. Gives its path.
     **/
    static std::string iesFile(const std::string & name, float candela) {
        const std::string path = (boost::filesystem::temp_directory_path() / ("v3d_lights_kept_" + name + ".ies")).string();
        std::ofstream(path, std::ios::binary) << "IESNA:LM-63-2002\n[TEST] moya\nTILT=NONE\n1 -1 1 2 1 1 1 0 0 0\n1 1 0\n0 90\n0\n"
                                              << candela << " " << (0.5F * candela) << "\n";
        return path;
    }

    /** The handle a parameter of an object names. **/
    static scene::Handle named(const scene::Scene & scene, const scene::Handle & object, const std::string & parameter) {
        return scene.value(object, parameter).handleArray().front();
    }

    Lit() {
        rect = scene.create("rectLight", "rect");
        scene.set(rect, "transform", scene::Value::matrix(moved(0.0F, 1.0F, 2.0F)));
        scene.set(rect, "texture", scene::Value::string(imageFile("rect", 4, glm::vec3 { 1.0F, 0.5F, 0.25F })));
        scene.set(rect, "iesFile", scene::Value::string(iesFile("rect", 2.0F)));
        sphere = scene.create("sphereLight", "sphere");
        scene.set(sphere, "radius", scene::Value::number(0.5F));
        scene.set(sphere, "transform", scene::Value::matrix(moved(2.0F, 0.0F, 1.0F)));
        point = scene.create("sphereLight", "point");
        scene.set(point, "radius", scene::Value::number(0.0F));
        scene.set(point, "iesFile", scene::Value::string(iesFile("point", 3.0F)));
        sun = scene.create("distantLight", "sun");
        scene.set(sun, "angle", scene::Value::number(1.0F));
        dome = scene.create("domeLight", "sky");
        scene.set(dome, "texture", scene::Value::string(imageFile("sky", 8, glm::vec3 { 0.25F, 0.5F, 1.0F })));
        glowing = staged.material("glowing", { { "emission_luminance", scene::Value::number(2.0F) } });
        matte = staged.material("matte", { { "base_color", scene::Value::colour(glm::vec3 { 0.5F }) } });
        const scene::Handle panelMesh = staged.quad("panel", { { -1.0F, -1.0F, 3.0F }, { 1.0F, -1.0F, 3.0F }, { 1.0F, 1.0F, 3.0F }, { -1.0F, 1.0F, 3.0F } }, {});
        glow = scene.create("meshLight", "glow");
        scene.set(glow, "geometry", scene::Value::handle(panelMesh));
        staged.quad("lamp", { { 2.0F, -1.0F, 3.0F }, { 3.0F, -1.0F, 3.0F }, { 3.0F, 1.0F, 3.0F }, { 2.0F, 1.0F, 3.0F } }, glowing);
        staged.quad("floor", { { -4.0F, -4.0F, 0.0F }, { 4.0F, -4.0F, 0.0F }, { 4.0F, 4.0F, 0.0F }, { -4.0F, 4.0F, 0.0F } }, matte);
        committed();
        // what a client created is held once its batch is applied
        camera = named(scene, staged.options, "camera");
        panel = *scene.find("instance", "panel");
        lamp = *scene.find("instance", "lamp");
        floor = *scene.find("instance", "floor");
    }

    /** Applies what was edited as a version of its own. **/
    void committed() {
        scene.commit();
        scene.apply();
    }

    /** The node of a material's surface. **/
    scene::Handle surfaceOf(const scene::Handle & material) const {
        return named(scene, material, "surface");
    }
};

/** Lights kept across the versions of a scene. Each update is given its version's prototypes. **/
struct Kept {
    colour::Config config;
    CpuDevice device;
    light::Lights lights { device, config, {} };

    void update(const scene::Scene & scene) {
        const geometry::Prototypes prototypes = geometry::gather(scene, {});
        const geometry::Surfaces surfaces(device, prototypes, config);
        const volume::Volumes volumes(device, prototypes);
        lights.update(scene, prototypes, light::Shading { .surfaces = surfaces.view(), .textures = {}, .volumes = volumes.view() }, kReach);
    }
};

/** Whether two runs hold the same bytes. **/
template <typename T>
bool same(const kernel::Array<T> & left, const kernel::Array<T> & right) {
    return left.size == right.size && (left.size == 0U || std::memcmp(left.data, right.data, sizeof(T) * left.size) == 0);
}

/**
 * Whether two values hold the same bytes. A float is compared by its bytes, so a zero of either
 * sign and a NaN are told apart as the device reads them.
 **/
template <typename T>
bool sameValue(const T & left, const T & right) {
    // NOLINTNEXTLINE(bugprone-suspicious-memory-comparison) - the bytes are what is compared
    return std::memcmp(&left, &right, sizeof(T)) == 0;
}

/** Whether two lights hold the same bytes in every field, which leaves out what pads them. **/
bool sameLight(const kernel::light::Light & left, const kernel::light::Light & right) {
    return sameValue(left.id, right.id) && sameValue(left.shape, right.shape) && sameValue(left.size, right.size) && sameValue(left.transform, right.transform) &&
        sameValue(left.keys, right.keys) && sameValue(left.keyCount, right.keyCount) && sameValue(left.colour, right.colour) && sameValue(left.scale, right.scale) &&
        sameValue(left.temperature, right.temperature) && sameValue(left.planckScale, right.planckScale) && sameValue(left.planckColour, right.planckColour) &&
        sameValue(left.axis, right.axis) && sameValue(left.coneAngle, right.coneAngle) && sameValue(left.coneSoftness, right.coneSoftness) &&
        sameValue(left.chain, right.chain) && sameValue(left.depth, right.depth) && sameValue(left.geometry, right.geometry) &&
        sameValue(left.triangles, right.triangles) && sameValue(left.count, right.count) && sameValue(left.tree, right.tree) && sameValue(left.leaves, right.leaves) &&
        sameValue(left.bricks, right.bricks) && left.camera == right.camera && sameValue(left.image, right.image) && sameValue(left.profile, right.profile) &&
        left.shadowLinked == right.shadowLinked && sameValue(left.shadowFirst, right.shadowFirst) && sameValue(left.shadowCount, right.shadowCount) &&
        sameValue(left.label, right.label) && left.object == right.object && left.twoSided == right.twoSided;
}

/**
 * Whether kept lights hold, byte for byte, everything the lights made from nothing for the same
 * scene hold, with the same labels.
 **/
void agrees(Kept & kept, const scene::Scene & scene) {
    const geometry::Prototypes prototypes = geometry::gather(scene, {});
    const geometry::Surfaces surfaces(kept.device, prototypes, kept.config);
    const volume::Volumes volumes(kept.device, prototypes);
    const light::Lights fresh(kept.device, scene, prototypes, kept.config,
        light::Shading { .surfaces = surfaces.view(), .textures = {}, .volumes = volumes.view() }, kReach, {});
    const kernel::light::Lights want = fresh.view();
    const kernel::light::Lights got = kept.lights.view();
    bool lights = want.lights.size == got.lights.size;
    for (std::uint32_t i = 0; lights && i < want.lights.size; ++i) {
        lights = sameLight(want.lights[i], got.lights[i]);
    }
    const std::vector<std::pair<const char *, bool>> runs { { "lights", lights }, { "keys", same(want.keys, got.keys) },
        { "distant", same(want.distant, got.distant) }, { "byId", same(want.byId, got.byId) }, { "cdf", same(want.cdf, got.cdf) },
        { "triangles", same(want.triangles, got.triangles) }, { "meshes", same(want.meshes, got.meshes) }, { "images", same(want.images, got.images) },
        { "texels", same(want.texels, got.texels) }, { "sums", same(want.sums, got.sums) }, { "domes", same(want.domes, got.domes) },
        { "nodes", same(want.nodes, got.nodes) }, { "roots", same(want.roots, got.roots) }, { "leaves", same(want.leaves, got.leaves) },
        { "triangleLeaves", same(want.triangleLeaves, got.triangleLeaves) }, { "infinite", same(want.infinite, got.infinite) },
        { "infiniteFirst", same(want.infiniteFirst, got.infiniteFirst) }, { "bricks", same(want.bricks, got.bricks) }, { "cells", same(want.cells, got.cells) },
        { "glowing", same(want.glowing, got.glowing) }, { "enclosing", same(want.enclosing, got.enclosing) },
        { "linkFirst", same(want.links.first, got.links.first) }, { "linkSteps", same(want.links.steps, got.links.steps) },
        { "lit", same(want.links.lit, got.links.lit) }, { "linkCount", want.links.count == got.links.count },
        { "casters", same(want.links.casters, got.links.casters) }, { "profiles", same(want.profiles, got.profiles) },
        { "photometry", same(want.photometry, got.photometry) }, { "labels", fresh.labels() == kept.lights.labels() } };
    for (const auto & [name, held] : runs) {
        BOOST_TEST_CONTEXT(name) {
            BOOST_TEST(held);
        }
    }
}

/** Lights kept from the first version of a scene, then updated after an edit. **/
template <typename Edit>
void edited(Lit & lit, Kept & kept, Edit && edit) {
    kept.update(lit.scene);
    edit();
    lit.committed();
    kept.update(lit.scene);
    BOOST_TEST(kept.lights.version() == lit.scene.version());
}

// the first update makes every light of the scene and builds the tree, and one after no edit
// makes and builds nothing
BOOST_AUTO_TEST_CASE(moya_lights_kept_the_first_update_makes_every_light_test) {
    const Lit lit;
    Kept kept;
    kept.update(lit.scene);
    BOOST_TEST((kept.lights.madeLights() == Indices { lit.rect.index, lit.sphere.index, lit.point.index, lit.sun.index, lit.dome.index, lit.glow.index }));
    BOOST_TEST(kept.lights.builtTree());
    agrees(kept, lit.scene);
    BOOST_TEST(kept.lights.view().profiles.size == 2U);
    BOOST_TEST(kept.lights.view().images.size == 2U);
    kept.update(lit.scene);
    BOOST_TEST(kept.lights.madeLights().empty());
    BOOST_TEST(!kept.lights.builtTree());
    agrees(kept, lit.scene);
}

// a light whose parameter changed is made again, the others are kept, and the tree is built again
BOOST_AUTO_TEST_CASE(moya_lights_kept_a_lights_parameter_makes_it_again_test) {
    Lit lit;
    Kept kept;
    edited(lit, kept, [&] { lit.scene.set(lit.point, "intensity", scene::Value::number(3.0F)); });
    BOOST_TEST((kept.lights.madeLights() == Indices { lit.point.index }));
    BOOST_TEST(kept.lights.builtTree());
    agrees(kept, lit.scene);
}

// a light moved is made again where it now stands, and the tree is built again
BOOST_AUTO_TEST_CASE(moya_lights_kept_a_light_moved_makes_it_again_test) {
    Lit lit;
    Kept kept;
    edited(lit, kept, [&] { lit.scene.set(lit.sphere, "transform", scene::Value::matrix(moved(-2.0F, 0.0F, 1.0F))); });
    BOOST_TEST((kept.lights.madeLights() == Indices { lit.sphere.index }));
    BOOST_TEST(kept.lights.builtTree());
    agrees(kept, lit.scene);
}

// a light created is made, and takes its place among the kept ones
BOOST_AUTO_TEST_CASE(moya_lights_kept_a_light_created_is_made_test) {
    Lit lit;
    Kept kept;
    scene::Handle disk;
    edited(lit, kept, [&] {
        disk = lit.scene.create("diskLight", "disk");
        lit.scene.set(disk, "transform", scene::Value::matrix(moved(0.0F, -2.0F, 2.0F)));
    });
    BOOST_TEST((kept.lights.madeLights() == Indices { disk.index }));
    BOOST_TEST(kept.lights.builtTree());
    agrees(kept, lit.scene);
}

// a light removed leaves the kept ones, whose images and profiles then sit where the lights made
// from nothing read them
BOOST_AUTO_TEST_CASE(moya_lights_kept_a_light_removed_leaves_them_test) {
    Lit lit;
    Kept kept;
    edited(lit, kept, [&] { lit.scene.remove(lit.rect); });
    BOOST_TEST(kept.lights.madeLights().empty());
    BOOST_TEST(kept.lights.builtTree());
    BOOST_TEST(kept.lights.view().profiles.size == 1U);
    BOOST_TEST(kept.lights.view().images.size == 1U);
    agrees(kept, lit.scene);
}

// an instance moved that places the geometry a mesh light names builds the tree again, and makes
// no light of the scene again
BOOST_AUTO_TEST_CASE(moya_lights_kept_a_mesh_lights_instance_moved_builds_the_tree_test) {
    Lit lit;
    Kept kept;
    edited(lit, kept, [&] { lit.scene.set(lit.panel, "transform", scene::Value::matrix(moved(0.0F, 0.0F, 1.0F))); });
    BOOST_TEST(kept.lights.madeLights().empty());
    BOOST_TEST(kept.lights.builtTree());
    agrees(kept, lit.scene);
}

// an instance moved that places a geometry its material makes emit builds the tree again
BOOST_AUTO_TEST_CASE(moya_lights_kept_an_emitting_instance_moved_builds_the_tree_test) {
    Lit lit;
    Kept kept;
    edited(lit, kept, [&] { lit.scene.set(lit.lamp, "transform", scene::Value::matrix(glm::rotate(glm::mat4(1.0F), glm::radians(30.0F), glm::vec3 { 0.0F, 0.0F, 1.0F }))); });
    BOOST_TEST(kept.lights.madeLights().empty());
    BOOST_TEST(kept.lights.builtTree());
    agrees(kept, lit.scene);
}

// a material that emits, and changes what it emits, builds the tree again
BOOST_AUTO_TEST_CASE(moya_lights_kept_an_emitting_material_changed_builds_the_tree_test) {
    Lit lit;
    Kept kept;
    edited(lit, kept, [&] { lit.scene.set(lit.surfaceOf(lit.glowing), "emission_luminance", scene::Value::number(5.0F)); });
    BOOST_TEST(kept.lights.madeLights().empty());
    BOOST_TEST(kept.lights.builtTree());
    agrees(kept, lit.scene);
}

// a material that comes to emit builds the tree again, though it emitted nothing before
BOOST_AUTO_TEST_CASE(moya_lights_kept_a_material_that_comes_to_emit_builds_the_tree_test) {
    Lit lit;
    Kept kept;
    edited(lit, kept, [&] {
        const scene::Handle surface = lit.surfaceOf(lit.matte);
        const scene::Value luminance = scene::Value::number(1.0F);
        lit.scene.declare(surface, scene::Declaration { .name = "emission_luminance", .type = luminance.type(), .count = 1, .fallback = luminance });
        lit.scene.set(surface, "emission_luminance", luminance);
    });
    BOOST_TEST(kept.lights.madeLights().empty());
    BOOST_TEST(kept.lights.builtTree());
    agrees(kept, lit.scene);
}

// a camera's edit rebuilds nothing of the lights
BOOST_AUTO_TEST_CASE(moya_lights_kept_a_camera_edit_rebuilds_nothing_test) {
    Lit lit;
    Kept kept;
    edited(lit, kept, [&] { lit.scene.set(lit.camera, "transform", scene::Value::matrix(moved(1.0F, 0.0F, -6.0F))); });
    BOOST_TEST(kept.lights.madeLights().empty());
    BOOST_TEST(!kept.lights.builtTree());
    agrees(kept, lit.scene);
}

// an option's edit rebuilds nothing of the lights
BOOST_AUTO_TEST_CASE(moya_lights_kept_an_option_edit_rebuilds_nothing_test) {
    Lit lit;
    Kept kept;
    edited(lit, kept, [&] { lit.scene.set(lit.staged.options, "samples", scene::Value::integer(16)); });
    BOOST_TEST(kept.lights.madeLights().empty());
    BOOST_TEST(!kept.lights.builtTree());
    agrees(kept, lit.scene);
}

// a material's edit that leaves it emitting nothing rebuilds nothing of the lights, and nor does
// moving the instance it is on
BOOST_AUTO_TEST_CASE(moya_lights_kept_a_material_that_does_not_emit_rebuilds_nothing_test) {
    Lit lit;
    Kept kept;
    edited(lit, kept, [&] {
        lit.scene.set(lit.surfaceOf(lit.matte), "base_color", scene::Value::colour(glm::vec3 { 0.25F }));
        lit.scene.set(lit.floor, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -1.0F)));
    });
    BOOST_TEST(kept.lights.madeLights().empty());
    BOOST_TEST(!kept.lights.builtTree());
    agrees(kept, lit.scene);
}

// with a light linked to the instances it names, an instance moved builds the tree again, since
// the sets of linked lights follow the chains of instances
BOOST_AUTO_TEST_CASE(moya_lights_kept_a_linked_lights_instances_build_the_tree_test) {
    Lit lit;
    lit.scene.set(lit.sphere, "includeRoot", scene::Value::integer(0));
    lit.scene.set(lit.sphere, "instances", scene::Value::handles({ lit.floor }));
    lit.committed();
    Kept kept;
    edited(lit, kept, [&] { lit.scene.set(lit.floor, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -1.0F))); });
    BOOST_TEST(kept.lights.madeLights().empty());
    BOOST_TEST(kept.lights.builtTree());
    agrees(kept, lit.scene);
}

};  // namespace
};  // namespace v3d::moya::tracing
