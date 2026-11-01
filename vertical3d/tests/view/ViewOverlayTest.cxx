/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/view/ViewOverlay.h>
#include <vertical3d/src/view/ViewPort.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::editor::view::AxisEnd;
using v3d::editor::view::OverlayMetrics;
using v3d::editor::view::ViewPort;

constexpr float CLOSE = 1e-4F;

/**
 * A view of a profile looking at the origin from an eye, over a 400 by 300 region at 10, 20, its
 * matrices built as a frame builds them. An eye on the y axis has z for up, as the Top view does,
 * and any other y.
 **/
ViewPort view(const glm::vec3& eye, bool orthographic) {
    const glm::vec3 up = eye.x == 0.0F && eye.z == 0.0F ? glm::vec3(0.0F, 0.0F, 1.0F) : glm::vec3(0.0F, 1.0F, 0.0F);
    v3d::type::camera::Profile profile("Test");
    profile.orthographic(orthographic);
    profile.orthoZoom(10.0F);
    profile.eye(eye);
    profile.up(up);
    profile.lookat(glm::vec3(0.0F));
    ViewPort made("Test", profile);
    made.resize(glm::vec4(10.0F, 20.0F, 400.0F, 300.0F));
    made.camera()->createProjection();
    made.camera()->createView();
    return made;
}

/**
 * @return the end of an axis among a list, which has to hold it
 **/
const AxisEnd& find(const std::vector<AxisEnd>& ends, std::size_t axis, bool positive) {
    const auto found = std::ranges::find_if(ends, [axis, positive](const AxisEnd& end) { return end.axis == axis && end.positive == positive; });
    BOOST_REQUIRE(found != ends.end());
    return *found;
}

BOOST_AUTO_TEST_SUITE(view_overlay_test)

/**
 * Seen from the front, x runs right and y up the screen, and z points straight away, so its end
 * sits on the centre and is drawn first.
 **/
BOOST_AUTO_TEST_CASE(the_front_sees_x_right_and_y_up) {
    const ViewPort front = view(glm::vec3(0.0F, 0.0F, -10.0F), true);
    const glm::vec2 centre(100.0F, 100.0F);
    const std::vector<AxisEnd> ends = v3d::editor::view::axisEnds(*front.camera(), centre, 20.0F, false);
    BOOST_REQUIRE_EQUAL(ends.size(), 3U);

    BOOST_CHECK_CLOSE(find(ends, 0, true).point.x, 120.0F, CLOSE);
    BOOST_CHECK_SMALL(find(ends, 0, true).point.y - 100.0F, CLOSE);
    BOOST_CHECK_CLOSE(find(ends, 1, true).point.y, 80.0F, CLOSE);
    BOOST_CHECK_SMALL(glm::distance(find(ends, 2, true).point, centre), CLOSE);
    BOOST_CHECK_CLOSE(find(ends, 2, true).depth, -1.0F, CLOSE);
    BOOST_CHECK_EQUAL(ends.front().axis, 2U);
}

/**
 * Seen from the top, z runs up the screen and y points at the viewer, so its end is drawn last.
 **/
BOOST_AUTO_TEST_CASE(the_top_sees_z_up_and_y_toward_the_viewer) {
    const ViewPort top = view(glm::vec3(0.0F, 10.0F, 0.0F), true);
    const std::vector<AxisEnd> ends = v3d::editor::view::axisEnds(*top.camera(), glm::vec2(0.0F), 10.0F, false);
    BOOST_CHECK_CLOSE(find(ends, 2, true).point.y, -10.0F, CLOSE);
    BOOST_CHECK_CLOSE(find(ends, 1, true).depth, 1.0F, CLOSE);
    BOOST_CHECK_EQUAL(ends.back().axis, 1U);
}

/**
 * Each negative end lies opposite its positive one, and the ends run from the farthest to the
 * nearest.
 **/
BOOST_AUTO_TEST_CASE(a_negative_end_is_opposite_its_positive_one) {
    const ViewPort perspective = view(glm::vec3(4.0F, 5.0F, -20.0F), false);
    const glm::vec2 centre(50.0F, 60.0F);
    const std::vector<AxisEnd> ends = v3d::editor::view::axisEnds(*perspective.camera(), centre, 30.0F, true);
    BOOST_REQUIRE_EQUAL(ends.size(), 6U);
    for (std::size_t axis = 0; axis < 3; axis++) {
        const AxisEnd& positive = find(ends, axis, true);
        const AxisEnd& negative = find(ends, axis, false);
        BOOST_CHECK_SMALL(glm::distance((positive.point + negative.point) * 0.5F, centre), 1e-3F);
        BOOST_CHECK_SMALL(positive.depth + negative.depth, CLOSE);
    }
    BOOST_CHECK(std::ranges::is_sorted(ends, {}, &AxisEnd::depth));
}

/**
 * The marker sits in from the lower left corner, and the gizmo in from the upper right corner
 * under the header.
 **/
BOOST_AUTO_TEST_CASE(the_marker_and_the_gizmo_sit_in_opposite_corners) {
    const OverlayMetrics metrics = OverlayMetrics::scaled(2.0F, 28.0F);
    BOOST_CHECK_EQUAL(metrics.gizmo, OverlayMetrics::GIZMO * 2.0F);
    BOOST_CHECK_EQUAL(metrics.top, 28.0F);

    const glm::vec4 region(10.0F, 20.0F, 400.0F, 300.0F);
    BOOST_CHECK(v3d::editor::view::markerCentre(region, metrics) == glm::vec2(10.0F + metrics.inset, 320.0F - metrics.inset));
    const float reach = metrics.gizmo + metrics.ball;
    BOOST_CHECK(v3d::editor::view::gizmoCentre(region, metrics) == glm::vec2(410.0F - metrics.margin - reach, 20.0F + 28.0F + metrics.margin + reach));
}

/**
 * A press on a ball of a perspective view's gizmo finds its end, the nearest where two overlap.
 * An orthographic view has no gizmo.
 **/
BOOST_AUTO_TEST_CASE(a_ball_of_the_gizmo_is_found_under_the_cursor) {
    const OverlayMetrics metrics;
    const ViewPort perspective = view(glm::vec3(0.0F, 0.0F, -10.0F), false);
    BOOST_REQUIRE(v3d::editor::view::hasGizmo(perspective));
    const glm::vec2 centre = v3d::editor::view::gizmoCentre(perspective.region(), metrics);

    const std::optional<AxisEnd> right = v3d::editor::view::gizmoAt(perspective, metrics, centre + glm::vec2(metrics.gizmo, 2.0F));
    BOOST_REQUIRE(right.has_value());
    BOOST_CHECK_EQUAL(right.value_or(AxisEnd{ .axis = 9 }).axis, 0U);
    BOOST_CHECK(right.value_or(AxisEnd{ .positive = false }).positive);

    // both ends of z sit on the centre, and the negative one faces the viewer
    const std::optional<AxisEnd> middle = v3d::editor::view::gizmoAt(perspective, metrics, centre);
    BOOST_REQUIRE(middle.has_value());
    BOOST_CHECK_EQUAL(middle.value_or(AxisEnd{ .axis = 9 }).axis, 2U);
    BOOST_CHECK(!middle.value_or(AxisEnd{ .positive = true }).positive);

    BOOST_CHECK(!v3d::editor::view::gizmoAt(perspective, metrics, centre + glm::vec2(metrics.gizmo * 0.5F, metrics.gizmo * 0.5F)));

    const ViewPort front = view(glm::vec3(0.0F, 0.0F, -10.0F), true);
    BOOST_CHECK(!v3d::editor::view::hasGizmo(front));
    BOOST_CHECK(!v3d::editor::view::gizmoAt(front, metrics, centre));
}

/**
 * Looking down an axis puts the eye on that axis's side of the point the camera looked at, as
 * far from it as before, facing it.
 **/
BOOST_AUTO_TEST_CASE(looking_along_an_axis_keeps_the_distance) {
    const ViewPort perspective = view(glm::vec3(0.0F, 5.0F, -20.0F), false);
    v3d::type::camera::Camera* camera = perspective.camera().get();
    const float distance = std::sqrt(425.0F);

    v3d::editor::view::lookAlong(camera, { .axis = 0, .positive = true });
    BOOST_CHECK_SMALL(glm::distance(camera->profile().eye(), glm::vec3(distance, 0.0F, 0.0F)), 1e-3F);
    BOOST_CHECK_SMALL(glm::distance(camera->profile().direction(), glm::vec3(-1.0F, 0.0F, 0.0F)), 1e-4F);

    // looking down y has z for up, as the Top view does
    v3d::editor::view::lookAlong(camera, { .axis = 1, .positive = true });
    BOOST_CHECK_SMALL(glm::distance(camera->profile().eye(), glm::vec3(0.0F, distance, 0.0F)), 1e-3F);
    BOOST_CHECK_SMALL(glm::distance(camera->profile().up(), glm::vec3(0.0F, 0.0F, 1.0F)), 1e-4F);

    v3d::editor::view::lookAlong(camera, { .axis = 2, .positive = false });
    BOOST_CHECK_SMALL(glm::distance(camera->profile().eye(), glm::vec3(0.0F, 0.0F, -distance)), 1e-3F);
    BOOST_CHECK_SMALL(glm::distance(camera->profile().direction(), glm::vec3(0.0F, 0.0F, 1.0F)), 1e-4F);
}

/**
 * A camera looking away from the origin turns about a point a unit in front of its eye.
 **/
BOOST_AUTO_TEST_CASE(a_camera_facing_away_turns_about_a_near_point) {
    v3d::type::camera::Profile profile("Away");
    profile.eye(glm::vec3(0.0F, 0.0F, 5.0F));
    profile.up(glm::vec3(0.0F, 1.0F, 0.0F));
    profile.lookat(glm::vec3(0.0F, 0.0F, 10.0F));
    v3d::type::camera::Camera camera(profile);

    v3d::editor::view::lookAlong(&camera, { .axis = 0, .positive = false });
    BOOST_CHECK_SMALL(glm::distance(camera.profile().eye(), glm::vec3(-1.0F, 0.0F, 6.0F)), 1e-4F);
    BOOST_CHECK_SMALL(glm::distance(camera.profile().direction(), glm::vec3(1.0F, 0.0F, 0.0F)), 1e-4F);
}

/**
 * A view draws its marker's letters, less that of an axis seen end on, and a perspective view
 * also the gizmo's three. The disc behind the gizmo is faint at rest and brighter while the
 * cursor is over it.
 **/
BOOST_AUTO_TEST_CASE(the_overlay_draws_the_marker_and_the_gizmo) {
    const OverlayMetrics metrics;
    const v3d::editor::view::OverlayLook look;
    std::vector<std::string> letters;
    const auto measure = [](std::string_view text) { return static_cast<float>(text.size()) * 6.0F; };
    const auto write = [&letters](std::string_view text, const glm::vec2&, const glm::vec4&) { letters.emplace_back(text); };

    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(420, 320));
    const ViewPort front = view(glm::vec3(0.0F, 0.0F, -10.0F), true);
    v3d::editor::view::drawOverlay(&canvas, front, metrics, look, measure, write, std::nullopt);
    BOOST_CHECK((letters == std::vector<std::string>{ "X", "Y" }) || (letters == std::vector<std::string>{ "Y", "X" }));

    letters.clear();
    canvas.clear();
    const ViewPort perspective = view(glm::vec3(4.0F, 5.0F, -20.0F), false);
    v3d::editor::view::drawOverlay(&canvas, perspective, metrics, look, measure, write, std::nullopt);
    BOOST_CHECK_EQUAL(letters.size(), 6U);
    const auto drawn = [&canvas](const glm::vec4& colour) {
        return std::ranges::any_of(canvas.vertices(), [&colour](const auto& vertex) { return vertex.colour == colour; });
    };
    BOOST_CHECK(drawn(look.rest));
    BOOST_CHECK(!drawn(look.hover));

    canvas.clear();
    v3d::editor::view::drawOverlay(&canvas, perspective, metrics, look, measure, write, v3d::editor::view::gizmoCentre(perspective.region(), metrics));
    BOOST_CHECK(drawn(look.hover));
    BOOST_CHECK(!drawn(look.rest));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
