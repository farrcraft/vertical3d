/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ViewOverlay.h"

#include <api/render/realtime/canvas/Canvas.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <api/ui/paint/Text.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include "ViewPort.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::view {

namespace {

/**
 * How many sides a ball and the disc behind the gizmo are drawn with.
 **/
constexpr unsigned int BALL_SIDES = 20;
constexpr unsigned int DISC_SIDES = 40;

/**
 * Where a letter's baseline sits below the point it is centred on, as a share of its size.
 **/
constexpr float BASELINE = 0.35F;

/**
 * How far beyond the end of a marker's line its letter is centred, as a share of the letter's
 * size.
 **/
constexpr float LETTER_GAP = 0.7F;

/**
 * How short a marker's line is, as a share of its full length, for its axis to be seen end on.
 **/
constexpr float END_ON = 0.25F;

/**
 * How much of an axis's colour fills a ball at a negative end.
 **/
constexpr float NEGATIVE_FILL = 0.25F;

constexpr std::string_view LETTERS[] = { "X", "Y", "Z" };

/**
 * Draw an axis's letter centred on a point.
 **/
void letter(std::size_t axis, const glm::vec2& at, const glm::vec4& colour, const OverlayLook& look,
    const v3d::ui::paint::Measure& measure, const v3d::ui::paint::Write& write) {
    const std::string_view text = LETTERS[axis];
    const float width = measure ? measure(text) : 0.0F;
    if (write) {
        write(text, glm::vec2(at.x - (width / 2.0F), at.y + (look.text * BASELINE)), colour);
    }
}

};  // namespace

/**
 **/
OverlayMetrics OverlayMetrics::scaled(float factor, float top) {
    return OverlayMetrics{ .marker = MARKER * factor, .inset = INSET * factor, .gizmo = GIZMO * factor, .ball = BALL * factor,
        .margin = MARGIN * factor, .top = top };
}

/**
 **/
std::vector<AxisEnd> axisEnds(const v3d::type::camera::Camera& camera, const glm::vec2& centre, float length, bool negative) {
    // the view turns a world direction into the camera's axes, and the projection's diagonal
    // says which way each of those runs on the screen
    const glm::mat3 turn(camera.view());
    const glm::mat4 projection = camera.projection();
    const float across = projection[0][0] < 0.0F ? -1.0F : 1.0F;
    const float down = projection[1][1] < 0.0F ? -1.0F : 1.0F;
    const glm::vec3 sight = glm::normalize(camera.profile().direction());

    std::vector<AxisEnd> ends;
    for (std::size_t axis = 0; axis < 3; axis++) {
        for (const bool positive : { true, false }) {
            if (!positive && !negative) {
                continue;
            }
            glm::vec3 world(0.0F);
            world[static_cast<glm::length_t>(axis)] = positive ? 1.0F : -1.0F;
            const glm::vec3 seen = turn * world;
            ends.push_back({ .axis = axis, .positive = positive,
                .point = centre + (glm::vec2(across * seen.x, down * seen.y) * length), .depth = -glm::dot(world, sight) });
        }
    }
    std::ranges::stable_sort(ends, {}, &AxisEnd::depth);
    return ends;
}

/**
 **/
glm::vec2 markerCentre(const glm::vec4& region, const OverlayMetrics& metrics) {
    return glm::vec2(region.x + metrics.inset, region.y + region.w - metrics.inset);
}

/**
 **/
glm::vec2 gizmoCentre(const glm::vec4& region, const OverlayMetrics& metrics) {
    const float reach = metrics.gizmo + metrics.ball;
    return glm::vec2(region.x + region.z - metrics.margin - reach, region.y + metrics.top + metrics.margin + reach);
}

/**
 **/
bool hasGizmo(const ViewPort& view) {
    return view.camera() && !view.camera()->orthographic();
}

/**
 **/
std::optional<AxisEnd> gizmoAt(const ViewPort& view, const OverlayMetrics& metrics, const glm::vec2& point) {
    if (!hasGizmo(view)) {
        return std::nullopt;
    }
    const std::vector<AxisEnd> ends = axisEnds(*view.camera(), gizmoCentre(view.region(), metrics), metrics.gizmo, true);
    // the nearest ball is drawn last, on top of the rest
    for (auto end = ends.rbegin(); end != ends.rend(); ++end) {
        if (glm::distance(point, end->point) <= metrics.ball) {
            return *end;
        }
    }
    return std::nullopt;
}

/**
 **/
void lookAlong(v3d::type::camera::Camera* camera, const AxisEnd& end) {
    v3d::type::camera::Profile& profile = camera->profile();
    const glm::vec3 eye = profile.eye();
    const glm::vec3 sight = glm::normalize(profile.direction());
    const float along = glm::dot(-eye, sight);
    const glm::vec3 focus = eye + (sight * (along > 0.0F ? along : 1.0F));
    const float distance = std::max(glm::distance(eye, focus), 1.0F);

    glm::vec3 axis(0.0F);
    axis[static_cast<glm::length_t>(end.axis)] = end.positive ? 1.0F : -1.0F;
    const glm::vec3 up = end.axis == 1 ? glm::vec3(0.0F, 0.0F, end.positive ? 1.0F : -1.0F) : glm::vec3(0.0F, 1.0F, 0.0F);

    profile.eye(focus + (axis * distance));
    profile.up(up);
    profile.lookat(focus);
    camera->createView();
}

/**
 **/
void drawOverlay(v3d::render::realtime::canvas::Canvas* canvas, const ViewPort& view, const OverlayMetrics& metrics,
    const OverlayLook& look, const v3d::ui::paint::Measure& measure, const v3d::ui::paint::Write& write,
    const std::optional<glm::vec2>& cursor) {
    const glm::vec4& region = view.region();
    if (!view.camera() || region.z <= 0.0F || region.w <= 0.0F) {
        return;
    }
    const v3d::type::camera::Camera& camera = *view.camera();

    const glm::vec2 marker = markerCentre(region, metrics);
    for (const AxisEnd& end : axisEnds(camera, marker, metrics.marker, false)) {
        const glm::vec4& colour = look.axes[end.axis];
        canvas->segment({ .from = marker, .to = end.point, .width = look.line }, colour);
        const glm::vec2 out = end.point - marker;
        const float reach = glm::length(out);
        if (reach >= metrics.marker * END_ON) {
            letter(end.axis, end.point + (out * (look.text * LETTER_GAP / reach)), colour, look, measure, write);
        }
    }

    if (!hasGizmo(view)) {
        return;
    }
    const glm::vec2 centre = gizmoCentre(region, metrics);
    const float reach = metrics.gizmo + metrics.ball;
    const bool over = cursor && glm::distance(*cursor, centre) <= reach;
    canvas->circle(centre, reach, DISC_SIDES, over ? look.hover : look.rest);
    const std::optional<AxisEnd> lit = cursor ? gizmoAt(view, metrics, *cursor) : std::nullopt;
    const float sweep = glm::two_pi<float>();
    for (const AxisEnd& end : axisEnds(camera, centre, metrics.gizmo, true)) {
        const glm::vec4& colour = look.axes[end.axis];
        const bool hovered = lit && lit->axis == end.axis && lit->positive == end.positive;
        if (end.positive) {
            canvas->segment({ .from = centre, .to = end.point, .width = look.line }, colour);
            canvas->circle(end.point, metrics.ball, BALL_SIDES, colour);
            letter(end.axis, end.point, look.label, look, measure, write);
        } else {
            canvas->circle(end.point, metrics.ball, BALL_SIDES, glm::vec4(colour.r, colour.g, colour.b, colour.a * NEGATIVE_FILL));
            canvas->ring({ .centre = end.point, .outer = metrics.ball, .inner = metrics.ball - look.line, .sides = BALL_SIDES,
                .start = 0.0F, .sweep = sweep }, colour);
        }
        if (hovered) {
            canvas->ring({ .centre = end.point, .outer = metrics.ball + look.line, .inner = metrics.ball, .sides = BALL_SIDES,
                .start = 0.0F, .sweep = sweep }, look.lit);
        }
    }
}

};  // namespace v3d::editor::view
