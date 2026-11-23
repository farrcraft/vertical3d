/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/film/Compare.h>
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
#include <glm/vec2.hpp>
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

/** The light of each group, of the lights no group names, and of the emitting square. **/
const char * const kGroups[] = { "C.*<L.'key'>", "C.*<L.'fill'>", "C.*<L.[^'key''fill']>", "C.*O" };

/** The key light's intensity, which the brighter render doubles. **/
constexpr float kKey = 6.0F;

/** How many standard errors a picture may lie from another and still match it. **/
constexpr float kMatched = 5.0F;

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

/**
 * A grey floor and back wall under two rect lights, a dome in no group, and a square whose
 * material emits. The key light, on the left, is in the group key and as bright as it is given.
 * The fill light, on the right, is in the group fill.
 **/
void grouped(Staged & staged, float key) {
    const scene::Handle grey = staged.material("grey", { { "base_color", scene::Value::colour(glm::vec3 { 0.6F }) }, { "specular_roughness", scene::Value::number(0.4F) } });
    const scene::Handle glowing = staged.material("glowing", { { "emission_luminance", scene::Value::number(2.0F) } });
    staged.quad("floor", { { -2.0F, -1.0F, -2.0F }, { 2.0F, -1.0F, -2.0F }, { 2.0F, -1.0F, 3.0F }, { -2.0F, -1.0F, 3.0F } }, grey);
    staged.quad("back", { { -2.0F, -1.0F, 2.0F }, { 2.0F, -1.0F, 2.0F }, { 2.0F, 2.0F, 2.0F }, { -2.0F, 2.0F, 2.0F } }, grey);
    staged.quad("glow", { { -0.2F, -0.9F, 1.5F }, { 0.2F, -0.9F, 1.5F }, { 0.2F, -0.5F, 1.5F }, { -0.2F, -0.5F, 1.5F } }, glowing);
    const scene::Handle keyLight = staged.light("rectLight", "key", "key");
    staged.made.set(keyLight, "intensity", scene::Value::number(key));
    staged.made.set(keyLight, "transform", scene::Value::matrix(glm::rotate(moved(-0.8F, 0.9F, 0.8F), -kQuarterTurn, glm::vec3 { 1.0F, 0.0F, 0.0F })));
    const scene::Handle fillLight = staged.light("rectLight", "fill", "fill");
    staged.made.set(fillLight, "intensity", scene::Value::number(3.0F));
    staged.made.set(fillLight, "transform", scene::Value::matrix(glm::rotate(moved(0.8F, 0.9F, 0.8F), -kQuarterTurn, glm::vec3 { 1.0F, 0.0F, 0.0F })));
    staged.light("domeLight", "sky", "");
}

/**
 * Renders a scene that build() fills with a channel for each split, and checks that the channels
 * sum to the beauty at each pixel and that each holds some light.
 **/
template <typename F>
void sumsToTheBeauty(const std::vector<std::string> & splits, F && build) {
    Staged staged(8, 16);
    build(staged);
    std::vector<scene::Handle> channels;
    channels.reserve(splits.size());
    for (std::size_t i = 0; i < splits.size(); ++i) {
        channels.push_back(expression(staged, "split" + std::to_string(i), splits[i]));
    }
    written(staged, channels);
    const film::Snapshot picture = staged.render(nullptr);
    std::vector<double> held(splits.size(), 0.0);
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            for (std::uint32_t component = 0; component < 3U; ++component) {
                float sum = 0.0F;
                for (std::size_t i = 0; i < splits.size(); ++i) {
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
    for (std::size_t i = 0; i < splits.size(); ++i) {
        BOOST_TEST(held[i] > 0.0, splits[i]);
    }
}

// light path expressions that split every path by its first bounce's scattering, and the light
// the camera sees, take channels that sum to the beauty at each pixel. Each holds some light
BOOST_AUTO_TEST_CASE(moya_lightpaths_the_first_bounces_lobes_sum_to_the_beauty_test) {
    sumsToTheBeauty({ std::begin(kSplits), std::end(kSplits) }, room);
}

// a fog in front of the room adds paths whose first event is in a medium. That event has no
// scattering, so the splits by the first bounce's scattering leave it out. One more split, of the
// paths that start with an event in a medium, makes the channels sum to the beauty again
BOOST_AUTO_TEST_CASE(moya_lightpaths_a_fogs_events_and_the_first_bounces_lobes_sum_to_the_beauty_test) {
    std::vector<std::string> splits { std::begin(kSplits), std::end(kSplits) };
    splits.emplace_back("C<V.>.*");
    sumsToTheBeauty(splits, [](Staged & staged) {
        room(staged);
        staged.fog("fog", 0.5F, glm::vec3 { -0.4F, 0.3F, -1.0F }, glm::vec3 { 0.1F }, glm::vec3 { 0.6F, 1.2F, 2.0F }, 0.3F);
    });
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

/**
 * A fog sphere of a medium that scatters by colour, under a sun of no angle that shines from
 * behind the camera, with nothing else in the scene.
 **/
void fogged(Staged & staged, std::int32_t volumeDepth) {
    staged.made.set(staged.options, "volumeDepth", scene::Value::integer(volumeDepth));
    staged.fog("fog", 1.0F, glm::vec3 { 0.0F }, glm::vec3 { 0.1F, 0.2F, 0.1F }, glm::vec3 { 0.5F, 1.0F, 2.0F }, 0.3F);
    const scene::Handle sun = staged.light("distantLight", "sun", "");
    staged.made.set(sun, "angle", scene::Value::number(0.0F));
    staged.made.set(sun, "intensity", scene::Value::number(3.0F));
    staged.made.set(sun, "transform", scene::Value::matrix(glm::rotate(glm::mat4(1.0F), 2.0F * kQuarterTurn, glm::vec3 { 1.0F, 0.0F, 0.0F })));
}

// C<V.>L in a fog under one light collects the light of the first scattering in the medium alone,
// by its light sample. A render whose paths scatter in media once at most gathers just that
// light, from the same samples, so the two agree at each pixel. The deeper render's beauty holds
// more, from the paths that scatter again
BOOST_AUTO_TEST_CASE(moya_lightpaths_one_scattering_in_a_fog_matches_a_render_of_one_scattering_test) {
    Staged deep(8, 16);
    fogged(deep, 8);
    written(deep, { expression(deep, "single", "C<V.>L") });
    const film::Snapshot split = deep.render(nullptr);
    Staged shallow(8, 16);
    fogged(shallow, 1);
    const film::Snapshot alone = shallow.render(nullptr);
    double lit = 0.0;
    double beauty = 0.0;
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            for (std::uint32_t component = 0; component < 3U; ++component) {
                const float want = at(alone, x, y, component);
                lit += want;
                beauty += at(split, x, y, component);
                BOOST_TEST_CONTEXT("pixel " << x << ", " << y << ", component " << component) {
                    BOOST_TEST(std::fabs(at(split, x, y, split.offset("single") + component) - want) <= kRounding * std::fmax(want, 1.0F));
                }
            }
        }
    }
    BOOST_TEST(lit > 0.0);
    BOOST_TEST(beauty > lit);
}

/**
 * Light path expressions add channels and change nothing the paths gather, so a render with five
 * gives the same beauty, bit for bit, as one with none.
 *
 * Passes before the change: before it, no expression reaches the paths at all, and adding light
 * to channels as the paths gather it is the change that could disturb the beauty.
 **/
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

/**
 * The light of each light group, of the lights no group names and of an emitting object split
 * every path one way, so their channels sum to the beauty at each pixel. Each holds some light.
 *
 * Passes before the change: the change adds no code, since a group's channel is a light path
 * expression, and the case fails without the expressions on the device.
 **/
BOOST_AUTO_TEST_CASE(moya_lightpaths_the_light_groups_sum_to_the_beauty_test) {
    Staged staged(8, 16);
    grouped(staged, kKey);
    std::vector<scene::Handle> channels;
    channels.reserve(std::size(kGroups));
    for (std::size_t i = 0; i < std::size(kGroups); ++i) {
        channels.push_back(expression(staged, "group" + std::to_string(i), kGroups[i]));
    }
    written(staged, channels);
    const film::Snapshot picture = staged.render(nullptr);
    std::vector<double> held(std::size(kGroups), 0.0);
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            for (std::uint32_t component = 0; component < 3U; ++component) {
                float sum = 0.0F;
                for (std::size_t i = 0; i < std::size(kGroups); ++i) {
                    const float part = at(picture, x, y, picture.offset("group" + std::to_string(i)) + component);
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
    for (std::size_t i = 0; i < std::size(kGroups); ++i) {
        BOOST_TEST(held[i] > 0.0, kGroups[i]);
    }
}

/**
 * A group's channel added once more to the beauty is the picture of its lights twice as bright,
 * within errors at each pixel and over the picture. A compositor relies on that to change a
 * light after rendering. The beauty alone lies far from that picture over the whole of it.
 *
 * Passes before the change: the change adds no code, since a group's channel is a light path
 * expression, and the case fails without the expressions on the device.
 **/
BOOST_AUTO_TEST_CASE(moya_lightpaths_a_groups_channel_doubled_matches_its_lights_doubled_test) {
    Staged split(8, 64);
    grouped(split, kKey);
    written(split, { expression(split, "key", kGroups[0]) });
    film::Snapshot doubled = split.render(nullptr);
    const film::Snapshot beauty = doubled;
    const std::uint32_t key = doubled.offset("key");
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            const std::size_t first = ((static_cast<std::size_t>(y) * kSide) + x) * doubled.stride;
            for (std::uint32_t component = 0; component < 3U; ++component) {
                doubled.values[first + component] += doubled.values[first + key + component];
            }
        }
    }
    Staged brighter(8, 64);
    grouped(brighter, 2.0F * kKey);
    const film::Snapshot want = brighter.render(nullptr);
    const glm::uvec2 corner { kSide, kSide };
    BOOST_TEST(film::furthest(doubled, want).sigmas < kMatched);
    BOOST_TEST(film::sigmas(film::mean(doubled, {}, corner), film::mean(want, {}, corner)) < kMatched);
    BOOST_TEST(film::sigmas(film::mean(beauty, {}, corner), film::mean(want, {}, corner)) > kMatched);
}

};  // namespace
};  // namespace v3d::moya::tracing
