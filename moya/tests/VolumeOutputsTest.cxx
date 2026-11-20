/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/film/DeepSample.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/Cryptomatte.h>
#include <moya/libmoya/output/Outputs.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <OpenEXR/ImfCompositeDeepScanLine.h>
#include <OpenEXR/ImfDeepScanLineInputFile.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfPixelType.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include "Staged.h"
#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace film = v3d::moya::film;
namespace output = v3d::moya::output;
namespace scene = v3d::moya::scene;

constexpr std::uint32_t kSide = Staged::kSide;

/** The fog's radius and centre: a sphere whose chord is all but the same across the picture. **/
constexpr float kRadius = 30.0F;
constexpr float kCentre = 40.0F;

/** The camera's place along z, and so the depth of the fog's front along the camera's view. **/
constexpr float kCamera = -5.0F;
constexpr float kFront = kCentre - kRadius - kCamera;

/** The file the deep case writes. **/
constexpr const char * kFile = "volumeoutputs.exr";

/** How far a flattened float may lie from the flat picture's, as a share of it or of one. **/
constexpr float kRounding = 1.0e-4F;

/**
 * A grey fog of an absorption and a scattering, a sphere ahead of the camera, which an
 * orthographic camera sees through along its whole width. Its chord runs from twice its radius at
 * the picture's middle to a part in two thousand less at a corner. A channel of each name and
 * source asked for collects its values.
 **/
struct Fogged {
    Staged staged;
    std::vector<scene::Handle> channels;

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the absorption, then the scattering
    Fogged(float absorption, float scattering, std::int32_t samples) : staged(4, samples) {
        staged.fog("fog", kRadius, glm::vec3 { 0.0F, 0.0F, kCentre }, glm::vec3 { absorption }, glm::vec3 { scattering }, 0.0F);
    }

    /** A raw channel of a source, of components, or a Cryptomatte layer of ranks. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the name, then the source
    void channel(const std::string & name, const std::string & source, std::int32_t components, std::int32_t ranks = 0) {
        const scene::Handle out = staged.made.create("channel", name);
        staged.made.set(out, "sourceType", scene::Value::string("raw"));
        staged.made.set(out, "sourceName", scene::Value::string(source));
        if (ranks > 0) {
            staged.made.set(out, "ranks", scene::Value::integer(ranks));
        } else {
            staged.made.set(out, "components", scene::Value::integer(components));
        }
        channels.push_back(out);
    }

    /** The picture of one output that holds every channel, of the format given or the default. **/
    film::Snapshot render(const std::string & format = {}) {
        const scene::Handle out = staged.made.create("output", "picture");
        staged.made.set(out, "file", scene::Value::string(kFile));
        if (!format.empty()) {
            staged.made.set(out, "format", scene::Value::string(format));
        }
        staged.made.set(out, "precision", scene::Value::string("float"));
        staged.made.set(out, "channels", scene::Value::handles(channels));
        return staged.render(nullptr);
    }
};

/** The transmittance of the fog's chord at a place on the screen, from -1 to 1 on each axis. **/
float transmittanceAt(float extinction, float x, float y) {
    const float chord = 2.0F * std::sqrt((kRadius * kRadius) - (x * x) - (y * y));
    return std::exp(-extinction * chord);
}

/** A pixel's float in a channel. **/
float at(const film::Snapshot & picture, const std::string & name, std::uint32_t x, std::uint32_t y, std::uint32_t component = 0) {
    return picture.at(x, y, picture.offset(name) + component);
}

// a grey fog over nothing has an alpha of one less its transmittance along each camera ray, so it
// composites over a plate. Its picture's mean alpha is the mean of one less the closed form's
BOOST_AUTO_TEST_CASE(moya_volume_outputs_a_fogs_alpha_is_one_less_its_transmittance_test) {
    constexpr float kAbsorption = 0.01F;
    constexpr float kScattering = 0.005F;
    Fogged fogged(kAbsorption, kScattering, 256);
    const film::Snapshot picture = fogged.render();
    double got = 0.0;
    double want = 0.0;
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            const float across = -1.0F + ((2.0F * (static_cast<float>(x) + 0.5F)) / kSide);
            const float down = -1.0F + ((2.0F * (static_cast<float>(y) + 0.5F)) / kSide);
            got += static_cast<double>(picture.at(x, y, 3U));
            want += static_cast<double>(1.0F - transmittanceAt(kAbsorption + kScattering, across, down));
        }
    }
    const double pixels = static_cast<double>(kSide) * kSide;
    BOOST_TEST(got / pixels == want / pixels, boost::test_tools::tolerance(0.01));
}

// a camera ray's collision in the fog is its first hit. Its depth lies within the fog, as deep as
// the nearest collision of the pixel's samples. Its position is inside the sphere, its instance
// the fog's, and its normal none
BOOST_AUTO_TEST_CASE(moya_volume_outputs_a_collision_is_the_first_hit_test) {
    Fogged fogged(0.05F, 0.05F, 64);
    fogged.channel("depth", "depth", 1);
    fogged.channel("position", "position", 3);
    fogged.channel("instance", "instance", 1);
    fogged.channel("normal", "normal", 3);
    const film::Snapshot picture = fogged.render();
    const auto fog = static_cast<float>(fogged.staged.made.objects("instance").front().index);
    for (std::uint32_t y = 0; y < kSide; y += 5U) {
        for (std::uint32_t x = 0; x < kSide; x += 5U) {
            BOOST_TEST_CONTEXT("pixel " << x << ", " << y) {
                const float depth = at(picture, "depth", x, y);
                BOOST_TEST(depth >= kFront - 1.0e-3F);
                BOOST_TEST(depth < kFront + 2.0F);
                const glm::vec3 position { at(picture, "position", x, y, 0), at(picture, "position", x, y, 1), at(picture, "position", x, y, 2) };
                BOOST_TEST(glm::length(position - glm::vec3 { 0.0F, 0.0F, kCentre }) <= kRadius + 1.0e-3F);
                BOOST_TEST(at(picture, "instance", x, y) == fog);
                for (std::uint32_t component = 0; component < 3U; ++component) {
                    BOOST_TEST(at(picture, "normal", x, y, component) == 0.0F);
                }
            }
        }
    }
}

// a denoiser reads a thick fog's single-scattering albedo, its scattering over its extinction, and
// a normal back along the camera ray. Each sample's albedo varies by its wavelengths, so the
// picture's mean is held to it
BOOST_AUTO_TEST_CASE(moya_volume_outputs_a_denoiser_reads_the_fogs_albedo_test) {
    Fogged fogged(0.4F, 1.6F, 64);
    fogged.channel("albedo", "albedo", 3);
    fogged.channel("denoisingNormal", "denoisingNormal", 3);
    const film::Snapshot picture = fogged.render();
    double albedo[3] {};
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            for (std::uint32_t component = 0; component < 3U; ++component) {
                albedo[component] += static_cast<double>(at(picture, "albedo", x, y, component));
            }
            BOOST_TEST(at(picture, "denoisingNormal", x, y, 2) == -1.0F, boost::test_tools::tolerance(1.0e-4F));
        }
    }
    const double pixels = static_cast<double>(kSide) * kSide;
    for (const double sum : albedo) {
        BOOST_TEST(sum / pixels == 0.8, boost::test_tools::tolerance(0.01));
    }
}

// Cryptomatte names the fog by its instance, at a coverage that is the pixel's alpha
BOOST_AUTO_TEST_CASE(moya_volume_outputs_cryptomatte_covers_a_fog_by_its_alpha_test) {
    Fogged fogged(0.01F, 0.005F, 64);
    fogged.channel("CryptoObject", "crypto_object", 0, 2);
    const film::Snapshot picture = fogged.render();
    const float fog = output::floatOf(output::idOf("fog"));
    const std::uint32_t offset = picture.offset("CryptoObject00");
    for (std::uint32_t y = 1; y < kSide; y += 4U) {
        for (std::uint32_t x = 1; x < kSide; x += 4U) {
            BOOST_TEST_CONTEXT("pixel " << x << ", " << y) {
                BOOST_TEST(picture.at(x, y, offset) == fog);
                BOOST_TEST(picture.at(x, y, offset + 1U) == picture.at(x, y, 3U), boost::test_tools::tolerance(1.0e-4F));
            }
        }
    }
}

// a deep file of a lit fog in front of a wall holds stretches of the fog, each from a front depth
// to a back one. It flattens through OpenEXR's own compositing to the flat beauty and alpha
BOOST_AUTO_TEST_CASE(moya_volume_outputs_a_deep_fog_flattens_to_the_flat_picture_test) {
    Fogged fogged(0.01F, 0.02F, 64);
    const scene::Handle grey = fogged.staged.material("grey", { { "base_color", scene::Value::colour(glm::vec3 { 0.5F }) } });
    constexpr float kWall = kCentre + kRadius + 5.0F;
    fogged.staged.quad("wall", { { -2.0F, -2.0F, kWall }, { 2.0F, -2.0F, kWall }, { 2.0F, 2.0F, kWall }, { -2.0F, 2.0F, kWall } }, grey);
    fogged.staged.light("domeLight", "sky", "");
    const film::Snapshot picture = fogged.render("deepexr");
    std::size_t stretches = 0;
    for (const std::vector<film::DeepSample> & pixel : picture.deep) {
        for (const film::DeepSample & sample : pixel) {
            BOOST_TEST(sample.back >= sample.depth);
            stretches += sample.back > sample.depth ? 1U : 0U;
        }
    }
    BOOST_TEST(stretches > static_cast<std::size_t>(kSide) * kSide);
    output::write(output::outputsOf(fogged.staged.made).front(), picture, colour::Config(), {});
    Imf::DeepScanLineInputFile file(kFile);
    Imf::CompositeDeepScanLine composite;
    composite.addSource(&file);
    const char * const names[] = { "R", "G", "B", "A" };
    std::vector<float> flat[4];
    Imf::FrameBuffer frame;
    for (std::size_t i = 0; i < 4U; ++i) {
        flat[i].assign(static_cast<std::size_t>(kSide) * kSide, -1.0F);
        frame.insert(names[i], Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(flat[i].data()), sizeof(float), sizeof(float) * kSide));
    }
    composite.setFrameBuffer(frame);
    composite.readPixels(0, static_cast<int>(kSide) - 1);
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            for (std::uint32_t i = 0; i < 4U; ++i) {
                const float want = picture.at(x, y, i);
                const float got = flat[i][(static_cast<std::size_t>(y) * kSide) + x];
                BOOST_TEST_CONTEXT("pixel " << x << ", " << y << ", " << names[i]) {
                    BOOST_TEST(std::fabs(got - want) <= kRounding * std::fmax(std::fabs(want), 1.0F));
                }
            }
        }
    }
    std::remove(kFile);
}

};  // namespace

};  // namespace v3d::moya::tracing
