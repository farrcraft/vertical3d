/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/rib/Reader.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Compare.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/output/Png.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <Imath/ImathBox.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfCompression.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfPixelType.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

namespace film = v3d::moya::film;
namespace scene = v3d::moya::scene;

/**
 * The scenes made for the path tracer, its materials and its volumes, and those written for RI's
 * own surfaces and lights, each a RIB file in data/ with its render beside it.
 **/
constexpr const char * kScenes[] = {
    "paths-furnace",
    "paths-room",
    "paths-shapes",
    "paths-standard",
    "paths-mesh",
    "paths-linked",
    "paths-ies",
    "materials-furnace",
    "materials-lobes",
    "materials-surfaces",
    "materials-displaced",
    "volumes-fog",
    "volumes-chromatic",
    "volumes-glass",
    "volumes-smoke",
    "volumes-fire",
    "volumes-marble",
    "volumes-skin",
    "volumes-hair",
    "raytrace-lit-quad",
    "raytrace-spheres",
    "raytrace-triangle",
    "reference-area",
    "reference-bump",
    "reference-displaced",
    "reference-focus",
    "reference-glass",
    "reference-motion",
    "reference-polygon",
    "reference-shaded",
    "reference-shadow",
    "spec-cube",
    "textured",
};

/** The scenes made for materials, each of whose surfaces is OpenPBR's. **/
constexpr const char * kMaterialScenes[] = {
    "materials-furnace",
    "materials-lobes",
    "materials-surfaces",
    "materials-displaced",
};

/** The scenes of media, and of surfaces whose subsurface a walk scatters below. **/
constexpr const char * kMediaScenes[] = {
    "volumes-fog",
    "volumes-chromatic",
    "volumes-glass",
    "volumes-smoke",
    "volumes-fire",
    "volumes-marble",
    "volumes-skin",
};

/** The definitions of MaterialX's VDFs, and the coefficients each takes. **/
constexpr const char * kVdfs[] = { "ND_absorption_vdf", "ND_anisotropic_vdf" };
constexpr const char * kCoefficients[] = { "absorption", "scattering" };

/**
 * How many standard errors a pixel may lie from its committed render. The same samples are drawn
 * on every run, so a pixel lies far less than an error away unless the light changed. The bound
 * leaves room for a ray that rounding between compilers takes to the other side of an edge.
 **/
constexpr float kAllowed = 5.0F;

/** The kinds of every light the scene can hold. **/
constexpr const char * kLightKinds[] = { "cylinderLight", "diskLight", "distantLight", "domeLight", "meshLight", "rectLight", "sphereLight" };

/** The channels of a committed render: the beauty, and its luminance's standard error. **/
constexpr const char * kChannels[] = { "R", "G", "B", "error" };

/**
 * A RIB file of data/ read through the scene client and rendered through the integrator. A
 * change, when it is given, is made to the scene before it renders.
 **/
film::Snapshot rendered(const std::string & name, const std::function<void(scene::Scene &)> & change = {}) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    scene::Scene scene(scene::kinds());
    v3d::moya::ri::SceneClient client(scene, logger);
    v3d::moya::device::CpuDevice device;
    film::Snapshot out;
    client.frame([&]() {
        scene.apply();
        if (change) {
            change(scene);
            scene.commit();
            scene.apply();
        }
        out = v3d::moya::integrate::render(device, scene, logger, v3d::moya::colour::Config());
    });
    v3d::rib::Reader reader(logger);
    BOOST_TEST_REQUIRE(reader.read("data/" + name + ".rib", &client), reader.error());
    return out;
}

/** Whether a node declares a parameter of a name. **/
bool declares(const scene::Scene & scene, const scene::Handle & node, const std::string & name) {
    const std::vector<scene::Declaration> & declared = scene.primvars(node);
    return std::any_of(declared.begin(), declared.end(), [&](const scene::Declaration & declaration) {
        return declaration.name == name;
    });
}

/**
 * Multiplies a VDF's coefficient by four. A coefficient a connection feeds is fed through a node
 * that multiplies it, and one the node declares is set to four times its value. Any other is
 * MaterialX's default of zero, which stays zero.
 **/
void quadrupled(scene::Scene & scene, const scene::Handle & vdf, const std::string & input) {
    for (const scene::Handle & connection : scene.objects("connection")) {
        if (!(scene.value(connection, "to").handleArray().front() == vdf) || scene.value(connection, "input").stringArray().front() != input) {
            continue;
        }
        const scene::Handle times = scene.create("shader", scene.name(vdf) + " four times its " + input);
        scene.set(times, "language", scene::Value::string("materialx"));
        scene.set(times, "shader", scene::Value::string("ND_multiply_vector3FA"));
        scene.declare(times, scene::Declaration { .name = "in2", .fallback = scene::Value::number(1.0F) });
        scene.set(times, "in2", scene::Value::number(4.0F));
        const scene::Handle feeding = scene.create("connection", scene.name(vdf) + " into four times its " + input);
        scene.set(feeding, "from", scene.value(connection, "from"));
        scene.set(feeding, "output", scene.value(connection, "output"));
        scene.set(feeding, "to", scene::Value::handle(times));
        scene.set(feeding, "input", scene::Value::string("in1"));
        scene.set(connection, "from", scene::Value::handle(times));
        scene.set(connection, "output", scene::Value::string("out"));
        return;
    }
    if (!declares(scene, vdf, input)) {
        return;
    }
    const std::vector<float> & value = scene.value(vdf, input).floatArray();
    scene.set(vdf, input, scene::Value::vector(glm::vec3 { value[0], value[1], value[2] } * 2.0F));
}

/**
 * How many standard errors a render lies from its committed one: the further of its furthest pixel
 * and of its mean luminance over the whole picture. A change too small to show in any one noisy
 * pixel still moves the mean, whose error is far smaller.
 **/
float apart(const film::Snapshot & picture, const film::Snapshot & reference) {
    const glm::uvec2 whole = picture.resolution;
    const float pixel = film::furthest(picture, reference).sigmas;
    return std::fmax(pixel, film::sigmas(film::mean(picture, { 0, 0 }, whole), film::mean(reference, { 0, 0 }, whole)));
}

/** Writes a render as its committed EXR: the beauty's floats and each pixel's standard error. **/
void writeReference(const std::string & path, const film::Snapshot & snapshot) {
    const auto width = static_cast<int>(snapshot.resolution.x);
    Imf::Header header(width, static_cast<int>(snapshot.resolution.y));
    header.compression() = Imf::ZIP_COMPRESSION;
    Imf::FrameBuffer frame;
    // OpenEXR reads through a pointer that is not const, and writing a file reads nothing back
    char * const values = const_cast<char *>(reinterpret_cast<const char *>(snapshot.values.data()));
    for (std::size_t i = 0; i < 3; ++i) {
        header.channels().insert(kChannels[i], Imf::Channel(Imf::FLOAT));
        frame.insert(kChannels[i], Imf::Slice(Imf::FLOAT, values + (i * sizeof(float)), sizeof(float) * snapshot.stride,
                                      sizeof(float) * snapshot.stride * snapshot.resolution.x));
    }
    header.channels().insert(kChannels[3], Imf::Channel(Imf::FLOAT));
    char * const errors = const_cast<char *>(reinterpret_cast<const char *>(snapshot.errors.data()));
    frame.insert(kChannels[3], Imf::Slice(Imf::FLOAT, errors, sizeof(float), sizeof(float) * snapshot.resolution.x));
    Imf::OutputFile file(path.c_str(), header);
    file.setFrameBuffer(frame);
    file.writePixels(static_cast<int>(snapshot.resolution.y));
}

/** A committed render read back as a snapshot of the beauty and its errors. **/
film::Snapshot readReference(const std::string & path) {
    Imf::InputFile file(path.c_str());
    const Imath::Box2i & window = file.header().dataWindow();
    film::Snapshot out;
    out.resolution = glm::uvec2 { static_cast<std::uint32_t>(window.max.x - window.min.x + 1), static_cast<std::uint32_t>(window.max.y - window.min.y + 1) };
    out.stride = 4;
    const std::size_t pixels = static_cast<std::size_t>(out.resolution.x) * out.resolution.y;
    out.values.assign(pixels * 4U, 1.0F);
    out.errors.assign(pixels, 0.0F);
    Imf::FrameBuffer frame;
    for (std::size_t i = 0; i < 3; ++i) {
        frame.insert(kChannels[i], Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(out.values.data()) + (i * sizeof(float)), sizeof(float) * 4U,
                                      sizeof(float) * 4U * out.resolution.x));
    }
    frame.insert(kChannels[3], Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(out.errors.data()), sizeof(float), sizeof(float) * out.resolution.x));
    file.setFrameBuffer(frame);
    file.readPixels(window.min.y, window.max.y);
    return out;
}

/** Writes a render to data_out/, as an EXR to commit and a PNG to look at. **/
void keep(const std::string & name, const film::Snapshot & snapshot) {
    boost::filesystem::create_directories("data_out");
    writeReference("data_out/" + name + ".exr", snapshot);
    const v3d::moya::colour::Config config;
    const std::string display = config.defaultDisplay();
    v3d::moya::output::writePng("data_out/" + name + ".png", snapshot, config.display(display, config.defaultView(display)),
        boost::make_shared<v3d::log::Logger>());
}

/**
 * An RGB build carries no wavelengths, so its pictures are not the committed ones, which are
 * rendered by a spectral build.
 **/
#ifndef MOYA_RGB

// each scene renders as its committed render, every pixel and the picture's mean within kAllowed
// of it. A scene with no committed render yet, or one whose pixels differ, leaves what it
// rendered in data_out/
BOOST_AUTO_TEST_CASE(moya_paths_references_every_scene_renders_as_committed_test) {
    for (const char * name : kScenes) {
        BOOST_TEST_CONTEXT(name) {
            const film::Snapshot picture = rendered(name);
            const std::string reference = "data/" + std::string(name) + ".exr";
            if (!boost::filesystem::exists(reference)) {
                keep(name, picture);
                BOOST_TEST(false, "no committed render; data_out/ holds this one");
                continue;
            }
            const film::Furthest furthest = film::furthest(picture, readReference(reference));
            if (!(furthest.sigmas < kAllowed)) {
                keep(name, picture);
            }
            BOOST_TEST(furthest.sigmas < kAllowed, "pixel " << furthest.pixel.x << ", " << furthest.pixel.y << " lies " << furthest.sigmas << " errors away");
            BOOST_TEST(apart(picture, readReference(reference)) < kAllowed, "the mean lies " << apart(picture, readReference(reference)) << " errors away");
            BOOST_TEST(furthest.unmeasured == 0U);
        }
    }
}

// a light twice as bright fails the comparison in every scene, so the comparison sees a change of
// light and not only a change of picture. RI's constant surface emits its colour, and is the only
// light of a scene that has no other, so it is twice as bright too
BOOST_AUTO_TEST_CASE(moya_paths_references_a_brighter_light_fails_the_comparison_test) {
    for (const char * name : kScenes) {
        BOOST_TEST_CONTEXT(name) {
            const film::Snapshot brighter = rendered(name, [](scene::Scene & scene) {
                for (const char * kind : kLightKinds) {
                    for (const scene::Handle & light : scene.objects(kind)) {
                        scene.set(light, "exposure", scene::Value::number(scene.value(light, "exposure").floatArray().front() + 1.0F));
                    }
                }
                for (const scene::Handle & node : scene.objects("shader")) {
                    if (scene.value(node, "language").stringArray().front() != "ri" || scene.value(node, "shader").stringArray().front() != "constant") {
                        continue;
                    }
                    const scene::Value & colour = scene.value(node, "Cs");
                    const std::vector<float> & cs = colour.floatArray();
                    scene.set(node, "Cs", scene::Value::colour(glm::vec3 { cs[0], cs[1], cs[2] } * 2.0F, colour.space()));
                }
            });
            BOOST_TEST(apart(brighter, readReference("data/" + std::string(name) + ".exr")) > kAllowed);
        }
    }
}

// a base of half the weight fails the comparison in every material scene, so the comparison sees
// a change of material and not only a change of light
BOOST_AUTO_TEST_CASE(moya_paths_references_a_changed_material_fails_the_comparison_test) {
    for (const char * name : kMaterialScenes) {
        BOOST_TEST_CONTEXT(name) {
            const film::Snapshot halved = rendered(name, [](scene::Scene & scene) {
                for (const scene::Handle & node : scene.objects("shader")) {
                    if (scene.value(node, "shader").stringArray().front() != "ND_open_pbr_surface_surfaceshader") {
                        continue;
                    }
                    scene.declare(node, scene::Declaration { .name = "base_weight", .fallback = scene::Value::number(1.0F) });
                    scene.set(node, "base_weight", scene::Value::number(0.5F));
                }
            });
            BOOST_TEST(apart(halved, readReference("data/" + std::string(name) + ".exr")) > kAllowed);
        }
    }
}

// a changed medium fails the comparison in every scene of media or subsurface, so the comparison
// sees a change of medium and not only a change of light. Each VDF's coefficients are four times
// as large, and each subsurface colour a surface sets is halved
BOOST_AUTO_TEST_CASE(moya_paths_references_a_changed_medium_fails_the_comparison_test) {
    for (const char * name : kMediaScenes) {
        BOOST_TEST_CONTEXT(name) {
            const film::Snapshot changed = rendered(name, [](scene::Scene & scene) {
                for (const scene::Handle & node : scene.objects("shader")) {
                    const std::string & shader = scene.value(node, "shader").stringArray().front();
                    for (const char * vdf : kVdfs) {
                        if (shader != vdf) {
                            continue;
                        }
                        for (const char * input : kCoefficients) {
                            if (shader != "ND_absorption_vdf" || std::string(input) == "absorption") {
                                quadrupled(scene, node, input);
                            }
                        }
                    }
                    if (shader == "ND_open_pbr_surface_surfaceshader" && declares(scene, node, "subsurface_color")) {
                        const scene::Value & colour = scene.value(node, "subsurface_color");
                        const std::vector<float> & held = colour.floatArray();
                        scene.set(node, "subsurface_color", scene::Value::colour(glm::vec3 { held[0], held[1], held[2] } * 0.5F, colour.space()));
                    }
                }
            });
            BOOST_TEST(apart(changed, readReference("data/" + std::string(name) + ".exr")) > kAllowed);
        }
    }
}

#endif

};  // namespace
