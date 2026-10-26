/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Compare.h>
#include <api/render/offline/FrameBuffer.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/RenderContext.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Reference.h"

namespace v3d::moya::reference {
namespace {

/**
 * The ray hider's references, each rendered from a file and through the render context, against
 * one committed picture. Neither route is the reference, and the case fails if they disagree.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_triangle_reference_test) {
    v3d::moya::RenderContext rc;
    triangleScene(rc);
    rc.render();
    check(picture(rc), TRIANGLE, TRIANGLE_RENDERED);
    check(read({.file = TRIANGLE_SCENE})->image(v3d::moya::Planes::CHANNELS), TRIANGLE, TRIANGLE_RIB_RENDERED);
}

/**
 * A matte floor and a plastic panel, three lights of three kinds, and the panel's shadow
 * across the floor, one sample at each pixel centre.
 *
 * The picture does not test individual shaders, which have their own cases in
 * v3dtest_render_offline. This case checks the wiring between the shaders and the ray hider.
 * It fails if ambient() reaches no light, if a shadow ray starts on the surface it leaves, or if
 * an imager runs over the wrong plane.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_shaded_reference_test) {
    v3d::moya::RenderContext rc;
    raytraceShadedScene(rc);
    pixelCentres(rc);
    rc.render();
    check(picture(rc), TRACED_SHADED, TRACED_SHADED_RENDERED);
    check(read({.file = TRACED_SHADED_SCENE})->image(v3d::moya::Planes::CHANNELS), TRACED_SHADED,
        TRACED_SHADED_RIB_RENDERED);
}

/**
 * The same scene at the RI defaults, two by two samples under a gaussian two pixels wide.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_sampled_reference_test) {
    v3d::moya::RenderContext rc;
    raytraceShadedScene(rc);
    rc.render();
    check(picture(rc), TRACED_SAMPLED, TRACED_SAMPLED_RENDERED);
    check(read({.file = TRACED_SAMPLED_SCENE})->image(v3d::moya::Planes::CHANNELS), TRACED_SAMPLED,
        TRACED_SAMPLED_RIB_RENDERED);
}

/**
 * Two renders are equal byte for byte. A sampled picture can have a reference only if they are.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_render_is_repeatable_test) {
    v3d::moya::RenderContext first;
    raytraceShadedScene(first);
    first.render();
    v3d::moya::RenderContext second;
    raytraceShadedScene(second);
    second.render();

    const v3d::image::Difference difference = v3d::image::compare(*picture(first), *picture(second), 0);
    BOOST_CHECK_MESSAGE(difference.match, difference.description());
}

BOOST_AUTO_TEST_CASE(moya_raytrace_focus_reference_test) {
    v3d::moya::RenderContext rc;
    rc.hider("raytrace");
    focusScene(rc, true);
    rc.render();
    check(picture(rc), TRACED_FOCUS, TRACED_FOCUS_RENDERED);
    check(read({.file = TRACED_FOCUS_SCENE})->image(v3d::moya::Planes::CHANNELS), TRACED_FOCUS,
        TRACED_FOCUS_RIB_RENDERED);
}

BOOST_AUTO_TEST_CASE(moya_raytrace_motion_reference_test) {
    v3d::moya::RenderContext rc;
    rc.hider("raytrace");
    motionScene(rc);
    rc.render();
    check(picture(rc), TRACED_MOTION, TRACED_MOTION_RENDERED);
    check(read({.file = TRACED_MOTION_SCENE})->image(v3d::moya::Planes::CHANNELS), TRACED_MOTION,
        TRACED_MOTION_RIB_RENDERED);
}

/**
 * A metal sphere and a glass one over a checked floor: reflection, refraction, the fresnel
 * split between them, spheres, and a shadow through an occluder that is not opaque.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_spheres_reference_test) {
    v3d::moya::RenderContext rc;
    spheresScene(rc);
    rc.render();
    check(picture(rc), SPHERES, SPHERES_RENDERED);
    check(read({.file = SPHERES_SCENE})->image(v3d::moya::Planes::CHANNELS), SPHERES, SPHERES_RIB_RENDERED);
}

/**
 * A white quad under one distant light forty five degrees off its normal comes out at the
 * cosine of that angle. An unshaded quad would come out white.
 **/
BOOST_AUTO_TEST_CASE(moya_raytrace_lit_quad_test) {
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = read({.file = "data/raytrace-lit-quad.rib"});

    BOOST_CHECK_CLOSE(planes->value(v3d::moya::Planes::RED, 32, 24), 0.70710678F, 0.5F);
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::Planes::BLUE, 32, 24), 0.70710678F, 0.5F);
    // and nothing outside the quad
    BOOST_CHECK_SMALL(planes->value(v3d::moya::Planes::RED, 2, 2), 0.0001F);
}

};  // namespace
};  // namespace v3d::moya::reference
