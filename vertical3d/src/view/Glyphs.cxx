/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Glyphs.h"

#include <api/render/realtime/canvas/Line.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Light.h>

#include <cmath>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor {

namespace {

/**
 * How many segments a ring of a spot light's glyph is drawn with.
 **/
constexpr unsigned int RING_SIDES = 24;

/**
 * A point light's star reaches this far from the light along each axis, as a share of
 * GLYPH_LENGTH.
 **/
constexpr float STAR_REACH = 0.25F;

/**
 * How far a distant light's outer lines sit from its middle one, and how long its arrowhead's
 * strokes are, each as a share of GLYPH_LENGTH.
 **/
constexpr float RAY_SPACING = 0.2F;
constexpr float ARROWHEAD = 0.15F;

/**
 * A camera's picture: a rectangle about the +z axis, at a distance along it.
 **/
struct Picture {
    float halfWidth = 0.0F;
    float halfHeight = 0.0F;
    float depth = 0.0F;
};

/**
 * The picture's rectangle, joined round.
 **/
void frame(const Picture& shape, const glm::vec4& colour, v3d::render::realtime::canvas::Line* canvas) {
    const float x = shape.halfWidth;
    const float y = shape.halfHeight;
    const float z = shape.depth;
    canvas->polyline({ glm::vec3(-x, -y, z), glm::vec3(x, -y, z), glm::vec3(x, y, z), glm::vec3(-x, y, z) }, colour, true);
}

/**
 * The triangle above a picture that says which way is up.
 **/
void upMarker(const Picture& shape, const glm::vec4& colour, v3d::render::realtime::canvas::Line* canvas) {
    const float half = shape.halfWidth * 0.5F;
    const float y = shape.halfHeight;
    const float z = shape.depth;
    canvas->polyline({ glm::vec3(-half, y, z), glm::vec3(half, y, z), glm::vec3(0.0F, y + half, z) }, colour, true);
}

/**
 * A ring about the +z axis.
 **/
void ring(float depth, float radius, const glm::vec4& colour, v3d::render::realtime::canvas::Line* canvas) {
    canvas->circle({ .centre = glm::vec3(0.0F, 0.0F, depth), .axisU = glm::vec3(1.0F, 0.0F, 0.0F),
        .axisV = glm::vec3(0.0F, 1.0F, 0.0F), .radius = radius, .sides = RING_SIDES }, colour);
}

};  // namespace

/**
 **/
glm::mat4 glyphPlacement(const glm::mat4& world) {
    return withoutScale(world);
}

/**
 **/
void drawCamera(const Camera::Lens& lens, const glm::vec4& colour, v3d::render::realtime::canvas::Line* canvas) {
    if (canvas == nullptr) {
        return;
    }
    const float depth = GLYPH_LENGTH;
    if (lens.projection == Camera::Projection::Orthographic) {
        const Picture near{ .halfWidth = lens.extent * GLYPH_ASPECT, .halfHeight = lens.extent, .depth = 0.0F };
        const Picture far{ .halfWidth = near.halfWidth, .halfHeight = near.halfHeight, .depth = depth };
        frame(near, colour, canvas);
        frame(far, colour, canvas);
        for (const float x : { -far.halfWidth, far.halfWidth }) {
            for (const float y : { -far.halfHeight, far.halfHeight }) {
                canvas->line(glm::vec3(x, y, 0.0F), glm::vec3(x, y, depth), colour);
            }
        }
        upMarker(far, colour, canvas);
        return;
    }

    // the angle of view spans the picture's shorter side, which is its height
    const float halfHeight = depth * std::tan(glm::radians(lens.fov) * 0.5F);
    const Picture far{ .halfWidth = halfHeight * GLYPH_ASPECT, .halfHeight = halfHeight, .depth = depth };
    frame(far, colour, canvas);
    for (const float x : { -far.halfWidth, far.halfWidth }) {
        for (const float y : { -far.halfHeight, far.halfHeight }) {
            canvas->line(glm::vec3(0.0F), glm::vec3(x, y, depth), colour);
        }
    }
    upMarker(far, colour, canvas);
}

/**
 **/
void drawLight(const Light::Emission& emission, const glm::vec4& colour, v3d::render::realtime::canvas::Line* canvas) {
    if (canvas == nullptr) {
        return;
    }
    switch (emission.type) {
        case Light::Type::Point: {
            const float reach = GLYPH_LENGTH * STAR_REACH;
            canvas->line(glm::vec3(-reach, 0.0F, 0.0F), glm::vec3(reach, 0.0F, 0.0F), colour);
            canvas->line(glm::vec3(0.0F, -reach, 0.0F), glm::vec3(0.0F, reach, 0.0F), colour);
            canvas->line(glm::vec3(0.0F, 0.0F, -reach), glm::vec3(0.0F, 0.0F, reach), colour);
            break;
        }
        case Light::Type::Distant: {
            const float spacing = GLYPH_LENGTH * RAY_SPACING;
            const float head = GLYPH_LENGTH * ARROWHEAD;
            const glm::vec3 tip(0.0F, 0.0F, GLYPH_LENGTH);
            for (const float x : { -spacing, 0.0F, spacing }) {
                canvas->line(glm::vec3(x, 0.0F, 0.0F), glm::vec3(x, 0.0F, GLYPH_LENGTH), colour);
            }
            canvas->line(tip, tip + glm::vec3(-head, 0.0F, -head), colour);
            canvas->line(tip, tip + glm::vec3(head, 0.0F, -head), colour);
            break;
        }
        case Light::Type::Spot: {
            // each ring is where a side of that angle ends, so the glyph holds its shape for
            // any cone, a cone wider than a right angle included
            const float edge = emission.cone;
            const float soft = emission.cone - emission.delta;
            ring(GLYPH_LENGTH * std::cos(edge), GLYPH_LENGTH * std::sin(edge), colour, canvas);
            if (emission.delta > 0.0F) {
                ring(GLYPH_LENGTH * std::cos(soft), GLYPH_LENGTH * std::sin(soft), colour, canvas);
            }
            const float along = GLYPH_LENGTH * std::cos(edge);
            const float out = GLYPH_LENGTH * std::sin(edge);
            canvas->line(glm::vec3(0.0F), glm::vec3(out, 0.0F, along), colour);
            canvas->line(glm::vec3(0.0F), glm::vec3(-out, 0.0F, along), colour);
            canvas->line(glm::vec3(0.0F), glm::vec3(0.0F, out, along), colour);
            canvas->line(glm::vec3(0.0F), glm::vec3(0.0F, -out, along), colour);
            break;
        }
    }
}

/**
 **/
void drawGroup(const glm::vec4& colour, v3d::render::realtime::canvas::Line* canvas) {
    if (canvas == nullptr) {
        return;
    }
    canvas->box(glm::vec3(-GROUP_REACH), glm::vec3(GROUP_REACH), colour);
}

};  // namespace v3d::editor
