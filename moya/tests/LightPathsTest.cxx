/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Value.h>

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Staged.h"
#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace film = v3d::moya::film;
namespace scene = v3d::moya::scene;

constexpr std::uint32_t kSide = Staged::kSide;

/** A quarter turn, in radians. **/
constexpr float kQuarterTurn = 1.57079632679489661923F;

/** The most a channel's sum strays from what it splits, as a share of it, by float rounding. **/
constexpr float kRounding = 1.0e-4F;

/** The light that splits the beauty by the first bounce's scattering, or by none. **/
const char * const kSplits[] = { "C<.D>.*", "C<.G>.*", "C<.S>.*", "C[LO]" };

/** A channel of a light path expression, of ACEScg's three components. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the channel's name, then the expression
scene::Handle expression(Staged & staged, const std::string & name, const std::string & text) {
    const scene::Handle out = staged.made.create("channel", name);
    staged.made.set(out, "sourceType", scene::Value::string("lpe"));
    staged.made.set(out, "sourceName", scene::Value::string(text));
    staged.made.set(out, "components", scene::Value::integer(3));
    return out;
}

/** An output of the channels given. **/
void written(Staged & staged, const std::vector<scene::Handle> & channels) {
    const scene::Handle output = staged.made.create("output", "picture");
    staged.made.set(output, "file", scene::Value::string("picture.exr"));
    staged.made.set(output, "channels", scene::Value::handles(channels));
}

/**
 * A room of surfaces of every lobe in front of the camera, under a rect light the camera sees and
 * a dome. Its floor and back wall are plastic, a diffuse base under a glossy coat, and in front of
 * them stand a smooth mirror and a pane of rough glass.
 **/
void room(Staged & staged) {
    const scene::Handle plastic = staged.material("plastic", { { "base_color", scene::Value::colour(glm::vec3 { 0.6F, 0.5F, 0.4F }) },
                                                                { "specular_roughness", scene::Value::number(0.3F) } });
    const scene::Handle mirror = staged.material("mirror", { { "base_color", scene::Value::colour(glm::vec3 { 1.0F }) },
                                                              { "base_metalness", scene::Value::number(1.0F) },
                                                              { "specular_roughness", scene::Value::number(0.0F) } });
    const scene::Handle glass = staged.material("glass", { { "transmission_weight", scene::Value::number(1.0F) },
                                                            { "specular_roughness", scene::Value::number(0.2F) } });
    staged.quad("floor", { { -2.0F, -1.0F, -2.0F }, { 2.0F, -1.0F, -2.0F }, { 2.0F, -1.0F, 3.0F }, { -2.0F, -1.0F, 3.0F } }, plastic);
    staged.quad("back", { { -2.0F, -1.0F, 2.0F }, { 2.0F, -1.0F, 2.0F }, { 2.0F, 2.0F, 2.0F }, { -2.0F, 2.0F, 2.0F } }, plastic);
    staged.quad("mirror", { { -0.9F, -0.8F, 1.0F }, { -0.1F, -0.8F, 0.6F }, { -0.1F, 0.2F, 0.6F }, { -0.9F, 0.2F, 1.0F } }, mirror);
    staged.quad("pane", { { 0.1F, -0.6F, 0.0F }, { 0.8F, -0.6F, 0.0F }, { 0.8F, 0.4F, 0.0F }, { 0.1F, 0.4F, 0.0F } }, glass);
    const scene::Handle panel = staged.light("rectLight", "panel", "");
    staged.made.set(panel, "camera", scene::Value::integer(1));
    staged.made.set(panel, "intensity", scene::Value::number(8.0F));
    staged.made.set(panel, "transform", scene::Value::matrix(glm::rotate(moved(0.0F, 0.9F, 0.8F), -kQuarterTurn, glm::vec3 { 1.0F, 0.0F, 0.0F })));
    staged.light("domeLight", "sky", "");
}

/** A pixel's float at an offset in the snapshot. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the pixel, then the offset
float at(const film::Snapshot & picture, std::uint32_t x, std::uint32_t y, std::uint32_t offset) {
    return picture.at(x, y, offset);
}

// light path expressions that split every path by its first bounce's scattering, and the light
// the camera sees, take channels that sum to the beauty at each pixel. Each holds some light
BOOST_AUTO_TEST_CASE(moya_lightpaths_the_first_bounces_lobes_sum_to_the_beauty_test) {
    Staged staged(8, 16);
    room(staged);
    std::vector<scene::Handle> channels;
    channels.reserve(std::size(kSplits));
    for (std::size_t i = 0; i < std::size(kSplits); ++i) {
        channels.push_back(expression(staged, "split" + std::to_string(i), kSplits[i]));
    }
    written(staged, channels);
    const film::Snapshot picture = staged.render(nullptr);
    std::vector<double> held(std::size(kSplits), 0.0);
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            for (std::uint32_t component = 0; component < 3U; ++component) {
                float sum = 0.0F;
                for (std::size_t i = 0; i < std::size(kSplits); ++i) {
                    const float part = at(picture, x, y, picture.offset("split" + std::to_string(i)) + component);
                    sum += part;
                    held[i] += part;
                }
                const float beauty = at(picture, x, y, component);
                BOOST_TEST_CONTEXT("pixel " << x << ", " << y << ", component " << component) {
                    BOOST_TEST(std::fabs(sum - beauty) <= kRounding * std::fmax(beauty, 1.0F));
                }
            }
        }
    }
    for (std::size_t i = 0; i < std::size(kSplits); ++i) {
        BOOST_TEST(held[i] > 0.0, kSplits[i]);
    }
}

/**
 * A box of grey diffuse walls, open towards the camera, under a rect light below its ceiling that
 * camera rays pass through.
 **/
void box(Staged & staged) {
    const scene::Handle grey = staged.material("grey", { { "base_color", scene::Value::colour(glm::vec3 { 0.7F }) }, { "specular_weight", scene::Value::number(0.0F) } });
    staged.quad("floor", { { -1.0F, -1.0F, -1.0F }, { 1.0F, -1.0F, -1.0F }, { 1.0F, -1.0F, 1.0F }, { -1.0F, -1.0F, 1.0F } }, grey);
    staged.quad("ceiling", { { -1.0F, 1.0F, 1.0F }, { 1.0F, 1.0F, 1.0F }, { 1.0F, 1.0F, -1.0F }, { -1.0F, 1.0F, -1.0F } }, grey);
    staged.quad("back", { { -1.0F, -1.0F, 1.0F }, { 1.0F, -1.0F, 1.0F }, { 1.0F, 1.0F, 1.0F }, { -1.0F, 1.0F, 1.0F } }, grey);
    staged.quad("left", { { -1.0F, -1.0F, 1.0F }, { -1.0F, 1.0F, 1.0F }, { -1.0F, 1.0F, -1.0F }, { -1.0F, -1.0F, -1.0F } }, grey);
    staged.quad("right", { { 1.0F, -1.0F, -1.0F }, { 1.0F, 1.0F, -1.0F }, { 1.0F, 1.0F, 1.0F }, { 1.0F, -1.0F, 1.0F } }, grey);
    const scene::Handle panel = staged.light("rectLight", "panel", "");
    staged.made.set(panel, "intensity", scene::Value::number(4.0F));
    staged.made.set(panel, "transform", scene::Value::matrix(glm::rotate(moved(0.0F, 0.95F, 0.2F), -kQuarterTurn, glm::vec3 { 1.0F, 0.0F, 0.0F })));
}

// C<RD>L in a diffuse box under one light collects the light of one diffuse bounce alone, by
// light samples and by bounces that meet the light. A render whose paths end after one bounce
// gathers just that light, from the same samples, so the two agree at each pixel
BOOST_AUTO_TEST_CASE(moya_lightpaths_one_diffuse_bounce_matches_a_render_of_one_bounce_test) {
    Staged deep(8, 16);
    box(deep);
    written(deep, { expression(deep, "direct", "C<RD>L") });
    const film::Snapshot split = deep.render(nullptr);
    Staged shallow(1, 16);
    box(shallow);
    const film::Snapshot alone = shallow.render(nullptr);
    double lit = 0.0;
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            for (std::uint32_t component = 0; component < 3U; ++component) {
                const float want = at(alone, x, y, component);
                lit += want;
                BOOST_TEST_CONTEXT("pixel " << x << ", " << y << ", component " << component) {
                    BOOST_TEST(std::fabs(at(split, x, y, split.offset("direct") + component) - want) <= kRounding * std::fmax(want, 1.0F));
                }
            }
        }
    }
    BOOST_TEST(lit > 0.0);
}

// light path expressions add channels and change nothing the paths gather, so a render with five
// gives the same beauty, bit for bit, as one with none
BOOST_AUTO_TEST_CASE(moya_lightpaths_expressions_leave_the_beauty_as_it_is_test) {
    Staged plain(8, 16);
    room(plain);
    const film::Snapshot want = plain.render(nullptr);
    Staged split(8, 16);
    room(split);
    std::vector<scene::Handle> channels;
    for (const char * text : { "C<.D>.*", "C<.G>.*", "C<.S>.*", "C[LO]", "C.*<TG>.*L" }) {
        channels.push_back(expression(split, "channel" + std::to_string(channels.size()), text));
    }
    written(split, channels);
    const film::Snapshot got = split.render(nullptr);
    BOOST_TEST_REQUIRE(got.errors.size() == want.errors.size());
    bool same = true;
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            for (std::uint32_t component = 0; component < 4U; ++component) {
                const float left = at(got, x, y, component);
                const float right = at(want, x, y, component);
                same = same && std::bit_cast<std::uint32_t>(left) == std::bit_cast<std::uint32_t>(right);
            }
        }
    }
    BOOST_TEST(same);
    BOOST_TEST(std::memcmp(got.errors.data(), want.errors.data(), want.errors.size() * sizeof(float)) == 0);
}

// a render takes the first kMostExpressions channels of light path expressions, and each one
// past them holds zero. An expression among them that cannot be read holds zero too
BOOST_AUTO_TEST_CASE(moya_lightpaths_a_channel_past_the_most_or_unread_holds_zero_test) {
    Staged staged(2, 4);
    box(staged);
    std::vector<scene::Handle> channels { expression(staged, "unread", "C<RD") };
    for (std::uint32_t i = 0; i < 32U; ++i) {
        channels.push_back(expression(staged, "every" + std::to_string(i), "C.*L"));
    }
    written(staged, channels);
    const film::Snapshot picture = staged.render(nullptr);
    const std::uint32_t x = kSide / 2U;
    const std::uint32_t y = kSide / 2U;
    BOOST_TEST(at(picture, x, y, picture.offset("every0")) > 0.0F);
    BOOST_TEST(at(picture, x, y, picture.offset("every30")) == at(picture, x, y, picture.offset("every0")));
    BOOST_TEST(at(picture, x, y, picture.offset("every31")) == 0.0F);
    BOOST_TEST(at(picture, x, y, picture.offset("unread")) == 0.0F);
}

};  // namespace
};  // namespace v3d::moya::tracing
