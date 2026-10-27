/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/rib/Bases.h>
#include <api/render/offline/rib/Requests.h>
#include <api/render/offline/trace/Curve.h>
#include <api/render/offline/trace/Hit.h>
#include <api/render/offline/trace/Intersection.h>
#include <api/render/offline/trace/Pose.h>
#include <api/render/offline/trace/Scene.h>
#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Ray.h>

#include <cmath>
#include <cstddef>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

using v3d::render::offline::trace::Curve;
using v3d::render::offline::trace::Intersection;
using v3d::render::offline::trace::Pose;

constexpr float CLOSE = 1e-4F;

/**
 * A straight segment along x from -1 to 1 at z = 5, its handles a third of the way along, a
 * tenth of a unit wide.
 **/
Curve straight() {
    return Curve({ glm::vec3(-1, 0, 5), glm::vec3(-1.0F / 3.0F, 0, 5), glm::vec3(1.0F / 3.0F, 0, 5), glm::vec3(1, 0, 5) },
        glm::vec2(0.1F), glm::vec3(1.0F));
}

/**
 * A ray from the origin's plane straight along +z through (x, y).
 **/
v3d::type::geometry::Ray down(float x, float y) {
    return v3d::type::geometry::Ray({ .origin = glm::vec3(x, y, 0), .direction = glm::vec3(0, 0, 1) });
}

/**
 * A point on a cubic under a basis matrix, as RI defines it: [t^3 t^2 t 1] times the matrix
 * times the four points.
 **/
glm::vec3 underBasis(const v3d::render::offline::rib::Basis::Matrix & basis, const v3d::render::offline::rib::Segment & points, float t) {
    const float powers[4] = { t * t * t, t * t, t, 1.0F };
    glm::vec3 point(0.0F);
    for (std::size_t row = 0; row < 4; row++) {
        for (std::size_t column = 0; column < 4; column++) {
            point += powers[row] * basis[(row * 4) + column] * points[column];
        }
    }
    return point;
}

glm::vec3 bezierAt(const v3d::render::offline::rib::Segment & points, float t) {
    const float s = 1.0F - t;
    return (s * s * s * points[0]) + (3.0F * s * s * t * points[1]) + (3.0F * s * t * t * points[2]) + (t * t * t * points[3]);
}

BOOST_AUTO_TEST_SUITE(trace_curve)

/**
 * A ray through the ribbon meets it where the centre line is, within half its width of it, and
 * misses it further out or past either end.
 **/
BOOST_AUTO_TEST_CASE(a_ray_meets_the_ribbon_within_half_its_width) {
    const Curve curve = straight();
    Intersection found;
    BOOST_REQUIRE(curve.intersect(down(0.5F, 0.0F), 0.0F, Pose(), &found));
    BOOST_CHECK_SMALL(found.distance - 5.0F, CLOSE);
    BOOST_CHECK_SMALL(found.v - 0.75F, 0.01F);
    BOOST_CHECK_SMALL(found.u - 0.5F, 0.01F);

    // a ray a little off the centre line still meets it, part of the way across
    BOOST_REQUIRE(curve.intersect(down(0.0F, 0.04F), 0.0F, Pose(), &found));
    BOOST_CHECK(std::abs(found.u - 0.5F) > 0.3F);

    BOOST_CHECK(!curve.intersect(down(0.0F, 0.06F), 0.0F, Pose(), &found));
    BOOST_CHECK(!curve.intersect(down(1.2F, 0.0F), 0.0F, Pose(), &found));
    // nor is it met behind the ray, or nearer than the distance asked past
    BOOST_CHECK(!curve.intersect(v3d::type::geometry::Ray({ .origin = glm::vec3(0, 0, 0), .direction = glm::vec3(0, 0, -1) }),
        0.0F, Pose(), &found));
    BOOST_CHECK(!curve.intersect(down(0.0F, 0.0F), 6.0F, Pose(), &found));
}

/**
 * The ribbon faces whatever ray meets it, so its normal points back along the ray, turned
 * square to the curve, and it is as wide seen from the side as from the front.
 **/
BOOST_AUTO_TEST_CASE(the_ribbon_faces_the_ray) {
    const Curve curve = straight();
    Intersection found;
    const v3d::type::geometry::Ray slanted({ .origin = glm::vec3(0, -5, 0), .direction = glm::vec3(0, 1, 1) });
    BOOST_REQUIRE(curve.intersect(slanted, 0.0F, Pose(), &found));
    BOOST_CHECK_SMALL(glm::length(found.normal - glm::normalize(glm::vec3(0, -1, -1))), CLOSE);
    v3d::render::offline::trace::Hit hit;
    curve.describe(found, &hit);
    BOOST_CHECK((hit.normal == found.normal));
    BOOST_CHECK_EQUAL(hit.t, found.v);

    // from the side, along the curve's own plane, the ray still meets it
    const v3d::type::geometry::Ray side({ .origin = glm::vec3(0, -5, 5), .direction = glm::vec3(0, 1, 0) });
    BOOST_CHECK(curve.intersect(side, 0.0F, Pose(), &found));
}

/**
 * A bent segment is met where its curve is, not where its chord is, and the width can vary
 * along it.
 **/
BOOST_AUTO_TEST_CASE(a_bent_segment_is_met_on_its_curve) {
    // an arch from (-1, 0) to (1, 0) through (0, 0.75), and wider at its end
    const Curve arch({ glm::vec3(-1, 0, 5), glm::vec3(-1, 1, 5), glm::vec3(1, 1, 5), glm::vec3(1, 0, 5) }, glm::vec2(0.02F, 0.2F),
        glm::vec3(1.0F));
    Intersection found;
    BOOST_CHECK(arch.intersect(down(0.0F, 0.75F), 0.0F, Pose(), &found));
    BOOST_CHECK(!arch.intersect(down(0.0F, 0.0F), 0.0F, Pose(), &found));
    // the arch crosses y = 0.25 at x = -0.9525 near its narrow start and at 0.9525 near its wide
    // end, so a ray 0.0525 inside it misses at the start and meets it at the end
    BOOST_CHECK(!arch.intersect(down(-0.9F, 0.25F), 0.0F, Pose(), &found));
    BOOST_CHECK(arch.intersect(down(0.9F, 0.25F), 0.0F, Pose(), &found));
}

BOOST_AUTO_TEST_CASE(the_bound_holds_the_ribbon) {
    v3d::type::geometry::AABBox box;
    BOOST_REQUIRE(straight().bound(&box));
    BOOST_CHECK_SMALL(box.min().x - -1.05F, CLOSE);
    BOOST_CHECK_SMALL(box.max().y - 0.05F, CLOSE);
}

/**
 * A traced scene finds a curve as it finds any primitive, and a moving one where its motion
 * puts it at the ray's time.
 **/
BOOST_AUTO_TEST_CASE(a_scene_traces_a_curve) {
    v3d::render::offline::trace::Scene scene;
    scene.add(boost::make_shared<Curve>(straight()), v3d::render::offline::MovingTransform());
    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(scene.nearest(down(0.25F, 0.0F), 0.0F, &hit));
    BOOST_CHECK_SMALL(hit.distance - 5.0F, CLOSE);
}

/**
 * Each standard basis converts four points to the Bezier segment that is the same cubic.
 **/
BOOST_AUTO_TEST_CASE(a_basis_segment_is_a_bezier_segment) {
    const v3d::render::offline::rib::Segment points{ glm::vec3(0, 0, 0), glm::vec3(1, 2, 0), glm::vec3(3, -1, 1), glm::vec3(4, 1, 0) };
    for (const char* name : { "bezier", "b-spline", "catmull-rom", "power" }) {
        v3d::render::offline::rib::Basis::Matrix basis{};
        BOOST_REQUIRE(v3d::render::offline::rib::namedBasis(name, &basis));
        const v3d::render::offline::rib::Segment bezier = v3d::render::offline::rib::toBezier(basis, points);
        for (const float t : { 0.0F, 0.3F, 0.5F, 1.0F }) {
            BOOST_CHECK_MESSAGE(glm::length(bezierAt(bezier, t) - underBasis(basis, points, t)) < CLOSE, name << " at " << t);
        }
    }
}

/**
 * A curve's segments follow its step and wrap, and the varying values count each segment's
 * ends once.
 **/
BOOST_AUTO_TEST_CASE(a_curve_has_a_segment_per_step) {
    using v3d::render::offline::rib::segments;
    using v3d::render::offline::rib::varyingCount;
    const std::vector<glm::vec3> seven(7, glm::vec3(0.0F));
    BOOST_CHECK_EQUAL(segments(seven, true, false, v3d::render::offline::rib::Basis::BEZIER, 3).size(), 2U);
    BOOST_CHECK_EQUAL(varyingCount(7, true, false, 3), 3U);
    v3d::render::offline::rib::Basis::Matrix catmullRom{};
    BOOST_REQUIRE(v3d::render::offline::rib::namedBasis("catmull-rom", &catmullRom));
    BOOST_CHECK_EQUAL(segments(seven, true, false, catmullRom, 1).size(), 4U);
    BOOST_CHECK_EQUAL(segments(seven, true, true, catmullRom, 1).size(), 7U);
    BOOST_CHECK_EQUAL(varyingCount(7, true, true, 1), 7U);
    BOOST_CHECK_EQUAL(segments(seven, false, false, catmullRom, 1).size(), 6U);
    BOOST_CHECK_EQUAL(varyingCount(7, false, false, 1), 7U);
    BOOST_CHECK(segments(std::vector<glm::vec3>(3), true, false, catmullRom, 1).empty());

    // a linear segment is the straight line between its points
    const std::vector<v3d::render::offline::rib::Segment> line =
        segments({ glm::vec3(0, 0, 0), glm::vec3(3, 0, 0) }, false, false, catmullRom, 1);
    BOOST_REQUIRE_EQUAL(line.size(), 1U);
    BOOST_CHECK_SMALL(glm::length(bezierAt(line[0], 0.5F) - glm::vec3(1.5F, 0, 0)), CLOSE);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
