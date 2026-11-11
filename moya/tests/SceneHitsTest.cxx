/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/render/offline/film/FrameBuffer.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/camera/Camera.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/state/Planes.h>
#include <moya/libmoya/view/Hits.h>

#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfPixelType.h>

#include <Imath/ImathBox.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include "Reference.h"

namespace v3d::moya::reference {
namespace {

namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
typedef v3d::moya::state::Planes Planes;

/** The hit view's channels as an EXR file names them, in the snapshot's order after the beauty. **/
constexpr std::array<const char *, 7> kChannels { "depth", "normal.R", "normal.G", "normal.B", "instance", "face", "coverage" };

/** A RIB file read into the new core's scene, as moya --hits reads it. **/
class FromFile final {
 public:
    explicit FromFile(const boost::filesystem::path & file) : scene_(scene::kinds()), logger_(boost::make_shared<v3d::log::Logger>()) {
        v3d::moya::ri::SceneClient client(scene_, logger_);
        client.frame([this]() {
            scene_.apply();
        });
        std::ifstream stream(file.string(), std::ios::binary);
        v3d::render::offline::rib::Reader reader(logger_);
        BOOST_REQUIRE(reader.read(stream, &client));
    }

    const scene::Scene & scene() const {
        return scene_;
    }

    const boost::shared_ptr<v3d::log::Logger> & logger() const {
        return logger_;
    }

 private:
    scene::Scene scene_;
    boost::shared_ptr<v3d::log::Logger> logger_;
};

/** A hit view: its size, and each channel's floats by name, row by row from the top left. **/
struct Picture {
    std::uint32_t width { 0 };
    std::uint32_t height { 0 };
    std::map<std::string, std::vector<float>> channels;

    float at(const std::string & channel, std::uint32_t x, std::uint32_t y) const {
        return channels.at(channel)[(static_cast<std::size_t>(y) * width) + x];
    }
};

/** The scenes that hold still and are not displaced, whose depth the ray hider's must match. **/
const std::set<std::string> & stillScenes() {
    static const std::set<std::string> scenes { "raytrace-area", "raytrace-bump", "raytrace-focus", "raytrace-lit-quad", "raytrace-sampled",
        "raytrace-shaded", "raytrace-spheres", "raytrace-triangle", "reference-area", "reference-bump", "reference-focus", "reference-glass",
        "reference-polygon", "reference-sampled", "reference-shaded", "reference-shadow", "spec-cube", "textured" };
    return scenes;
}

/** A committed hit view, read from its EXR file. **/
Picture readHits(const boost::filesystem::path & file) {
    Imf::InputFile input(file.string().c_str());
    const Imath::Box2i window = input.header().dataWindow();
    Picture out;
    out.width = static_cast<std::uint32_t>(window.max.x - window.min.x + 1);
    out.height = static_cast<std::uint32_t>(window.max.y - window.min.y + 1);
    Imf::FrameBuffer buffer;
    for (const char * name : kChannels) {
        std::vector<float> & values = out.channels[name];
        values.resize(static_cast<std::size_t>(out.width) * out.height);
        char * base = reinterpret_cast<char *>(values.data()) -
            (((static_cast<std::ptrdiff_t>(window.min.y) * out.width) + window.min.x) * static_cast<std::ptrdiff_t>(sizeof(float)));
        buffer.insert(name, Imf::Slice(Imf::FLOAT, base, sizeof(float), sizeof(float) * out.width));
    }
    input.setFrameBuffer(buffer);
    input.readPixels(window.min.y, window.max.y);
    return out;
}

/** A hit view as the snapshot holds it, by the channels' names. **/
Picture pictureOf(const v3d::moya::film::Snapshot & snapshot) {
    Picture out;
    out.width = snapshot.resolution.x;
    out.height = snapshot.resolution.y;
    for (std::size_t channel = 0; channel < kChannels.size(); ++channel) {
        std::vector<float> & values = out.channels[kChannels[channel]];
        for (std::uint32_t y = 0; y < out.height; ++y) {
            for (std::uint32_t x = 0; x < out.width; ++x) {
                values.push_back(snapshot.at(x, y, 3 + static_cast<std::uint32_t>(channel)));
            }
        }
    }
    return out;
}

/** Whether a pixel's ray met something other than one of its neighbours' rays met. **/
bool onEdge(const Picture & picture, std::uint32_t x, std::uint32_t y) {
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const int nx = static_cast<int>(x) + dx;
            const int ny = static_cast<int>(y) + dy;
            if (nx < 0 || ny < 0 || nx >= static_cast<int>(picture.width) || ny >= static_cast<int>(picture.height)) {
                continue;
            }
            for (const char * channel : { "instance", "face", "coverage" }) {
                if (picture.at(channel, static_cast<std::uint32_t>(nx), static_cast<std::uint32_t>(ny)) != picture.at(channel, x, y)) {
                    return true;
                }
            }
        }
    }
    return false;
}

/** Whether two depths agree, both infinite or within a ten thousandth of the farther. **/
bool sameDepth(float got, float want) {
    if (std::isinf(want) || std::isinf(got)) {
        return got == want;
    }
    return std::fabs(got - want) <= 1.0e-4F * std::max(1.0F, std::fabs(want));
}

/**
 * The depth along a camera's view of a depth the old hiders' depth plane holds. The plane holds
 * RI's screen depth moved onto [0, 2]: the clipping planes taken to 0 and 2, linearly under an
 * orthographic camera, and as a perspective camera divides by depth under one.
 **/
float depthOf(float raster, const v3d::moya::kernel::camera::Camera & camera) {
    const float near = camera.near;
    const float far = camera.far;
    if (!camera.perspective) {
        return near + (raster * (far - near) * 0.5F);
    }
    const float screen = raster - 1.0F;
    return (2.0F * far * near / (far - near)) / (((far + near) / (far - near)) - screen);
}

/** The scene files the suite renders, by their names without the extension. **/
std::vector<boost::filesystem::path> sceneFiles() {
    std::vector<boost::filesystem::path> out;
    for (const boost::filesystem::directory_entry & entry : boost::filesystem::directory_iterator("data")) {
        if (entry.path().extension() == ".rib") {
            out.push_back(entry.path());
        }
    }
    std::ranges::sort(out);
    return out;
}

/**
 * Every scene's hit view is its committed one: depths and normals to rounding, and what each ray
 * met exactly, except on an edge, where rounding can take a ray to either side.
 *
 * Covers the base: e8e902d8 added the hit view and moya --hits. Checked by widening the camera's
 * field of view by a tenth, by making a sphere a hundredth larger, and by making every depth a
 * thousandth longer, each of which fails this case.
 **/
BOOST_AUTO_TEST_CASE(moya_scenehits_every_scene_traces_as_committed_test) {
    v3d::moya::device::CpuDevice device;
    unsigned int scenes = 0;
    for (const boost::filesystem::path & file : sceneFiles()) {
        const std::string name = file.stem().string();
        const FromFile read(file);
        const Picture got = pictureOf(view::hits(device, read.scene(), read.logger()));
        const Picture want = readHits(boost::filesystem::path("data") / "scenes" / (name + ".exr"));
        BOOST_REQUIRE_EQUAL(got.width, want.width);
        BOOST_REQUIRE_EQUAL(got.height, want.height);
        std::size_t apart = 0;
        for (std::uint32_t y = 0; y < want.height; ++y) {
            for (std::uint32_t x = 0; x < want.width; ++x) {
                if (onEdge(want, x, y)) {
                    continue;
                }
                bool same = sameDepth(got.at("depth", x, y), want.at("depth", x, y));
                for (const char * channel : { "normal.R", "normal.G", "normal.B" }) {
                    same = same && std::fabs(got.at(channel, x, y) - want.at(channel, x, y)) <= 1.0e-4F;
                }
                for (const char * channel : { "instance", "face", "coverage" }) {
                    same = same && got.at(channel, x, y) == want.at(channel, x, y);
                }
                apart += same ? 0 : 1;
            }
        }
        BOOST_TEST(apart == 0U, name);
        scenes++;
    }
    BOOST_TEST(scenes == 29U);
}

/**
 * Each scene that holds still and is not displaced lies where the ray hider finds it: at every
 * pixel inside one surface, the hit view's depth is the ray hider's depth plane. The ray hider
 * takes one sample at each pixel's centre, as the hit view does, unless the scene names its own
 * sampling, so a curved surface's depth is not averaged across a pixel.
 *
 * Covers the base: e8e902d8 added the hit view and moya --hits. Checked by widening the camera's
 * field of view by a tenth, by making a sphere a hundredth larger, and by making every depth a
 * thousandth longer, each of which fails this case.
 **/
BOOST_AUTO_TEST_CASE(moya_scenehits_depth_matches_the_ray_hiders_test) {
    v3d::moya::device::CpuDevice device;
    unsigned int scenes = 0;
    for (const boost::filesystem::path & file : sceneFiles()) {
        const std::string name = file.stem().string();
        if (!stillScenes().contains(name)) {
            continue;
        }
        const FromFile read(file);
        const Picture hits = pictureOf(view::hits(device, read.scene(), read.logger()));
        const v3d::moya::kernel::camera::Camera camera = v3d::moya::camera::fromScene(read.scene());
        const std::string path = file.string();
        const boost::shared_ptr<v3d::render::offline::film::FrameBuffer> traced = reference::read({ .file = path.c_str(), .hider = "raytrace", .centres = true });
        std::size_t compared = 0;
        std::size_t apart = 0;
        float furthest = 0.0F;
        for (std::uint32_t y = 0; y < hits.height; ++y) {
            for (std::uint32_t x = 0; x < hits.width; ++x) {
                if (hits.at("coverage", x, y) == 0.0F || onEdge(hits, x, y) || traced->value(Planes::COVERAGE, x, y) < 1.0F) {
                    continue;
                }
                const float want = depthOf(traced->value(Planes::DEPTH, x, y), camera);
                const float got = hits.at("depth", x, y);
                furthest = std::max(furthest, std::fabs(got - want) / std::max(1.0F, std::fabs(want)));
                apart += std::fabs(got - want) <= 1.0e-4F * std::max(1.0F, std::fabs(want)) ? 0 : 1;
                compared++;
            }
        }
        BOOST_TEST(compared > 0U, name);
        BOOST_TEST(apart == 0U, name << ", furthest apart " << furthest);
        scenes++;
    }
    BOOST_TEST(scenes == stillScenes().size());
}

};  // namespace
};  // namespace v3d::moya::reference
