/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/Channel.h>
#include <moya/libmoya/output/Exr.h>
#include <moya/libmoya/output/Output.h>
#include <moya/libmoya/output/Outputs.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/vec3.hpp>

#include "Staged.h"
#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace film = v3d::moya::film;
namespace output = v3d::moya::output;
namespace scene = v3d::moya::scene;

constexpr std::uint32_t kSide = Staged::kSide;

/** A quarter turn, in radians. **/
constexpr float kQuarterTurn = 1.57079632679489661923F;

/** The samples of a noisy render, and of the converged one it is compared with. **/
constexpr std::int32_t kNoisy = 4;
constexpr std::int32_t kConverged = 1024;

/**
 * A grey floor and back wall under a rect light that camera rays pass through, and a red box
 * on the floor, rendered with samples. The options ask for the beauty denoised when denoising,
 * and a channel of every path's light is denoised beside it when channelled.
 **/
struct Lit {
    Staged staged;

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - whether denoising, then channelled
    Lit(std::int32_t samples, bool denoising, bool channelled) : staged(4, samples) {
        const scene::Handle grey = staged.material("grey", { { "base_color", scene::Value::colour(glm::vec3 { 0.6F }) }, { "specular_weight", scene::Value::number(0.0F) } });
        const scene::Handle red =
            staged.material("red", { { "base_color", scene::Value::colour(glm::vec3 { 0.7F, 0.15F, 0.1F }) }, { "specular_weight", scene::Value::number(0.0F) } });
        staged.quad("floor", { { -2.0F, -1.0F, -2.0F }, { 2.0F, -1.0F, -2.0F }, { 2.0F, -1.0F, 3.0F }, { -2.0F, -1.0F, 3.0F } }, grey);
        staged.quad("back", { { -2.0F, -1.0F, 2.0F }, { 2.0F, -1.0F, 2.0F }, { 2.0F, 2.0F, 2.0F }, { -2.0F, 2.0F, 2.0F } }, grey);
        staged.quad("front", { { -0.4F, -1.0F, 0.6F }, { 0.3F, -1.0F, 0.6F }, { 0.3F, -0.3F, 0.6F }, { -0.4F, -0.3F, 0.6F } }, red);
        const scene::Handle panel = staged.light("rectLight", "panel", "");
        staged.made.set(panel, "intensity", scene::Value::number(6.0F));
        staged.made.set(panel, "transform", scene::Value::matrix(glm::rotate(moved(0.0F, 1.2F, 0.5F), -kQuarterTurn, glm::vec3 { 1.0F, 0.0F, 0.0F })));
        staged.made.set(staged.options, "denoise", scene::Value::integer(denoising ? 1 : 0));
        std::vector<scene::Handle> channels;
        if (channelled) {
            const scene::Handle every = staged.made.create("channel", "every");
            staged.made.set(every, "sourceType", scene::Value::string("lpe"));
            staged.made.set(every, "sourceName", scene::Value::string("C.*"));
            staged.made.set(every, "denoise", scene::Value::integer(1));
            channels.push_back(every);
        }
        const scene::Handle out = staged.made.create("output", "picture");
        staged.made.set(out, "file", scene::Value::string("denoised.exr"));
        staged.made.set(out, "channels", scene::Value::handles(channels));
    }
};

/** The mean squared distance of a picture's three floats at an offset from another's beauty. **/
double distanceOf(const film::Snapshot & picture, std::uint32_t offset, const film::Snapshot & converged) {
    double sum = 0.0;
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            for (std::uint32_t i = 0; i < 3U; ++i) {
                const double gap = static_cast<double>(picture.at(x, y, offset + i)) - static_cast<double>(converged.at(x, y, i));
                sum += gap * gap;
            }
        }
    }
    return sum / (3.0 * kSide * kSide);
}

/** The offset of the denoised beauty. **/
std::uint32_t denoisedBeauty(const film::Snapshot & picture) {
    return picture.offset(output::denoisedOf(std::string(output::kBeauty)));
}

// a render of a few samples, denoised, lies nearer the converged picture than the noisy beauty
// does, in each float's mean squared distance, and is neither the noisy picture nor black
BOOST_AUTO_TEST_CASE(moya_denoise_a_denoised_render_is_nearer_the_converged_one_test) {
    Lit noisy(kNoisy, true, false);
    const film::Snapshot picture = noisy.staged.render(nullptr);
    Lit converged(kConverged, false, false);
    const film::Snapshot reference = converged.staged.render(nullptr);
    const double before = distanceOf(picture, 0, reference);
    const double after = distanceOf(picture, denoisedBeauty(picture), reference);
    BOOST_TEST_MESSAGE("noisy " << before << ", denoised " << after);
    BOOST_TEST(after < 0.5 * before);
    BOOST_TEST(distanceOf(picture, denoisedBeauty(picture), picture) > 0.0);
}

// a marked channel of a light path expression is denoised beside itself, and lies nearer the
// converged picture than its noisy self does
BOOST_AUTO_TEST_CASE(moya_denoise_a_marked_channel_is_denoised_beside_itself_test) {
    Lit noisy(kNoisy, false, true);
    const film::Snapshot picture = noisy.staged.render(nullptr);
    Lit converged(kConverged, false, false);
    const film::Snapshot reference = converged.staged.render(nullptr);
    const double before = distanceOf(picture, picture.offset("every"), reference);
    const double after = distanceOf(picture, picture.offset(output::denoisedOf("every")), reference);
    BOOST_TEST(after < 0.5 * before);
}

// denoising a render twice gives the same floats, bit for bit
BOOST_AUTO_TEST_CASE(moya_denoise_a_render_denoised_twice_gives_the_same_bytes_test) {
    Lit first(kNoisy, true, false);
    Lit second(kNoisy, true, false);
    const film::Snapshot one = first.staged.render(nullptr);
    const film::Snapshot other = second.staged.render(nullptr);
    BOOST_TEST_REQUIRE(one.values.size() == other.values.size());
    BOOST_TEST(std::memcmp(one.values.data(), other.values.data(), one.values.size() * sizeof(float)) == 0);
}

// a render that denoises adds the albedo and normal that guide it when the scene names none. An
// EXR output of it starts with the denoised beauty and follows a marked channel by its copy
BOOST_AUTO_TEST_CASE(moya_denoise_a_render_that_denoises_adds_its_guides_test) {
    Lit lit(kNoisy, true, true);
    lit.staged.made.commit();
    lit.staged.made.apply();
    const std::vector<output::Channel> channels = output::channelsOf(lit.staged.made);
    const auto named = [&](const std::string & name) {
        return std::any_of(channels.begin(), channels.end(), [&](const output::Channel & each) { return each.film.name == name; });
    };
    BOOST_TEST(named(std::string(output::kDenoiseAlbedo)));
    BOOST_TEST(named(std::string(output::kDenoiseNormal)));
    const output::Output picture = output::outputsOf(lit.staged.made).front();
    BOOST_TEST((picture.channels == std::vector<std::string> { "beauty_denoised", "every", "every_denoised" }));
    Lit plain(kNoisy, false, false);
    plain.staged.made.commit();
    plain.staged.made.apply();
    BOOST_TEST(output::channelsOf(plain.staged.made).empty());
}

};  // namespace
};  // namespace v3d::moya::tracing
