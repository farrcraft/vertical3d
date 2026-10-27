/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/curve/Basis.h>
#include <api/curve/Curve.h>

#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace {

using v3d::curve::Basis;
using v3d::curve::BasisCurve;
using v3d::curve::Bezier;
using v3d::curve::CatmullRom;
using v3d::curve::Curve;
using v3d::curve::Handle;
using v3d::curve::Nurbs;
using v3d::curve::Poly;

constexpr float CLOSE = 1e-4F;
constexpr unsigned int RESOLUTION = 7;
constexpr unsigned int PIECES = 4;

/**
 * The points every case builds its curve on.
 **/
std::vector<glm::vec3> corners() {
    return { { 0, 0, 0 }, { 1, 2, 0 }, { 3, 1, 1 }, { 4, 4, 0 }, { 6, 2, -1 } };
}

/**
 * Whether two lists of points are the same length and each pair is within CLOSE.
 **/
void same(const std::vector<glm::vec3>& got, const std::vector<glm::vec3>& expected, const std::string& what) {
    BOOST_REQUIRE_MESSAGE(got.size() == expected.size(), what << ": " << got.size() << " points against " << expected.size());
    for (std::size_t each = 0; each < got.size(); each++) {
        BOOST_TEST(glm::length(got[each] - expected[each]) < CLOSE, what << ", point " << each);
    }
}

/**
 * Convert a curve, and check that the basis curve evaluates to the curve's own points.
 **/
BasisCurve converts(const Curve& curve, const std::string& what, unsigned int resolution = RESOLUTION) {
    BOOST_REQUIRE(problem(curve).empty());
    BOOST_TEST(exact(curve), what);
    const BasisCurve basis = toBasis(curve, PIECES);
    BOOST_TEST(basis.periodic == curve.closed, what);
    same(evaluate(basis, resolution), evaluate(curve, resolution), what);
    return basis;
}

Bezier bezier() {
    Bezier made;
    for (const glm::vec3& point : corners()) {
        made.points.push_back({ .position = point, .left = point, .right = point, .leftType = Handle::Automatic,
            .rightType = Handle::Automatic });
    }
    // one point with free handles, so the handles are not all in line
    made.points[2].leftType = Handle::Free;
    made.points[2].rightType = Handle::Free;
    made.points[2].left = { 2, 3, 1 };
    made.points[2].right = { 3, -1, 2 };
    return made;
}

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

BOOST_AUTO_TEST_SUITE(curve_basis)

BOOST_AUTO_TEST_CASE(a_poly_curve_is_a_linear_basis_curve) {
    for (const bool closed : { false, true }) {
        const BasisCurve basis = converts(Curve{ .shape = Poly{ corners() }, .closed = closed }, "poly");
        BOOST_TEST((basis.basis == Basis::Linear));
        BOOST_TEST((basis.points == corners()));
    }
}

BOOST_AUTO_TEST_CASE(a_bezier_curve_is_a_bezier_basis_curve_of_its_points_and_handles) {
    for (const bool closed : { false, true }) {
        Bezier made = bezier();
        settle(&made, closed);
        const BasisCurve basis = converts(Curve{ .shape = made, .closed = closed }, closed ? "closed" : "open");
        BOOST_TEST((basis.basis == Basis::Bezier));
        // three points per segment, and one more on an open curve
        const std::size_t segments = closed ? corners().size() : corners().size() - 1;
        BOOST_TEST(basis.points.size() == (segments * 3) + (closed ? 0 : 1));
        BOOST_TEST((basis.points[1] == made.points[0].right));
        BOOST_TEST((basis.points[2] == made.points[1].left));
        BOOST_TEST((basis.points[3] == made.points[1].position));
    }
}

BOOST_AUTO_TEST_CASE(a_catmull_rom_curve_is_a_catmull_rom_basis_curve) {
    const BasisCurve open = converts(Curve{ .shape = CatmullRom{ corners() } }, "open");
    BOOST_TEST((open.basis == Basis::CatmullRom));
    // the end points are repeated beyond the ends, so the curve still reaches them
    BOOST_TEST(open.points.size() == corners().size() + 2);
    BOOST_TEST((open.points.front() == corners().front()));
    BOOST_TEST((open.points.back() == corners().back()));
    const BasisCurve closed = converts(Curve{ .shape = CatmullRom{ corners() }, .closed = true }, "closed");
    BOOST_TEST(closed.points.size() == corners().size());
}

BOOST_AUTO_TEST_CASE(a_two_point_catmull_rom_curve_converts) {
    converts(Curve{ .shape = CatmullRom{ { { 0, 0, 0 }, { 1, 1, 0 } } } }, "two points");
}

BOOST_AUTO_TEST_CASE(a_cubic_nurbs_curve_is_a_bezier_basis_curve_with_a_segment_per_span) {
    Nurbs nurbs;
    nurbs.points = corners();
    const BasisCurve basis = converts(Curve{ .shape = nurbs }, "clamped");
    BOOST_TEST((basis.basis == Basis::Bezier));
    BOOST_TEST(basis.points.size() == (2 * 3) + 1);

    // knots spaced unevenly, unclamped, with a span of no length
    nurbs.knots = { 0, 0.5F, 1, 1, 3, 3, 4, 7, 9 };
    converts(Curve{ .shape = nurbs }, "uneven");
}

BOOST_AUTO_TEST_CASE(a_closed_cubic_nurbs_curve_is_a_periodic_bezier_basis_curve) {
    Nurbs nurbs;
    nurbs.points = corners();
    const BasisCurve basis = converts(Curve{ .shape = nurbs, .closed = true }, "closed");
    BOOST_TEST(basis.points.size() == corners().size() * 3);
}

BOOST_AUTO_TEST_CASE(a_quadratic_nurbs_curve_converts_exactly) {
    Nurbs nurbs;
    nurbs.order = 3;
    nurbs.points = corners();
    converts(Curve{ .shape = nurbs }, "open");
    converts(Curve{ .shape = nurbs, .closed = true }, "closed");
}

BOOST_AUTO_TEST_CASE(equal_weights_other_than_one_still_convert_exactly) {
    Nurbs nurbs;
    nurbs.points = corners();
    nurbs.weights.assign(corners().size(), 2.5F);
    converts(Curve{ .shape = nurbs }, "weighted");
}

BOOST_AUTO_TEST_CASE(an_order_two_nurbs_curve_is_a_linear_basis_curve_on_its_points) {
    Nurbs nurbs;
    nurbs.order = 2;
    nurbs.points = corners();
    // a linear basis curve gives only its corners, which a NURBS curve gives at one piece a span
    const BasisCurve basis = converts(Curve{ .shape = nurbs }, "open", 1);
    BOOST_TEST((basis.basis == Basis::Linear));
    BOOST_TEST(basis.points.size() == corners().size());
    BOOST_TEST(converts(Curve{ .shape = nurbs, .closed = true }, "closed", 1).points.size() == corners().size());
}

BOOST_AUTO_TEST_CASE(a_weighted_circle_is_approximated_closely) {
    const Curve curve{ .shape = circle() };
    BOOST_TEST(!exact(curve));
    const BasisCurve basis = toBasis(curve, PIECES);
    BOOST_TEST((basis.basis == Basis::Bezier));
    BOOST_TEST(basis.points.size() == (4 * PIECES * 3) + 1);
    // four pieces to a quarter circle stay within 0.05% of its radius
    for (const glm::vec3& point : evaluate(basis, RESOLUTION)) {
        BOOST_TEST(std::abs(glm::length(point) - 1.0F) < 5e-4F);
    }
}

BOOST_AUTO_TEST_CASE(a_quartic_nurbs_curve_is_approximated_closely) {
    Nurbs nurbs;
    nurbs.order = 5;
    nurbs.points = corners();
    const Curve curve{ .shape = nurbs };
    BOOST_TEST(!exact(curve));
    const std::vector<glm::vec3> approximated = evaluate(toBasis(curve, PIECES), 1);
    const std::vector<glm::vec3> real = evaluate(curve, PIECES);
    // the pieces meet the curve where they join it
    same(approximated, real, "joins");
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
