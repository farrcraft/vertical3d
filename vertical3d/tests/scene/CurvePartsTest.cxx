/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/Index.h>
#include <api/curve/Curve.h>
#include <api/curve/Curves.h>
#include <vertical3d/src/scene/CurveParts.h>

#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * A poly curve of three points, a Bezier curve of two and a NURBS curve of four, in that order,
 * so that nine control points are numbered across them.
 **/
v3d::curve::Curves mixed() {
    v3d::curve::Curves curves;
    BOOST_REQUIRE(curves.add({ .shape = v3d::curve::Poly{ { { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 2.0F, 0.0F, 0.0F } } } }).empty());
    v3d::curve::Bezier bezier;
    for (const float x : { 0.0F, 1.0F }) {
        bezier.points.push_back({ .position = { x, 1.0F, 0.0F }, .left = { x - 0.25F, 1.0F, 0.0F },
            .right = { x + 0.25F, 1.0F, 0.0F }, .leftType = v3d::curve::Handle::Free, .rightType = v3d::curve::Handle::Free });
    }
    BOOST_REQUIRE(curves.add({ .shape = bezier }).empty());
    v3d::curve::Nurbs nurbs;
    nurbs.points = { { 0.0F, 2.0F, 0.0F }, { 1.0F, 3.0F, 0.0F }, { 2.0F, 3.0F, 0.0F }, { 3.0F, 2.0F, 0.0F } };
    BOOST_REQUIRE(curves.add({ .shape = nurbs }).empty());
    return curves;
}

BOOST_AUTO_TEST_CASE(curve_parts_points_test) {
    const v3d::curve::Curves curves = mixed();
    BOOST_CHECK_EQUAL(v3d::editor::pointCount(curves), 9U);

    // a point is numbered on from the points of the curves before it, and the number reads back
    const std::optional<v3d::editor::CurvePoint> fourth = v3d::editor::pointAt(curves, 3);
    BOOST_REQUIRE(fourth);
    BOOST_CHECK(*fourth == (v3d::editor::CurvePoint{ .curve = 1, .point = 0 }));
    BOOST_CHECK(*v3d::editor::pointAt(curves, 8) == (v3d::editor::CurvePoint{ .curve = 2, .point = 3 }));
    BOOST_CHECK(!v3d::editor::pointAt(curves, 9));
    for (v3d::brep::Index part = 0; part < 9; part++) {
        BOOST_CHECK_EQUAL(v3d::editor::pointPart(curves, *v3d::editor::pointAt(curves, part)), part);
    }

    // a Bezier point is its position, and every other type's is the point itself
    BOOST_CHECK(v3d::editor::controlPoint(curves.curve(1), 1) == glm::vec3(1.0F, 1.0F, 0.0F));
    BOOST_CHECK(v3d::editor::controlPoint(curves.curve(2), 1) == glm::vec3(1.0F, 3.0F, 0.0F));

    // every point is listed with its number, in order
    const std::vector<std::pair<v3d::brep::Index, glm::vec3>> listed = v3d::editor::controlPoints(curves);
    BOOST_REQUIRE_EQUAL(listed.size(), 9U);
    for (v3d::brep::Index part = 0; part < 9; part++) {
        BOOST_CHECK_EQUAL(listed[part].first, part);
    }
    BOOST_CHECK(listed[4].second == glm::vec3(1.0F, 1.0F, 0.0F));
}

BOOST_AUTO_TEST_CASE(curve_parts_handles_test) {
    const v3d::curve::Curves curves = mixed();

    // only the Bezier curve's points have handles, numbered twice the point's number and one more
    BOOST_CHECK(!v3d::editor::handleAt(curves, 0));
    BOOST_CHECK(!v3d::editor::handleAt(curves, 5));
    const std::optional<v3d::editor::CurveHandle> left = v3d::editor::handleAt(curves, 6);
    BOOST_REQUIRE(left);
    BOOST_CHECK(*left == (v3d::editor::CurveHandle{ .at = { .curve = 1, .point = 0 }, .side = v3d::curve::Side::Left }));
    const std::optional<v3d::editor::CurveHandle> right = v3d::editor::handleAt(curves, 9);
    BOOST_REQUIRE(right);
    BOOST_CHECK(*right == (v3d::editor::CurveHandle{ .at = { .curve = 1, .point = 1 }, .side = v3d::curve::Side::Right }));
    BOOST_CHECK(!v3d::editor::handleAt(curves, 10));
    BOOST_CHECK_EQUAL(v3d::editor::handlePart(curves, *right), 9U);
    BOOST_CHECK(v3d::editor::handlePosition(curves, *right) == glm::vec3(1.25F, 1.0F, 0.0F));

    const std::vector<std::pair<v3d::brep::Index, glm::vec3>> listed = v3d::editor::handlePoints(curves);
    BOOST_REQUIRE_EQUAL(listed.size(), 4U);
    BOOST_CHECK_EQUAL(listed[0].first, 6U);
    BOOST_CHECK_EQUAL(listed[3].first, 9U);
    BOOST_CHECK(listed[0].second == glm::vec3(-0.25F, 1.0F, 0.0F));
}

BOOST_AUTO_TEST_CASE(curve_parts_move_test) {
    const v3d::curve::Curves curves = mixed();
    const auto up = [](const glm::vec3& point) { return point + glm::vec3(0.0F, 1.0F, 0.0F); };

    // a Bezier point carries its handles with it
    v3d::curve::Curve bezier = curves.curve(1);
    v3d::editor::moveControl(&bezier, 0, up);
    const v3d::curve::BezierPoint& moved = std::get<v3d::curve::Bezier>(bezier.shape).points[0];
    BOOST_CHECK(moved.position == glm::vec3(0.0F, 2.0F, 0.0F));
    BOOST_CHECK(moved.left == glm::vec3(-0.25F, 2.0F, 0.0F));
    BOOST_CHECK(moved.right == glm::vec3(0.25F, 2.0F, 0.0F));
    BOOST_CHECK(std::get<v3d::curve::Bezier>(bezier.shape).points[1].position == glm::vec3(1.0F, 1.0F, 0.0F));

    // any other point moves alone
    v3d::curve::Curve nurbs = curves.curve(2);
    v3d::editor::moveControl(&nurbs, 2, up);
    BOOST_CHECK(v3d::editor::controlPoint(nurbs, 2) == glm::vec3(2.0F, 4.0F, 0.0F));
    BOOST_CHECK(v3d::editor::controlPoint(nurbs, 1) == glm::vec3(1.0F, 3.0F, 0.0F));
}

};  // namespace
