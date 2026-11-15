/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/DeepSample.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/output/Cryptomatte.h>
#include <moya/libmoya/output/Outputs.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <OpenEXR/ImfCompositeDeepScanLine.h>
#include <OpenEXR/ImfDeepScanLineInputFile.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfIDManifest.h>
#include <OpenEXR/ImfPixelType.h>
#include <OpenEXR/ImfStandardAttributes.h>

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

#include "Staged.h"

namespace v3d::moya::tracing {
namespace {

namespace film = v3d::moya::film;
namespace integrate = v3d::moya::integrate;
namespace output = v3d::moya::output;
namespace scene = v3d::moya::scene;

constexpr std::uint32_t kSide = Staged::kSide;

/** A pixel behind the card and the wall, a pixel of the wall alone, and a corner of neither. **/
constexpr std::uint32_t kCarded = 4;
constexpr std::uint32_t kWalled = 12;
constexpr std::uint32_t kRow = 8;

/** The depths of the card and of the wall from the camera. **/
constexpr float kCardDepth = 5.0F;
constexpr float kWallDepth = 6.0F;

/** How far a flattened float may lie from the flat picture's, as a share of it or of one. **/
constexpr float kRounding = 1.0e-4F;

/** The file the cases write. **/
constexpr const char * kFile = "deep.exr";

/**
 * A grey wall six units from the camera, and in front of its left half a red card a quarter
 * opaque, under a dome when lit. The output writes a deep file of floats.
 **/
struct Carded {
    Staged staged { 2, 64 };

    explicit Carded(bool lit) {
        const scene::Handle grey = staged.material("grey", { { "base_color", scene::Value::colour(glm::vec3 { 0.5F }) } });
        const scene::Handle red = staged.material("red", { { "base_color", scene::Value::colour(glm::vec3 { 0.8F, 0.1F, 0.1F }) },
                                                          { "geometry_opacity", scene::Value::number(0.25F) } });
        staged.quad("wall", { { -0.75F, -0.75F, 1.0F }, { 0.75F, -0.75F, 1.0F }, { 0.75F, 0.75F, 1.0F }, { -0.75F, 0.75F, 1.0F } }, grey);
        staged.quad("card", { { -0.75F, -0.75F, 0.0F }, { 0.0F, -0.75F, 0.0F }, { 0.0F, 0.75F, 0.0F }, { -0.75F, 0.75F, 0.0F } }, red);
        if (lit) {
            staged.light("domeLight", "sky", "");
        }
        const scene::Handle out = staged.made.create("output", "picture");
        staged.made.set(out, "file", scene::Value::string(kFile));
        staged.made.set(out, "format", scene::Value::string("deepexr"));
        staged.made.set(out, "precision", scene::Value::string("float"));
    }

    film::Snapshot render(const integrate::Checkpoints & checkpoints = {}) {
        staged.made.commit();
        staged.made.apply();
        device::CpuDevice device;
        return integrate::render(device, staged.made, {}, colour::Config(), checkpoints);
    }
};

/** A pixel's deep samples. **/
const std::vector<film::DeepSample> & samplesAt(const film::Snapshot & picture, std::uint32_t x, std::uint32_t y) {
    return picture.deep.at((static_cast<std::size_t>(y) * kSide) + x);
}

// a pixel behind the card holds the card's sample in front of the wall's, each with its own ID,
// the card's alpha near the quarter of samples it stops and the wall's covering the rest. A pixel
// of the wall alone holds the wall whole
BOOST_AUTO_TEST_CASE(moya_deep_a_pixel_behind_a_card_holds_the_card_and_the_wall_test) {
    Carded carded(true);
    const film::Snapshot picture = carded.render();
    const std::vector<film::DeepSample> & behind = samplesAt(picture, kCarded, kRow);
    BOOST_TEST_REQUIRE(behind.size() == 2U);
    BOOST_TEST(behind[0].depth == kCardDepth, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(behind[0].object == output::idOf("card"));
    BOOST_TEST(behind[0].material == output::idOf("red"));
    BOOST_TEST(std::fabs(behind[0].alpha - 0.25F) < 0.1F);
    BOOST_TEST(behind[1].depth == kWallDepth, boost::test_tools::tolerance(1.0e-5F));
    BOOST_TEST(behind[1].object == output::idOf("wall"));
    BOOST_TEST(behind[1].alpha == 1.0F, boost::test_tools::tolerance(1.0e-6F));
    const std::vector<film::DeepSample> & wall = samplesAt(picture, kWalled, kRow);
    BOOST_TEST_REQUIRE(wall.size() == 1U);
    BOOST_TEST(wall[0].alpha == 1.0F, boost::test_tools::tolerance(1.0e-6F));
}

// a pixel that meets nothing holds one sample of the dome's light behind everything, of no
// alpha. Under no dome it holds none
BOOST_AUTO_TEST_CASE(moya_deep_a_pixel_that_meets_nothing_holds_only_what_it_sees_test) {
    Carded lit(true);
    const std::vector<film::DeepSample> sky = samplesAt(lit.render(), 0, 0);
    BOOST_TEST_REQUIRE(sky.size() == 1U);
    BOOST_TEST(std::isinf(sky[0].depth));
    BOOST_TEST(sky[0].alpha == 0.0F);
    BOOST_TEST(sky[0].red > 0.0F);
    BOOST_TEST(sky[0].object == 0U);
    Carded dark(false);
    BOOST_TEST(samplesAt(dark.render(), 0, 0).empty());
}

// flattening the deep file through OpenEXR's own compositing gives the flat beauty and alpha at
// each pixel, to float rounding
BOOST_AUTO_TEST_CASE(moya_deep_flattening_the_file_gives_the_flat_picture_test) {
    Carded carded(true);
    const film::Snapshot picture = carded.render();
    output::write(output::outputsOf(carded.staged.made).front(), picture, colour::Config(), {});
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

// the file's ID manifest names each object's and material's ID by MurmurHash3
BOOST_AUTO_TEST_CASE(moya_deep_the_file_names_its_ids_test) {
    Carded carded(true);
    const film::Snapshot picture = carded.render();
    output::write(output::outputsOf(carded.staged.made).front(), picture, colour::Config(), {});
    const Imf::DeepScanLineInputFile file(kFile);
    BOOST_TEST_REQUIRE(Imf::hasIDManifest(file.header()));
    const Imf::IDManifest manifest(Imf::idManifest(file.header()));
    BOOST_TEST_REQUIRE(manifest.size() == 2U);
    BOOST_TEST(manifest[0].getHashScheme() == Imf::IDManifest::MURMURHASH3_32);
    BOOST_TEST(manifest.find("objectid") == 0U);
    BOOST_TEST(manifest.find("materialid") == 1U);
    BOOST_TEST((manifest[0].find(output::idOf("wall")) != manifest[0].end()));
    BOOST_TEST((manifest[0].find(output::idOf("card")) != manifest[0].end()));
    BOOST_TEST((manifest[1].find(output::idOf("red")) != manifest[1].end()));
    std::remove(kFile);
}

// a checkpoint holds each pixel's deep list exactly, so a render stopped after its first pass and
// resumed gives the same samples bit for bit as a render that never stopped
BOOST_AUTO_TEST_CASE(moya_deep_a_resumed_render_keeps_its_lists_test) {
    const film::Snapshot whole = Carded(true).render();
    const std::string checkpoint = "deep-checkpoint.exr";
    Carded stopping(true);
    BOOST_TEST(stopping.render(integrate::Checkpoints { .path = checkpoint, .stop = [](std::uint32_t passes) { return passes == 1U; } }).passes == 1U);
    Carded resuming(true);
    const film::Snapshot resumed = resuming.render(integrate::Checkpoints { .path = checkpoint, .resume = true });
    std::remove(checkpoint.c_str());
    BOOST_TEST_REQUIRE(resumed.deep.size() == whole.deep.size());
    bool same = true;
    for (std::size_t pixel = 0; pixel < whole.deep.size(); ++pixel) {
        same = same && resumed.deep[pixel].size() == whole.deep[pixel].size();
        for (std::size_t i = 0; same && i < whole.deep[pixel].size(); ++i) {
            const film::DeepSample & left = resumed.deep[pixel][i];
            const film::DeepSample & right = whole.deep[pixel][i];
            same = std::bit_cast<std::uint32_t>(left.alpha) == std::bit_cast<std::uint32_t>(right.alpha) &&
                std::bit_cast<std::uint32_t>(left.red) == std::bit_cast<std::uint32_t>(right.red) && left.depth == right.depth &&
                left.object == right.object;
        }
    }
    BOOST_TEST(same);
    BOOST_TEST(samplesAt(resumed, kCarded, kRow).size() == 2U);
}

};  // namespace
};  // namespace v3d::moya::tracing
