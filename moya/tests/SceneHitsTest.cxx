/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
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
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;

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
    const std::uint32_t first = snapshot.offset("depth");
    for (std::size_t channel = 0; channel < kChannels.size(); ++channel) {
        std::vector<float> & values = out.channels[kChannels[channel]];
        for (std::uint32_t y = 0; y < out.height; ++y) {
            for (std::uint32_t x = 0; x < out.width; ++x) {
                values.push_back(snapshot.at(x, y, first + static_cast<std::uint32_t>(channel)));
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
    BOOST_TEST(scenes == 31U);
}

/**
 * Every scene's hit view through the hierarchy is its hit view through the brute-force traversal,
 * which tests each triangle in turn. Depths and normals agree to rounding. What each ray met
 * agrees exactly, except on an edge of either view.
 *
 * Checked by making a leaf of the hierarchy test one triangle fewer than it holds, which fails
 * this case.
 **/
BOOST_AUTO_TEST_CASE(moya_scenehits_the_hierarchy_finds_the_brute_force_traversals_hits_test) {
    v3d::moya::device::CpuDevice device;
    unsigned int scenes = 0;
    for (const boost::filesystem::path & file : sceneFiles()) {
        const std::string name = file.stem().string();
        const FromFile read(file);
        const Picture got = pictureOf(view::hits(device, read.scene(), read.logger(), 0.0F, view::Tracer::Hierarchy));
        const Picture want = pictureOf(view::hits(device, read.scene(), read.logger(), 0.0F, view::Tracer::BruteForce));
        BOOST_REQUIRE_EQUAL(got.width, want.width);
        BOOST_REQUIRE_EQUAL(got.height, want.height);
        std::size_t apart = 0;
        for (std::uint32_t y = 0; y < want.height; ++y) {
            for (std::uint32_t x = 0; x < want.width; ++x) {
                if (onEdge(want, x, y) || onEdge(got, x, y)) {
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
    BOOST_TEST(scenes == 31U);
}

};  // namespace
