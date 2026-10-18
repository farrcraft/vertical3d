/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/camera/Camera.h>
#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Frustum.h>

#include <boost/test/unit_test.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

using v3d::type::geometry::Frustum;

v3d::type::geometry::AABBox box(const glm::vec3 & min, const glm::vec3 & max) {
    v3d::type::geometry::AABBox built;
    built.extents(min, max);
    return built;
}

/**
 * A symmetric orthographic volume two units on a side, with glm's default [-1, 1] depth.
 **/
glm::mat4x4 volume() {
    return glm::ortho(-1.0F, 1.0F, -1.0F, 1.0F, -1.0F, 1.0F);
}

/**
 * A camera at the origin looking along +z, keeping depths 1 to 100.
 **/
v3d::type::camera::Camera camera(bool orthographic) {
    v3d::type::camera::Camera built;
    built.orthographic(orthographic);
    built.profile().clipping(glm::vec2(1.0F, 100.0F));
    built.profile().eye(glm::vec3(0.0F, 0.0F, 0.0F));
    built.createProjection();
    built.createView();
    return built;
}

Frustum frustum(const v3d::type::camera::Camera & from, Frustum::Depth depth = Frustum::Depth::ZeroToOne) {
    return Frustum(from.projection() * from.view(), depth);
}

/**
 * A box well off each of the four side planes at depth 9 to 10 is outside, and one reaching
 * from the middle out past each is crossing. Wide enough to clear both cameras' volumes.
 **/
void checkSides(const Frustum & tested) {
    const glm::vec3 directions[] = {
        glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(-1.0F, 0.0F, 0.0F),
        glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(0.0F, -1.0F, 0.0F)
    };
    for (const glm::vec3 & direction : directions) {
        const glm::vec3 depth(0.0F, 0.0F, 9.0F);
        const glm::vec3 thickness(0.5F, 0.5F, 1.0F);
        const glm::vec3 offside = depth + direction * 50.0F;
        BOOST_TEST((tested.intersect(box(glm::min(offside, offside + thickness), glm::max(offside, offside + thickness))) == Frustum::OUTSIDE));

        const glm::vec3 middle = depth - thickness;
        const glm::vec3 reaching = depth + direction * 50.0F + thickness;
        BOOST_TEST((tested.intersect(box(glm::min(middle, reaching), glm::max(middle, reaching))) == Frustum::CROSSING));
    }
}

/**
 * A box inside every half space is inside the frustum.
 **/
BOOST_AUTO_TEST_CASE(frustum_inside_test) {
    Frustum tested(volume(), Frustum::Depth::MinusOneToOne);

    BOOST_TEST((tested.intersect(box(glm::vec3(-0.5F, -0.5F, -0.5F), glm::vec3(0.5F, 0.5F, 0.5F))) == Frustum::INSIDE));
}

/**
 * The half spaces are intersected, not unioned: a box excluded by one plane is out of the
 * frustum whatever the other five say.
 **/
BOOST_AUTO_TEST_CASE(frustum_outside_one_plane_test) {
    Frustum tested(volume(), Frustum::Depth::MinusOneToOne);

    // well off to the right, and so inside the left, top, bottom, near and far planes
    BOOST_TEST((tested.intersect(box(glm::vec3(10.0F, -0.5F, -0.5F), glm::vec3(20.0F, 0.5F, 0.5F))) == Frustum::OUTSIDE));

    // and the same off to the left
    BOOST_TEST((tested.intersect(box(glm::vec3(-20.0F, -0.5F, -0.5F), glm::vec3(-10.0F, 0.5F, 0.5F))) == Frustum::OUTSIDE));
}

/**
 * A box straddling a plane is neither in nor out.
 **/
BOOST_AUTO_TEST_CASE(frustum_crossing_test) {
    Frustum tested(volume(), Frustum::Depth::MinusOneToOne);

    BOOST_TEST((tested.intersect(box(glm::vec3(0.5F, -0.5F, -0.5F), glm::vec3(4.0F, 0.5F, 0.5F))) == Frustum::CROSSING));
}

/**
 * A camera's own frustum keeps what is just past its near plane and drops what is behind
 * its eye, and what lies between the eye and the near plane, and past the far plane.
 **/
BOOST_AUTO_TEST_CASE(frustum_camera_depth_test) {
    for (const bool orthographic : { false, true }) {
        const Frustum tested = frustum(camera(orthographic));

        BOOST_TEST((tested.intersect(box(glm::vec3(-0.1F, -0.1F, 1.1F), glm::vec3(0.1F, 0.1F, 1.5F))) == Frustum::INSIDE));
        BOOST_TEST((tested.intersect(box(glm::vec3(-0.1F, -0.1F, -3.0F), glm::vec3(0.1F, 0.1F, -2.0F))) == Frustum::OUTSIDE));
        BOOST_TEST((tested.intersect(box(glm::vec3(-0.1F, -0.1F, 0.6F), glm::vec3(0.1F, 0.1F, 0.8F))) == Frustum::OUTSIDE));
        BOOST_TEST((tested.intersect(box(glm::vec3(-0.1F, -0.1F, 0.5F), glm::vec3(0.1F, 0.1F, 2.0F))) == Frustum::CROSSING));
        BOOST_TEST((tested.intersect(box(glm::vec3(-0.1F, -0.1F, 110.0F), glm::vec3(0.1F, 0.1F, 120.0F))) == Frustum::OUTSIDE));
        BOOST_TEST((tested.intersect(box(glm::vec3(-0.1F, -0.1F, 90.0F), glm::vec3(0.1F, 0.1F, 110.0F))) == Frustum::CROSSING));
    }
}

/**
 * The depth range is not a formality. Read as [-1, 1], an orthographic camera's near plane
 * falls far behind its eye, and a perspective one's falls at half its near distance. A box
 * in that gap is kept by the wrong range and dropped by the right one.
 **/
BOOST_AUTO_TEST_CASE(frustum_depth_range_matters_test) {
    const v3d::type::geometry::AABBox gap = box(glm::vec3(-0.1F, -0.1F, 0.6F), glm::vec3(0.1F, 0.1F, 0.8F));
    for (const bool orthographic : { false, true }) {
        BOOST_TEST((frustum(camera(orthographic)).intersect(gap) == Frustum::OUTSIDE));
        BOOST_TEST((frustum(camera(orthographic), Frustum::Depth::MinusOneToOne).intersect(gap) == Frustum::INSIDE));
    }

    // and behind the eye, which only the orthographic camera's wrong near plane reaches
    const v3d::type::geometry::AABBox behind = box(glm::vec3(-0.1F, -0.1F, -3.0F), glm::vec3(0.1F, 0.1F, -2.0F));
    BOOST_TEST((frustum(camera(true), Frustum::Depth::MinusOneToOne).intersect(behind) == Frustum::INSIDE));
}

/**
 * Each of the four side planes excludes what is off its side and crosses what reaches past it.
 **/
BOOST_AUTO_TEST_CASE(frustum_camera_sides_test) {
    checkSides(frustum(camera(false)));
    checkSides(frustum(camera(true)));
}

/**
 * The planes come back in the order planes() documents, so a caller can pick one out.
 **/
BOOST_AUTO_TEST_CASE(frustum_planes_order_test) {
    const Frustum tested = frustum(camera(true));
    const glm::vec3 eye(0.0F, 0.0F, 0.0F);

    // the near plane faces away from the eye, the far plane back toward it
    BOOST_TEST((tested.planes()[4].classify(eye) == v3d::type::geometry::Plane::NEGATIVE));
    BOOST_TEST((tested.planes()[5].classify(eye) == v3d::type::geometry::Plane::POSITIVE));
    BOOST_TEST((tested.planes()[4].classify(glm::vec3(0.0F, 0.0F, 2.0F)) == v3d::type::geometry::Plane::POSITIVE));
}

};  // namespace
