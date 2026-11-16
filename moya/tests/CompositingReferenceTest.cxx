/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <api/rib/Reader.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Compare.h>
#include <moya/libmoya/film/DeepSample.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/output/Png.h>
#include <moya/libmoya/ri/SceneClient.h>
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
#include <OpenEXR/ImfStringAttribute.h>

#include <algorithm>
#include <bit>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

namespace film = v3d::moya::film;
namespace scene = v3d::moya::scene;

/** The scenes made for compositing, each a RIB file in data/ with its render beside it. **/
constexpr const char * kScenes[] = {
    "compositing-lpe",
    "compositing-groups",
    "compositing-crypto",
    "compositing-deep",
    "compositing-denoise",
};

/** How many of the beauty's standard errors a float may lie from its committed render. **/
constexpr float kAllowed = 5.0F;

/** The share of a float, or of one, that rounding between compilers may move it by. **/
constexpr float kRounding = 1.0e-4F;

/** How far a Cryptomatte coverage may lie from its committed render, by rounding alone. **/
constexpr float kCoverage = 1.0e-3F;

/** The kinds of every light the scene can hold. **/
constexpr const char * kLightKinds[] = { "cylinderLight", "diskLight", "distantLight", "domeLight", "meshLight", "rectLight", "sphereLight" };

/** The floats of a deep pixel flattened, RGBA, and the count of its samples. **/
constexpr std::size_t kDeepFloats = 5;

/**
 * What a render is compared by: each pixel's floats, the beauty's standard errors, each pixel's
 * deep samples flattened with their count, and each Cryptomatte layer's manifest.
 **/
struct Picture {
    film::Snapshot snapshot;
    std::vector<float> deep;
    std::map<std::string, std::string> manifests;
};

/** A pixel's deep samples composited front to back: red, green, blue, alpha, then their count. **/
void flattened(const std::vector<film::DeepSample> & samples, float * out) {
    float clear = 1.0F;
    for (std::size_t i = 0; i < 4U; ++i) {
        out[i] = 0.0F;
    }
    for (const film::DeepSample & sample : samples) {
        out[0] += clear * sample.red;
        out[1] += clear * sample.green;
        out[2] += clear * sample.blue;
        out[3] += clear * sample.alpha;
        clear *= 1.0F - sample.alpha;
    }
    out[4] = static_cast<float>(samples.size());
}

/**
 * A RIB file of data/ read through the scene client and rendered through the integrator. A
 * change, when it is given, is made to the scene before it renders.
 **/
Picture rendered(const std::string & name, const std::function<void(scene::Scene &)> & change = {}) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    scene::Scene made(scene::kinds());
    v3d::moya::ri::SceneClient client(made, logger);
    v3d::moya::device::CpuDevice device;
    Picture out;
    client.frame([&]() {
        made.apply();
        if (change) {
            change(made);
            made.commit();
            made.apply();
        }
        out.snapshot = v3d::moya::integrate::render(device, made, logger, v3d::moya::colour::Config());
    });
    v3d::rib::Reader reader(logger);
    BOOST_TEST_REQUIRE(reader.read("data/" + name + ".rib", &client), reader.error());
    out.manifests = out.snapshot.manifests;
    if (!out.snapshot.deep.empty()) {
        out.deep.assign(out.snapshot.deep.size() * kDeepFloats, 0.0F);
        for (std::size_t pixel = 0; pixel < out.snapshot.deep.size(); ++pixel) {
            flattened(out.snapshot.deep[pixel], &out.deep[pixel * kDeepFloats]);
        }
    }
    return out;
}

/** The name in a committed render of a float of each pixel. **/
std::string slotName(std::size_t slot) {
    return "v" + std::to_string(slot);
}

/** Writes a render as its committed EXR. **/
void writeReference(const std::string & path, const Picture & picture) {
    const film::Snapshot & snapshot = picture.snapshot;
    const std::size_t width = snapshot.resolution.x;
    Imf::Header header(static_cast<int>(snapshot.resolution.x), static_cast<int>(snapshot.resolution.y));
    header.compression() = Imf::ZIP_COMPRESSION;
    Imf::FrameBuffer frame;
    // OpenEXR reads through a pointer that is not const, and writing a file reads nothing back
    char * const values = const_cast<char *>(reinterpret_cast<const char *>(snapshot.values.data()));
    for (std::size_t slot = 0; slot < snapshot.stride; ++slot) {
        header.channels().insert(slotName(slot), Imf::Channel(Imf::FLOAT));
        frame.insert(slotName(slot), Imf::Slice(Imf::FLOAT, values + (slot * sizeof(float)), sizeof(float) * snapshot.stride, sizeof(float) * snapshot.stride * width));
    }
    header.channels().insert("error", Imf::Channel(Imf::FLOAT));
    frame.insert("error", Imf::Slice(Imf::FLOAT, const_cast<char *>(reinterpret_cast<const char *>(snapshot.errors.data())), sizeof(float), sizeof(float) * width));
    char * const deep = const_cast<char *>(reinterpret_cast<const char *>(picture.deep.data()));
    for (std::size_t i = 0; !picture.deep.empty() && i < kDeepFloats; ++i) {
        header.channels().insert("deep" + std::to_string(i), Imf::Channel(Imf::FLOAT));
        frame.insert("deep" + std::to_string(i), Imf::Slice(Imf::FLOAT, deep + (i * sizeof(float)), sizeof(float) * kDeepFloats, sizeof(float) * kDeepFloats * width));
    }
    for (const auto & [layer, manifest] : picture.manifests) {
        header.insert("manifest/" + layer, Imf::StringAttribute(manifest));
    }
    Imf::OutputFile file(path.c_str(), header);
    file.setFrameBuffer(frame);
    file.writePixels(static_cast<int>(snapshot.resolution.y));
}

/** A committed render read back, its floats laid out as the render it is compared with. **/
Picture readReference(const std::string & path, const Picture & like) {
    Imf::InputFile file(path.c_str());
    const Imath::Box2i & window = file.header().dataWindow();
    Picture out;
    out.snapshot = like.snapshot;
    film::Snapshot & snapshot = out.snapshot;
    const int across = window.max.x - window.min.x + 1;
    const int down = window.max.y - window.min.y + 1;
    const auto width = static_cast<std::size_t>(across);
    const std::size_t pixels = width * static_cast<std::size_t>(down);
    snapshot.values.assign(pixels * snapshot.stride, -1.0F);
    snapshot.errors.assign(pixels, 0.0F);
    out.deep.assign(like.deep.size(), -1.0F);
    Imf::FrameBuffer frame;
    char * const values = reinterpret_cast<char *>(snapshot.values.data());
    for (std::size_t slot = 0; slot < snapshot.stride; ++slot) {
        frame.insert(slotName(slot), Imf::Slice(Imf::FLOAT, values + (slot * sizeof(float)), sizeof(float) * snapshot.stride, sizeof(float) * snapshot.stride * width));
    }
    frame.insert("error", Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(snapshot.errors.data()), sizeof(float), sizeof(float) * width));
    for (std::size_t i = 0; !out.deep.empty() && i < kDeepFloats; ++i) {
        frame.insert("deep" + std::to_string(i), Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(out.deep.data()) + (i * sizeof(float)), sizeof(float) * kDeepFloats,
                                                     sizeof(float) * kDeepFloats * width));
    }
    file.setFrameBuffer(frame);
    file.readPixels(window.min.y, window.max.y);
    for (const auto & [layer, manifest] : like.manifests) {
        const auto * attribute = file.header().findTypedAttribute<Imf::StringAttribute>("manifest/" + layer);
        out.manifests[layer] = attribute == nullptr ? std::string() : attribute->value();
    }
    return out;
}

/** Whether a film channel is a Cryptomatte layer's ranks: the layer's name and two digits. **/
bool ranked(const std::string & channel, const std::map<std::string, std::string> & manifests) {
    return std::ranges::any_of(manifests, [&](const auto & held) {
        const std::string & layer = held.first;
        return channel.size() == layer.size() + 2U && channel.starts_with(layer) && std::isdigit(static_cast<unsigned char>(channel[layer.size()])) != 0 &&
            std::isdigit(static_cast<unsigned char>(channel[layer.size() + 1U])) != 0;
    });
}

/** Whether a float lies within kAllowed beauty errors, and rounding, of its committed one. **/
bool near(float got, float want, float error) {
    return std::fabs(got - want) <= (kAllowed * error) + (kRounding * std::fmax(1.0F, std::fabs(want)));
}

/** Whether a Cryptomatte rank's float matches its committed one: an ID exactly, coverage near. **/
bool sameRank(float got, float want, bool id) {
    if (id) {
        return std::bit_cast<std::uint32_t>(got) == std::bit_cast<std::uint32_t>(want);
    }
    return std::fabs(got - want) <= kCoverage;
}

/** Where a render's film channels first differ from their committed ones, or empty. **/
std::string channelsDiffer(const Picture & got, const Picture & want) {
    const film::Snapshot & snapshot = got.snapshot;
    const std::size_t pixels = snapshot.errors.size();
    for (std::size_t index = 0; index < snapshot.channels.size(); ++index) {
        const film::Channel & channel = snapshot.channels[index];
        const bool ids = ranked(channel.name, got.manifests);
        for (std::size_t pixel = 0; pixel < pixels; ++pixel) {
            for (std::uint32_t i = 0; i < channel.components; ++i) {
                const std::size_t at = (pixel * snapshot.stride) + snapshot.offsets[index] + i;
                const float left = snapshot.values[at];
                const float right = want.snapshot.values[at];
                const bool same = ids ? sameRank(left, right, i % 2U == 0U) : near(left, right, want.snapshot.errors[pixel]);
                if (!same) {
                    return channel.name + " at pixel " + std::to_string(pixel);
                }
            }
        }
    }
    return {};
}

/** Where a render's deep samples first differ from their committed ones, or empty. **/
std::string deepDiffers(const Picture & got, const Picture & want) {
    for (std::size_t pixel = 0; pixel * kDeepFloats < got.deep.size(); ++pixel) {
        for (std::size_t i = 0; i < kDeepFloats; ++i) {
            const float left = got.deep[(pixel * kDeepFloats) + i];
            const float right = want.deep[(pixel * kDeepFloats) + i];
            const bool same = i + 1U == kDeepFloats ? left == right : near(left, right, want.snapshot.errors[pixel]);
            if (!same) {
                return "the deep samples at pixel " + std::to_string(pixel);
            }
        }
    }
    return {};
}

/** Where a render first differs from its committed render, or empty where it does not. **/
std::string differs(const Picture & got, const Picture & want) {
    const film::Furthest furthest = film::furthest(got.snapshot, want.snapshot);
    if (!(furthest.sigmas < kAllowed) || furthest.unmeasured != 0U) {
        return "the beauty at pixel " + std::to_string(furthest.pixel.x) + ", " + std::to_string(furthest.pixel.y);
    }
    if (got.manifests != want.manifests) {
        return "a manifest";
    }
    const std::string channels = channelsDiffer(got, want);
    return channels.empty() ? deepDiffers(got, want) : channels;
}

/** Writes a render to data_out/, as an EXR to commit and a PNG of its beauty to look at. **/
void keep(const std::string & name, const Picture & picture) {
    boost::filesystem::create_directories("data_out");
    writeReference("data_out/" + name + ".exr", picture);
    const v3d::moya::colour::Config config;
    const std::string display = config.defaultDisplay();
    v3d::moya::output::writePng("data_out/" + name + ".png", picture.snapshot, config.display(display, config.defaultView(display)),
        boost::make_shared<v3d::log::Logger>());
}

/**
 * An RGB build carries no wavelengths, so its pictures are not the committed ones, which are
 * rendered by a spectral build.
 **/
#ifndef MOYA_RGB

/**
 * Each scene renders as its committed render. Its beauty and every channel lie within kAllowed of
 * the beauty's errors, and each Cryptomatte ID and manifest is exact. Each pixel's deep samples
 * match by their count exactly, and by their flattened picture within the errors. A scene with no
 * committed render yet, or one that differs, leaves what it rendered in data_out/.
 *
 * Passes before the change: the change adds the scenes and their renders, of what the renderer
 * already does, to hold it there.
 **/
BOOST_AUTO_TEST_CASE(moya_compositing_references_every_scene_renders_as_committed_test) {
    for (const char * name : kScenes) {
        BOOST_TEST_CONTEXT(name) {
            const Picture picture = rendered(name);
            const std::string reference = "data/" + std::string(name) + ".exr";
            if (!boost::filesystem::exists(reference)) {
                keep(name, picture);
                BOOST_TEST(false, "no committed render; data_out/ holds this one");
                continue;
            }
            const std::string where = differs(picture, readReference(reference, picture));
            if (!where.empty()) {
                keep(name, picture);
            }
            BOOST_TEST(where.empty(), where);
        }
    }
}

/**
 * Every light twice as bright fails the comparison in every scene, so the comparison sees a change
 * of light and not only a change of picture.
 *
 * Passes before the change: it checks the comparison the change adds against renders of what the
 * renderer already does.
 **/
BOOST_AUTO_TEST_CASE(moya_compositing_references_a_brighter_light_fails_the_comparison_test) {
    for (const char * name : kScenes) {
        BOOST_TEST_CONTEXT(name) {
            const std::string reference = "data/" + std::string(name) + ".exr";
            BOOST_TEST_REQUIRE(boost::filesystem::exists(reference));
            const Picture brighter = rendered(name, [](scene::Scene & made) {
                for (const char * kind : kLightKinds) {
                    for (const scene::Handle & light : made.objects(kind)) {
                        made.set(light, "exposure", scene::Value::number(made.value(light, "exposure").floatArray().front() + 1.0F));
                    }
                }
            });
            BOOST_TEST(!differs(brighter, readReference(reference, brighter)).empty());
        }
    }
}

#endif

};  // namespace
