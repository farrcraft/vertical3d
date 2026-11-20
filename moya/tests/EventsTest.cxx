/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Trails.h>
#include <moya/libmoya/kernel/integrate/Event.h>
#include <moya/libmoya/kernel/integrate/Trail.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Staged.h"

namespace v3d::moya::tracing {
namespace {

namespace film = v3d::moya::film;
namespace integrate = v3d::moya::integrate;
namespace events = v3d::moya::kernel::integrate;
namespace scene = v3d::moya::scene;

/** The picture's side, in pixels, and the pixels from one corner to the other of its middle. **/
constexpr std::int32_t kSide = Staged::kSide;
constexpr std::uint32_t kMiddleFrom = 6;
constexpr std::uint32_t kMiddleTo = 10;

/** A pixel's events, each spelled as a light path expression spells it, a star on a light's. **/
std::string spelled(const integrate::Trails & trails, std::uint32_t x, std::uint32_t y) {
    std::string out;
    for (const events::Step & step : trails.pixels[(y * kSide) + x]) {
        static const char * const kTypes = "CRTVLO";
        static const char * const kScatterings = " DGS";
        out += out.empty() ? "" : " ";
        out += kTypes[static_cast<std::uint32_t>(step.event.type)];
        if (step.event.scattering != events::Scattering::None) {
            out += kScatterings[static_cast<std::uint32_t>(step.event.scattering)];
        }
        if (step.event.label != kernel::light::kNoLabel) {
            out += "'" + trails.labels[step.event.label] + "'";
        }
        out += step.gathered != 0U ? "*" : "";
    }
    return out;
}

/** A transformation that turns a distant light's axis, -z, to shine along a direction. **/
glm::mat4 shining(const glm::vec3 & along) {
    const glm::vec3 axis = -glm::normalize(along);
    const glm::vec3 side { 0.0F, 0.0F, 1.0F };
    glm::mat4 out(1.0F);
    out[0] = glm::vec4(side, 0.0F);
    out[1] = glm::vec4(glm::cross(axis, side), 0.0F);
    out[2] = glm::vec4(axis, 0.0F);
    return out;
}

/**
 * A smooth mirror turned 45 degrees about y, which sends each camera ray along +x to a red
 * diffuse wall. A sun in the group key shines on the wall from above the mirror.
 **/
void mirrored(Staged & staged) {
    const scene::Handle mirror = staged.material("mirror", { { "base_color", scene::Value::colour(glm::vec3 { 1.0F }) },
                                                              { "base_metalness", scene::Value::number(1.0F) },
                                                              { "specular_roughness", scene::Value::number(0.0F) } });
    const scene::Handle red = staged.material("red", { { "base_color", scene::Value::colour(glm::vec3 { 0.8F, 0.1F, 0.1F }) }, { "specular_weight", scene::Value::number(0.0F) } });
    staged.quad("mirror", { { -0.7F, -0.7F, -0.7F }, { 0.7F, -0.7F, 0.7F }, { 0.7F, 0.7F, 0.7F }, { -0.7F, 0.7F, -0.7F } }, mirror);
    staged.quad("wall", { { 2.0F, -3.0F, -3.0F }, { 2.0F, 3.0F, -3.0F }, { 2.0F, 3.0F, 3.0F }, { 2.0F, -3.0F, 3.0F } }, red);
    const scene::Handle sun = staged.light("distantLight", "sun", "key");
    staged.made.set(sun, "transform", scene::Value::matrix(shining({ 1.0F, -2.0F, 0.0F })));
}

// a camera ray that a mirror sends to a wall under a sun makes a singular reflection there. It
// then takes a light sample of the sun at the wall, gathered through the wall's diffuse closure,
// and then makes a diffuse reflection. The path's depth limit ends it after that, so every pixel
// the mirror fills reports those events in that order
BOOST_AUTO_TEST_CASE(moya_events_a_mirror_then_a_wall_report_their_bounces_in_order_test) {
    Staged staged(2);
    mirrored(staged);
    integrate::Trails trails;
    staged.render(&trails);
    BOOST_TEST(trails.labels == std::vector<std::string> { "key" });
    for (std::uint32_t y = kMiddleFrom; y < kMiddleTo; ++y) {
        for (std::uint32_t x = kMiddleFrom; x < kMiddleTo; ++x) {
            BOOST_TEST_CONTEXT("pixel " << x << ", " << y) {
                BOOST_TEST(spelled(trails, x, y) == "C RS RD* L'key'* RD");
            }
        }
    }
}

// smooth glass facing the camera reflects or transmits each camera ray singularly, and rough
// glass glossily. Over the picture's middle, some rays cross it
BOOST_AUTO_TEST_CASE(moya_events_glass_transmits_singularly_or_glossily_by_its_roughness_test) {
    for (const auto & [roughness, scattering] : { std::pair { 0.0F, 'S' }, std::pair { 0.4F, 'G' } }) {
        BOOST_TEST_CONTEXT("roughness " << roughness) {
            Staged staged(1);
            const scene::Handle glass = staged.material("glass", { { "transmission_weight", scene::Value::number(1.0F) },
                                                                    { "specular_roughness", scene::Value::number(roughness) } });
            staged.quad("pane", { { -1.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }, glass);
            integrate::Trails trails;
            staged.render(&trails);
            std::uint32_t crossed = 0;
            for (std::uint32_t y = kMiddleFrom; y < kMiddleTo; ++y) {
                for (std::uint32_t x = kMiddleFrom; x < kMiddleTo; ++x) {
                    const std::string got = spelled(trails, x, y);
                    BOOST_TEST_CONTEXT("pixel " << x << ", " << y) {
                        BOOST_TEST((got == std::string("C R") + scattering || got == std::string("C T") + scattering), got);
                    }
                    crossed += got[2] == 'T' ? 1U : 0U;
                }
            }
            BOOST_TEST(crossed > 0U);
        }
    }
}

// a rect light in the group fill, which faces the camera and is seen by it, is gathered with its
// group's label by each camera ray that meets it. A dome in no group is gathered with none by
// each ray that leaves the scene. The labels are the groups' names, sorted, the sun's among them
BOOST_AUTO_TEST_CASE(moya_events_a_light_the_camera_sees_carries_its_groups_label_test) {
    Staged staged(0);
    staged.made.set(staged.light("rectLight", "panel", "fill"), "camera", scene::Value::integer(1));
    staged.light("distantLight", "sun", "key");
    staged.light("domeLight", "sky", "");
    integrate::Trails trails;
    staged.render(&trails);
    BOOST_TEST(trails.labels == (std::vector<std::string> { "fill", "key" }));
    BOOST_TEST(spelled(trails, kSide / 2U, kSide / 2U) == "C L'fill'*");
    BOOST_TEST(spelled(trails, 0, 0) == "C L*");
}

// a square whose material emits is an object, with no label, and a mesh light in the group glow
// is a light with its label. The camera gathers each where it sees it
BOOST_AUTO_TEST_CASE(moya_events_an_emitting_material_is_an_object_and_a_mesh_light_a_light_test) {
    Staged staged(0);
    const scene::Handle glowing = staged.material("glowing", { { "emission_luminance", scene::Value::number(1.0F) } });
    staged.quad("left", { { -1.0F, -1.0F, 0.0F }, { 0.0F, -1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }, glowing);
    const scene::Handle right = staged.quad("right", { { 0.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } }, {});
    const scene::Handle mesh = staged.light("meshLight", "panel", "glow");
    staged.made.set(mesh, "geometry", scene::Value::handle(right));
    staged.made.set(mesh, "camera", scene::Value::integer(1));
    integrate::Trails trails;
    staged.render(&trails);
    BOOST_TEST(spelled(trails, 4, kSide / 2U) == "C O*");
    BOOST_TEST(spelled(trails, 12, kSide / 2U) == "C L'glow'*");
}

// a camera ray into a dense fog under a sun behind the camera scatters in it. It takes a light
// sample of the sun there, gathered after the event in the medium, and then goes on from that
// event. Its depth limit in media ends it after that, so every pixel the fog fills reports those
// events in that order
BOOST_AUTO_TEST_CASE(moya_events_a_fog_reports_its_scattering_as_an_event_in_a_medium_test) {
    Staged staged(8);
    staged.made.set(staged.options, "volumeDepth", scene::Value::integer(1));
    staged.fog("fog", 1.0F, glm::vec3 { 0.0F }, glm::vec3 { 0.0F }, glm::vec3 { 8.0F }, 0.0F);
    const scene::Handle sun = staged.light("distantLight", "sun", "key");
    staged.made.set(sun, "angle", scene::Value::number(0.0F));
    staged.made.set(sun, "transform", scene::Value::matrix(glm::rotate(glm::mat4(1.0F), glm::radians(180.0F), glm::vec3 { 1.0F, 0.0F, 0.0F })));
    integrate::Trails trails;
    staged.render(&trails);
    for (std::uint32_t y = kMiddleFrom; y < kMiddleTo; ++y) {
        for (std::uint32_t x = kMiddleFrom; x < kMiddleTo; ++x) {
            BOOST_TEST_CONTEXT("pixel " << x << ", " << y) {
                BOOST_TEST(spelled(trails, x, y) == "C V* L'key'* V");
            }
        }
    }
}

// recording the events changes nothing a path gathers, so the picture is the same, bit for bit
BOOST_AUTO_TEST_CASE(moya_events_recording_them_leaves_the_picture_as_it_is_test) {
    Staged plain(2);
    mirrored(plain);
    const film::Snapshot want = plain.render(nullptr);
    Staged recorded(2);
    mirrored(recorded);
    integrate::Trails trails;
    const film::Snapshot got = recorded.render(&trails);
    BOOST_TEST(trails.pixels.size() == static_cast<std::size_t>(kSide) * kSide);
    BOOST_REQUIRE(got.values.size() == want.values.size());
    BOOST_REQUIRE(got.errors.size() == want.errors.size());
    const std::uint32_t differ = static_cast<std::uint32_t>(std::memcmp(got.values.data(), want.values.data(), want.values.size() * sizeof(float)) != 0) +
        static_cast<std::uint32_t>(std::memcmp(got.errors.data(), want.errors.data(), want.errors.size() * sizeof(float)) != 0);
    BOOST_TEST(differ == 0U);
}

};  // namespace
};  // namespace v3d::moya::tracing
