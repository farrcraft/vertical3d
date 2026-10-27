/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/curve/Curve.h>

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace {

using v3d::curve::Bezier;
using v3d::curve::BezierPoint;
using v3d::curve::CatmullRom;
using v3d::curve::Curve;
using v3d::curve::Handle;
using v3d::curve::Nurbs;
using v3d::curve::Poly;
using v3d::curve::Side;

constexpr float CLOSE = 1e-5F;

bool near(const glm::vec3& got, const glm::vec3& expected) {
    return glm::length(got - expected) < CLOSE;
}

/**
 * A unit circle in the xy plane as a rational quadratic NURBS curve: four quarter arcs, each
 * from a point on the circle through a corner of the square around it, weighted by cos 45°.
 **/
Nurbs circle() {
    const float corner = std::sqrt(0.5F);
    Nurbs nurbs;
    nurbs.order = 3;
    nurbs.points = { { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { -1, 1, 0 }, { -1, 0, 0 }, { -1, -1, 0 }, { 0, -1, 0 },
        { 1, -1, 0 }, { 1, 0, 0 } };
    nurbs.weights = { 1, corner, 1, corner, 1, corner, 1, corner, 1 };
    nurbs.knots = { 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 4 };
    return nurbs;
}

/**
 * A Bezier point whose handles are both of one type and start on the point.
 **/
BezierPoint at(const glm::vec3& position, Handle type) {
    return { .position = position, .left = position, .right = position, .leftType = type, .rightType = type };
}

BOOST_AUTO_TEST_SUITE(curve_problem)

BOOST_AUTO_TEST_CASE(every_type_needs_two_points) {
    BOOST_TEST(!problem(Curve{ .shape = Poly{ { { 0, 0, 0 } } } }).empty());
    BOOST_TEST(!problem(Curve{ .shape = CatmullRom{ { { 0, 0, 0 } } } }).empty());
    BOOST_TEST(!problem(Curve{ .shape = Bezier{ { at({ 0, 0, 0 }, Handle::Free) } } }).empty());
    BOOST_TEST(problem(Curve{ .shape = Poly{ { { 0, 0, 0 }, { 1, 0, 0 } } } }).empty());
}

BOOST_AUTO_TEST_CASE(a_point_that_is_not_finite_is_refused) {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    BOOST_TEST(!problem(Curve{ .shape = Poly{ { { 0, 0, 0 }, { nan, 0, 0 } } } }).empty());
    BezierPoint handle = at({ 1, 0, 0 }, Handle::Free);
    handle.right.y = std::numeric_limits<float>::infinity();
    BOOST_TEST(!problem(Curve{ .shape = Bezier{ { at({ 0, 0, 0 }, Handle::Free), handle } } }).empty());
}

BOOST_AUTO_TEST_CASE(a_nurbs_curve_needs_as_many_points_as_its_order) {
    Nurbs nurbs;
    nurbs.points = { { 0, 0, 0 }, { 1, 0, 0 }, { 2, 0, 0 } };
    BOOST_TEST(!problem(Curve{ .shape = nurbs }).empty());
    nurbs.order = 3;
    BOOST_TEST(problem(Curve{ .shape = nurbs }).empty());
    nurbs.order = 1;
    BOOST_TEST(!problem(Curve{ .shape = nurbs }).empty());
}

BOOST_AUTO_TEST_CASE(weights_are_one_per_point_and_positive) {
    Nurbs nurbs = circle();
    nurbs.weights.pop_back();
    BOOST_TEST(!problem(Curve{ .shape = nurbs }).empty());
    nurbs = circle();
    nurbs.weights[1] = 0.0F;
    BOOST_TEST(!problem(Curve{ .shape = nurbs }).empty());
}

BOOST_AUTO_TEST_CASE(knots_are_counted_never_fall_and_leave_a_domain) {
    Nurbs nurbs = circle();
    BOOST_TEST(problem(Curve{ .shape = nurbs }).empty());
    nurbs.knots.pop_back();
    BOOST_TEST(!problem(Curve{ .shape = nurbs }).empty());
    nurbs = circle();
    nurbs.knots[5] = 0.5F;
    BOOST_TEST(!problem(Curve{ .shape = nurbs }).empty());
    nurbs = circle();
    nurbs.knots.assign(nurbs.knots.size(), 1.0F);
    BOOST_TEST(!problem(Curve{ .shape = nurbs }).empty());
}

BOOST_AUTO_TEST_CASE(a_closed_nurbs_curve_takes_no_knots_of_its_own) {
    Nurbs nurbs;
    nurbs.points = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 } };
    BOOST_TEST(problem(Curve{ .shape = nurbs, .closed = true }).empty());
    nurbs.knots = { 0, 1, 2, 3, 4, 5, 6, 7 };
    BOOST_TEST(problem(Curve{ .shape = nurbs }).empty());
    BOOST_TEST(!problem(Curve{ .shape = nurbs, .closed = true }).empty());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(curve_evaluate)

BOOST_AUTO_TEST_CASE(a_poly_curve_gives_its_points_and_a_closed_one_returns_to_its_first) {
    const std::vector<glm::vec3> points{ { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 } };
    BOOST_TEST((evaluate(Curve{ .shape = Poly{ points } }, 8) == points));
    const std::vector<glm::vec3> closed = evaluate(Curve{ .shape = Poly{ points }, .closed = true }, 8);
    BOOST_REQUIRE_EQUAL(closed.size(), 4U);
    BOOST_TEST((closed.back() == points.front()));
}

BOOST_AUTO_TEST_CASE(a_bezier_curve_passes_through_its_points_and_follows_its_handles) {
    // handles a third of the way along each segment make straight, evenly paced segments
    Bezier bezier;
    bezier.points = { at({ 0, 0, 0 }, Handle::Free), at({ 3, 0, 0 }, Handle::Free), at({ 3, 3, 0 }, Handle::Free) };
    bezier.points[0].right = { 1, 0, 0 };
    bezier.points[1].left = { 2, 0, 0 };
    bezier.points[1].right = { 3, 1, 0 };
    bezier.points[2].left = { 3, 2, 0 };
    const std::vector<glm::vec3> points = evaluate(Curve{ .shape = bezier }, 3);
    BOOST_REQUIRE_EQUAL(points.size(), 7U);
    const std::vector<glm::vec3> expected{ { 0, 0, 0 }, { 1, 0, 0 }, { 2, 0, 0 }, { 3, 0, 0 }, { 3, 1, 0 }, { 3, 2, 0 },
        { 3, 3, 0 } };
    for (std::size_t each = 0; each < expected.size(); each++) {
        BOOST_TEST(near(points[each], expected[each]), "point " << each);
    }
}

BOOST_AUTO_TEST_CASE(a_closed_bezier_curve_has_a_segment_back_to_its_first_point) {
    Bezier bezier;
    bezier.points = { at({ 0, 0, 0 }, Handle::Automatic), at({ 2, 0, 0 }, Handle::Automatic),
        at({ 1, 2, 0 }, Handle::Automatic) };
    settle(&bezier, true);
    const std::vector<glm::vec3> points = evaluate(Curve{ .shape = bezier, .closed = true }, 4);
    BOOST_REQUIRE_EQUAL(points.size(), 13U);
    BOOST_TEST(near(points[8], { 1, 2, 0 }));
    BOOST_TEST(near(points.back(), { 0, 0, 0 }));
}

BOOST_AUTO_TEST_CASE(a_catmull_rom_curve_passes_through_every_point) {
    const std::vector<glm::vec3> through{ { 0, 0, 0 }, { 1, 2, 0 }, { 3, 1, 1 }, { 4, 4, 0 } };
    for (const bool closed : { false, true }) {
        const std::vector<glm::vec3> points = evaluate(Curve{ .shape = CatmullRom{ through }, .closed = closed }, 5);
        const std::size_t segments = closed ? through.size() : through.size() - 1;
        BOOST_REQUIRE_EQUAL(points.size(), (segments * 5) + 1);
        for (std::size_t each = 0; each < through.size(); each++) {
            BOOST_TEST(near(points[each * 5], through[each]), "point " << each << (closed ? " closed" : " open"));
        }
        if (closed) {
            BOOST_TEST(near(points.back(), through.front()));
        }
    }
}

BOOST_AUTO_TEST_CASE(a_catmull_rom_segment_is_shaped_by_its_neighbours) {
    // the middle of a segment between equal neighbours lies off the line through its ends
    const std::vector<glm::vec3> points =
        evaluate(Curve{ .shape = CatmullRom{ { { 0, 0, 0 }, { 1, 0, 0 }, { 2, 1, 0 }, { 3, 1, 0 } } } }, 2);
    BOOST_TEST(near(points[3], { 1.5F, 0.5F, 0.0F }));
    BOOST_TEST(points[1].y < 0.0F);
}

BOOST_AUTO_TEST_CASE(a_clamped_nurbs_curve_starts_and_ends_on_its_end_points) {
    Nurbs nurbs;
    nurbs.points = { { 0, 0, 0 }, { 1, 3, 0 }, { 2, -1, 0 }, { 4, 2, 1 }, { 5, 0, 0 }, { 6, 1, 0 } };
    const std::vector<glm::vec3> points = evaluate(Curve{ .shape = nurbs }, 6);
    // six cubic points over clamped uniform knots make three spans
    BOOST_REQUIRE_EQUAL(points.size(), 19U);
    BOOST_TEST(near(points.front(), nurbs.points.front()));
    BOOST_TEST(near(points.back(), nurbs.points.back()));
}

BOOST_AUTO_TEST_CASE(a_cubic_nurbs_curve_of_four_points_is_a_bezier_segment) {
    Nurbs nurbs;
    nurbs.points = { { 0, 0, 0 }, { 1, 2, 0 }, { 3, 2, 1 }, { 4, 0, 0 } };
    Bezier bezier;
    bezier.points = { at({ 0, 0, 0 }, Handle::Free), at({ 4, 0, 0 }, Handle::Free) };
    bezier.points[0].right = { 1, 2, 0 };
    bezier.points[1].left = { 3, 2, 1 };
    const std::vector<glm::vec3> fromNurbs = evaluate(Curve{ .shape = nurbs }, 10);
    const std::vector<glm::vec3> fromBezier = evaluate(Curve{ .shape = bezier }, 10);
    BOOST_REQUIRE_EQUAL(fromNurbs.size(), fromBezier.size());
    for (std::size_t each = 0; each < fromNurbs.size(); each++) {
        BOOST_TEST(near(fromNurbs[each], fromBezier[each]), "point " << each);
    }
}

BOOST_AUTO_TEST_CASE(an_order_two_nurbs_curve_is_its_control_polygon) {
    Nurbs nurbs;
    nurbs.order = 2;
    nurbs.points = { { 0, 0, 0 }, { 2, 0, 0 }, { 2, 2, 0 } };
    const std::vector<glm::vec3> points = evaluate(Curve{ .shape = nurbs }, 2);
    BOOST_REQUIRE_EQUAL(points.size(), 5U);
    BOOST_TEST(near(points[1], { 1, 0, 0 }));
    BOOST_TEST(near(points[2], { 2, 0, 0 }));
    BOOST_TEST(near(points[4], { 2, 2, 0 }));
}

BOOST_AUTO_TEST_CASE(a_weighted_nurbs_curve_draws_an_exact_circle) {
    const std::vector<glm::vec3> points = evaluate(Curve{ .shape = circle() }, 16);
    BOOST_REQUIRE_EQUAL(points.size(), 65U);
    for (const glm::vec3& point : points) {
        BOOST_TEST(std::abs(glm::length(point) - 1.0F) < CLOSE);
    }
}

BOOST_AUTO_TEST_CASE(knots_of_no_length_are_skipped) {
    // a doubled interior knot makes a span of no length, and the curve has two spans, not three
    Nurbs nurbs;
    nurbs.order = 3;
    nurbs.points = { { 0, 0, 0 }, { 1, 1, 0 }, { 2, 0, 0 }, { 3, 1, 0 }, { 4, 0, 0 } };
    nurbs.knots = { 0, 0, 0, 1, 1, 2, 2, 2 };
    BOOST_TEST(evaluate(Curve{ .shape = nurbs }, 4).size() == 9U);
}

BOOST_AUTO_TEST_CASE(a_closed_nurbs_curve_returns_smoothly_to_its_start) {
    Nurbs nurbs;
    nurbs.points = { { 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 } };
    const std::vector<glm::vec3> points = evaluate(Curve{ .shape = nurbs, .closed = true }, 8);
    // one span per point
    BOOST_REQUIRE_EQUAL(points.size(), 33U);
    BOOST_TEST(near(points.front(), points.back()));
    // The points are symmetric, so the join turns as every other knot does. A curve that did not
    // wrap would turn sharply there instead.
    auto turn = [&](std::size_t before, std::size_t at, std::size_t after) {
        return glm::dot(glm::normalize(points[at] - points[before]), glm::normalize(points[after] - points[at]));
    };
    BOOST_TEST(std::abs(turn(points.size() - 2, 0, 1) - turn(7, 8, 9)) < CLOSE);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(curve_handles)

BOOST_AUTO_TEST_CASE(an_automatic_handle_lies_along_the_line_through_the_neighbours) {
    Bezier bezier;
    bezier.points = { at({ 0, 0, 0 }, Handle::Automatic), at({ 3, 3, 0 }, Handle::Automatic),
        at({ 6, 0, 0 }, Handle::Automatic) };
    settle(&bezier, false);
    const BezierPoint& middle = bezier.points[1];
    // the neighbours are level, so the handles are level, a third of the way to each neighbour
    const float reach = glm::length(glm::vec3(3, 3, 0)) / 3.0F;
    BOOST_TEST(near(middle.left, { 3.0F - reach, 3.0F, 0.0F }));
    BOOST_TEST(near(middle.right, { 3.0F + reach, 3.0F, 0.0F }));
}

BOOST_AUTO_TEST_CASE(an_automatic_handle_at_an_open_end_points_at_its_neighbour) {
    Bezier bezier;
    bezier.points = { at({ 0, 0, 0 }, Handle::Automatic), at({ 3, 0, 0 }, Handle::Automatic) };
    settle(&bezier, false);
    BOOST_TEST(near(bezier.points[0].right, { 1, 0, 0 }));
    // the handle facing away from the curve mirrors the one facing into it
    BOOST_TEST(near(bezier.points[0].left, { -1, 0, 0 }));
    BOOST_TEST(near(bezier.points[1].left, { 2, 0, 0 }));
    BOOST_TEST(near(bezier.points[1].right, { 4, 0, 0 }));
}

BOOST_AUTO_TEST_CASE(a_vector_handle_points_a_third_of_the_way_to_its_neighbour) {
    Bezier bezier;
    bezier.points = { at({ 0, 0, 0 }, Handle::Free), at({ 3, 3, 0 }, Handle::Vector), at({ 6, 0, 0 }, Handle::Free) };
    settle(&bezier, false);
    BOOST_TEST(near(bezier.points[1].left, { 2, 2, 0 }));
    BOOST_TEST(near(bezier.points[1].right, { 4, 2, 0 }));
}

BOOST_AUTO_TEST_CASE(free_and_aligned_handles_stay_where_they_are) {
    Bezier bezier;
    bezier.points = { at({ 0, 0, 0 }, Handle::Free), at({ 3, 3, 0 }, Handle::Aligned), at({ 6, 0, 0 }, Handle::Free) };
    bezier.points[1].left = { 1, 5, 0 };
    bezier.points[0].right = { 0, 7, 0 };
    const Bezier before = bezier;
    settle(&bezier, false);
    BOOST_TEST((bezier == before));
}

BOOST_AUTO_TEST_CASE(a_closed_curve_wraps_its_neighbours) {
    Bezier bezier;
    bezier.points = { at({ 0, 0, 0 }, Handle::Vector), at({ 3, 0, 0 }, Handle::Free), at({ 0, 3, 0 }, Handle::Free) };
    settle(&bezier, true);
    BOOST_TEST(near(bezier.points[0].left, { 0, 1, 0 }));
    BOOST_TEST(near(bezier.points[0].right, { 1, 0, 0 }));
}

BOOST_AUTO_TEST_CASE(a_moved_vector_handle_becomes_free) {
    Bezier bezier;
    bezier.points = { at({ 0, 0, 0 }, Handle::Vector), at({ 3, 0, 0 }, Handle::Vector) };
    settle(&bezier, false);
    moveHandle(&bezier, 0, Side::Right, { 1, 1, 0 });
    BOOST_TEST((bezier.points[0].rightType == Handle::Free));
    BOOST_TEST((bezier.points[0].leftType == Handle::Vector));
    BOOST_TEST(near(bezier.points[0].right, { 1, 1, 0 }));
}

BOOST_AUTO_TEST_CASE(a_moved_automatic_handle_aligns_both_and_turns_the_other) {
    Bezier bezier;
    bezier.points = { at({ 0, 0, 0 }, Handle::Automatic), at({ 3, 3, 0 }, Handle::Automatic),
        at({ 6, 0, 0 }, Handle::Automatic) };
    settle(&bezier, false);
    const float length = glm::length(bezier.points[1].left - bezier.points[1].position);
    moveHandle(&bezier, 1, Side::Right, { 3, 5, 0 });
    const BezierPoint& moved = bezier.points[1];
    BOOST_TEST((moved.leftType == Handle::Aligned));
    BOOST_TEST((moved.rightType == Handle::Aligned));
    BOOST_TEST(near(moved.right, { 3, 5, 0 }));
    BOOST_TEST(near(moved.left, glm::vec3(3, 3, 0) - glm::vec3(0, length, 0)));
}

BOOST_AUTO_TEST_CASE(an_aligned_handle_does_not_turn_a_free_one) {
    Bezier bezier;
    bezier.points = { at({ 0, 0, 0 }, Handle::Free), at({ 3, 0, 0 }, Handle::Free) };
    bezier.points[0].leftType = Handle::Aligned;
    bezier.points[0].left = { -1, 0, 0 };
    bezier.points[0].right = { 1, 0, 0 };
    moveHandle(&bezier, 0, Side::Left, { 0, -1, 0 });
    BOOST_TEST(near(bezier.points[0].right, { 1, 0, 0 }));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
