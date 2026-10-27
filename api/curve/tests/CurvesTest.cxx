/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/curve/Curve.h>
#include <api/curve/Curves.h>

#include <cstdint>
#include <variant>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

namespace {

using v3d::curve::Bezier;
using v3d::curve::BezierPoint;
using v3d::curve::CatmullRom;
using v3d::curve::Curve;
using v3d::curve::Curves;
using v3d::curve::Handle;
using v3d::curve::Poly;

Curve line() {
    return { .shape = Poly{ { { 0, 0, 0 }, { 1, 0, 0 } } } };
}

Curve bend() {
    return { .shape = CatmullRom{ { { 0, 0, 0 }, { 1, 1, 0 }, { 2, 0, 0 } } } };
}

BOOST_AUTO_TEST_SUITE(curve_curves)

BOOST_AUTO_TEST_CASE(a_curve_that_cannot_be_evaluated_is_refused_and_changes_nothing) {
    Curves curves;
    BOOST_TEST(!curves.add(Curve{ .shape = Poly{ { { 0, 0, 0 } } } }).empty());
    BOOST_TEST(curves.empty());
    BOOST_TEST(curves.changes() == 0U);

    BOOST_REQUIRE(curves.add(line()).empty());
    BOOST_TEST(!curves.set(0, Curve{ .shape = CatmullRom{} }).empty());
    BOOST_TEST((curves.curve(0) == line()));
    BOOST_TEST(curves.changes() == 1U);
}

BOOST_AUTO_TEST_CASE(curves_of_different_types_are_held_together_in_order) {
    Curves curves;
    BOOST_REQUIRE(curves.add(line()).empty());
    BOOST_REQUIRE(curves.add(bend()).empty());
    BOOST_REQUIRE(curves.size() == 2U);
    BOOST_TEST(std::holds_alternative<Poly>(curves.curve(0).shape));
    BOOST_TEST(std::holds_alternative<CatmullRom>(curves.curve(1).shape));

    curves.remove(0);
    BOOST_REQUIRE(curves.size() == 1U);
    BOOST_TEST((curves.curve(0) == bend()));
    BOOST_TEST(curves.evaluated(0).size() == (2 * Curves::DEFAULT_RESOLUTION) + 1);
}

BOOST_AUTO_TEST_CASE(every_change_moves_the_count) {
    Curves curves;
    std::uint64_t last = curves.changes();
    auto moved = [&]() {
        const bool did = curves.changes() > last;
        last = curves.changes();
        return did;
    };
    BOOST_REQUIRE(curves.add(line()).empty());
    BOOST_TEST(moved());
    BOOST_REQUIRE(curves.set(0, bend()).empty());
    BOOST_TEST(moved());
    curves.resolution(3);
    BOOST_TEST(moved());
    curves.remove(0);
    BOOST_TEST(moved());
    static_cast<void>(curves.size());
    BOOST_TEST(!moved());
}

BOOST_AUTO_TEST_CASE(evaluated_points_follow_the_curve_and_the_resolution) {
    Curves curves;
    BOOST_REQUIRE(curves.add(line()).empty());
    BOOST_TEST(curves.evaluated(0).size() == 2U);

    BOOST_REQUIRE(curves.set(0, bend()).empty());
    BOOST_TEST(curves.evaluated(0).size() == (2 * Curves::DEFAULT_RESOLUTION) + 1);

    curves.resolution(2);
    BOOST_TEST(curves.evaluated(0).size() == 5U);
    BOOST_TEST((curves.evaluated(0) == evaluate(bend(), 2)));
}

BOOST_AUTO_TEST_CASE(a_bezier_curve_is_settled_as_it_is_stored) {
    Bezier bezier;
    for (const glm::vec3& point : { glm::vec3(0, 0, 0), glm::vec3(3, 0, 0) }) {
        bezier.points.push_back(BezierPoint{ .position = point, .left = point, .right = point,
            .leftType = Handle::Vector, .rightType = Handle::Vector });
    }
    Curves curves;
    BOOST_REQUIRE(curves.add(Curve{ .shape = bezier }).empty());
    const Bezier& stored = std::get<Bezier>(curves.curve(0).shape);
    BOOST_TEST((stored.points[0].right == glm::vec3(1, 0, 0)));
    BOOST_TEST((stored.points[1].left == glm::vec3(2, 0, 0)));
}

BOOST_AUTO_TEST_CASE(a_copy_is_independent) {
    Curves curves;
    BOOST_REQUIRE(curves.add(line()).empty());
    Curves copy = curves;
    BOOST_REQUIRE(copy.set(0, bend()).empty());
    BOOST_TEST((curves.curve(0) == line()));
    BOOST_TEST(curves.evaluated(0).size() == 2U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
