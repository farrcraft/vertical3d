/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <boost/test/unit_test.hpp>

#include "../Painter.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

constexpr glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);

/**
 * A colour that lets what is behind it through, which is what an outline drawn under a
 * plate would stop doing.
 **/
constexpr glm::vec4 translucent(0.1f, 0.1f, 0.1f, 0.5f);

const glm::vec2 low(20.0f, 30.0f);
const glm::vec2 high(140.0f, 90.0f);

/**
 * @return whether a point is inside a rounded box, by more than a tolerance
 *
 * A rounded box is not the rectangle that bounds it: its corners curve away from it, so an
 * outline legitimately reaches into the square left over. Testing against the rectangle
 * would call that a defect.
 **/
bool within(const glm::vec2& min, const glm::vec2& max, float radius, const glm::vec2& point) {
    const float tolerance = 0.001f;
    if (point.x <= min.x + tolerance || point.x >= max.x - tolerance ||
        point.y <= min.y + tolerance || point.y >= max.y - tolerance) {
        return false;
    }
    if (radius <= 0.0f) {
        return true;
    }
    // the centre of the corner the point is nearest, which it has to be inside the radius
    // of to be inside the box at all. A point past neither end of an axis is level with
    // that centre, so the axis contributes nothing to the distance
    const glm::vec2 centre(
        point.x < min.x + radius ? min.x + radius : std::min(point.x, max.x - radius),
        point.y < min.y + radius ? min.y + radius : std::min(point.y, max.y - radius));
    const glm::vec2 offset = point - centre;
    return std::sqrt(offset.x * offset.x + offset.y * offset.y) < radius - tolerance;
}

/**
 * @return whether any vertex the canvas holds lands inside a rounded box
 **/
bool anythingInside(const v3d::render::realtime::Canvas& canvas, const glm::vec2& min,
    const glm::vec2& max, float radius) {
    for (const v3d::render::realtime::Canvas::Vertex& vertex : canvas.vertices()) {
        if (within(min, max, radius, glm::vec2(vertex.position))) {
            return true;
        }
    }
    return false;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(painter_test)

/**
 * An outline traces the edges of a box and covers nothing inside them, square or rounded.
 *
 * This is what lets a plate's interior carry an alpha: an outline drawn as a box behind the
 * fill would show through it as a tint, and would stop whatever the plate covers showing
 * through at all.
 **/
BOOST_AUTO_TEST_CASE(an_outline_covers_nothing_inside_it) {
    const float width = 4.0f;
    const glm::vec2 inset(width, width);

    v3d::render::realtime::Canvas square;
    v3d::ui::strokeBox(&square, low, high, 0.0f, width, white);
    BOOST_CHECK(!square.empty());
    BOOST_CHECK(!anythingInside(square, low + inset, high - inset, 0.0f));

    v3d::render::realtime::Canvas rounded;
    v3d::ui::strokeBox(&rounded, low, high, 8.0f, width, white);
    BOOST_CHECK(!rounded.empty());
    BOOST_CHECK(!anythingInside(rounded, low + inset, high - inset, 4.0f));

    // and one whose outline is thicker than the corner it turns, which squares its corners
    // off rather than folding them over
    v3d::render::realtime::Canvas blunt;
    v3d::ui::strokeBox(&blunt, low, high, 2.0f, width, white);
    BOOST_CHECK(!blunt.empty());
    BOOST_CHECK(!anythingInside(blunt, low + inset, high - inset, 0.0f));
}

/**
 * A square outline is four runs and no turn, because each corner belongs to the run above or
 * below it. A rounded one is four runs and four bands.
 **/
BOOST_AUTO_TEST_CASE(a_square_outline_is_four_runs) {
    v3d::render::realtime::Canvas square;
    v3d::ui::strokeBox(&square, low, high, 0.0f, 2.0f, white);
    BOOST_CHECK_EQUAL(square.indices().size(), 4U * 6U);

    v3d::render::realtime::Canvas rounded;
    v3d::ui::strokeBox(&rounded, low, high, 6.0f, 2.0f, white);
    BOOST_CHECK_GT(rounded.indices().size(), 4U * 6U);
    // all of it is untextured, so none of it costs a draw of its own
    BOOST_CHECK_EQUAL(rounded.batches().size(), 1U);
}

/**
 * A plate is its outline and its fill, and the fill is the only thing over the middle of it.
 * The fill is drawn inside the outline rather than over it, so nothing is drawn twice under
 * an alpha.
 **/
BOOST_AUTO_TEST_CASE(a_plate_draws_its_interior_once) {
    const float width = 3.0f;
    const glm::vec2 inset(width, width);

    v3d::render::realtime::Canvas plate;
    v3d::ui::plateBox(&plate, low, high, 0.0f, width, translucent, white);

    v3d::render::realtime::Canvas outline;
    v3d::ui::strokeBox(&outline, low, high, 0.0f, width, white);

    v3d::render::realtime::Canvas fill;
    v3d::ui::fillBox(&fill, low + inset, high - inset, 0.0f, translucent);

    BOOST_CHECK_EQUAL(plate.indices().size(), outline.indices().size() + fill.indices().size());
}

/**
 * An outline as thick as half the shorter side is the whole box, and neither it nor the fill
 * inside it folds over.
 *
 * The two runs that meet in the middle are the whole of it; the other two have no length
 * left to cover and are not drawn, which is what says the clamp happened rather than the
 * box being drawn over itself twice.
 **/
BOOST_AUTO_TEST_CASE(an_outline_thicker_than_the_box_is_the_box) {
    v3d::render::realtime::Canvas canvas;
    const glm::vec2 max(low.x + 40.0f, low.y + 20.0f);

    v3d::ui::plateBox(&canvas, low, max, 0.0f, 40.0f, translucent, white);

    // the outline took all of it, so there is no interior left to fill
    BOOST_CHECK_EQUAL(canvas.indices().size(), 2U * 6U);
}

/**
 * A plate with no outline is one shape, so a theme that asks for no border pays for none.
 **/
BOOST_AUTO_TEST_CASE(a_plate_with_no_border_is_one_shape) {
    v3d::render::realtime::Canvas canvas;
    v3d::ui::plateBox(&canvas, low, high, 0.0f, 0.0f, translucent, white);

    BOOST_CHECK_EQUAL(canvas.indices().size(), 6U);
}

BOOST_AUTO_TEST_SUITE_END()
