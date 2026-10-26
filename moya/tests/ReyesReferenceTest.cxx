/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Compare.h>
#include <api/image/Factory.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/rib/Reader.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/RIBHandler.h>
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
 * Rendering the buckets writes samples where the polygon is, and leaves the rest of the planes
 * at what they were allocated with.
 **/
BOOST_AUTO_TEST_CASE(moya_renders_a_polygon_test) {
    v3d::moya::RenderContext rc;
    scene(rc);
    rc.render();

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = rc.picture();

    // the quad covers raster x over [15.2, 39.2] and y over [4.8, 28.8]
    BOOST_TEST(planes->value(v3d::moya::Planes::RED, 24, 16) == 1.0F);
    BOOST_TEST(planes->value(v3d::moya::Planes::BLUE, 24, 16) == 1.0F);
    // and nothing outside it
    BOOST_TEST(planes->value(v3d::moya::Planes::RED, 55, 40) == 0.0F);

    // a covered pixel took a depth from the geometry rather than keeping the one it was
    // cleared to
    BOOST_TEST(planes->value(v3d::moya::Planes::DEPTH, 24, 16) < 2.0F);
}

/**
 * A rendered image against a committed one. moya touches no window, device or swapchain,
 * so this runs on any machine.
 **/
BOOST_AUTO_TEST_CASE(moya_reference_test) {
    v3d::moya::RenderContext rc;
    scene(rc);
    rc.render();

    const boost::shared_ptr<v3d::image::Image> rendered =
        rc.picture()->image(v3d::moya::Planes::CHANNELS);

    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    const boost::shared_ptr<v3d::image::Image> reference = factory.read(REFERENCE);
    if (reference == nullptr) {
        // write what was rendered, so a missing reference can be inspected, or committed when
        // it is being regenerated on purpose
        keep(rendered, RENDERED);
    }
    BOOST_REQUIRE_MESSAGE(reference != nullptr,
        std::string("cannot read the reference image ") + REFERENCE + " - what was rendered is in " + RENDERED);

    // the tolerance is for float rounding across compilers, not for "close enough"
    const v3d::image::Difference difference = v3d::image::compare(*rendered, *reference, 1);
    if (!difference.match) {
        keep(rendered, RENDERED);
    }
    BOOST_CHECK_MESSAGE(difference.match,
        difference.description() + " - what was rendered instead is in " + RENDERED);
}

/**
 * The same scene said in RIB, against the same committed picture.
 *
 * The file and the code are two routes to one image, and neither route is the reference. If
 * the two disagree, this case fails. A fault in the reader shows as a wrong picture.
 **/
BOOST_AUTO_TEST_CASE(moya_reference_from_rib_test) {
    v3d::moya::RIBHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(RIB_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    const boost::shared_ptr<v3d::image::Image> rendered =
        handler.context().picture()->image(v3d::moya::Planes::CHANNELS);

    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);
    const boost::shared_ptr<v3d::image::Image> reference = factory.read(REFERENCE);
    BOOST_REQUIRE(reference != nullptr);

    const v3d::image::Difference difference = v3d::image::compare(*rendered, *reference, 1);
    if (!difference.match) {
        keep(rendered, RIB_RENDERED);
    }
    BOOST_CHECK_MESSAGE(difference.match,
        difference.description() + " - what the rib scene rendered instead is in " + RIB_RENDERED);
}

/**
 * A display of type "file" writes the sampled planes to a picture on disk. The driver's
 * --output reaches the context through RiDisplay, and nothing else writes a file.
 **/
BOOST_AUTO_TEST_CASE(moya_display_writes_a_file_test) {
    boost::filesystem::create_directory("data_out");
    const char* output = "data_out/display.png";
    boost::filesystem::remove(output);

    v3d::moya::RenderContext rc;
    scene(rc);
    rc.display({.name = output, .type = "file", .mode = "rgb"});
    rc.render();

    BOOST_TEST(boost::filesystem::exists(output));
}

/**
 * A context with no display leaves its samples in the planes.
 **/
BOOST_AUTO_TEST_CASE(moya_no_display_writes_nothing_test) {
    v3d::moya::RenderContext rc;
    scene(rc);
    rc.render();

    BOOST_TEST(rc.picture()->value(v3d::moya::Planes::RED, 24, 16) == 1.0F);
}

/**
 * A shaded scene: a matte surface and a plastic one, a distant light and a point light. Each
 * surface carries normals of its own, so the falloff is a gradient.
 *
 * The picture does not test individual shaders, which have their own cases in
 * v3dtest_render_offline. This case checks the wiring between the shading language and
 * the renderer.
 **/
BOOST_AUTO_TEST_CASE(moya_shaded_reference_test) {
    v3d::moya::RenderContext rc;
    shadedScene(rc);
    pixelCentres(rc);
    rc.render();

    check(rc.picture()->image(v3d::moya::Planes::CHANNELS),
        SHADED, SHADED_RENDERED);
}

/**
 * The same shaded scene said in RIB, against the same committed picture.
 *
 * The file and the code are two routes to one image. A scene with shaders has more for the two
 * routes to disagree about. A shader parameter bound on one route and defaulted on the other
 * shows here.
 **/
BOOST_AUTO_TEST_CASE(moya_shaded_reference_from_rib_test) {
    v3d::moya::RIBHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(SHADED_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().picture()->image(v3d::moya::Planes::CHANNELS),
        SHADED, SHADED_RIB_RENDERED);
}

/**
 * The shaded scene at the RI defaults, two by two samples under a gaussian two pixels wide.
 * Its edges are antialiased, and it is the same on every run because every pixel's samples
 * are seeded by where the pixel is.
 **/
BOOST_AUTO_TEST_CASE(moya_sampled_reference_test) {
    v3d::moya::RenderContext rc;
    shadedScene(rc);
    rc.render();

    check(rc.picture()->image(v3d::moya::Planes::CHANNELS),
        SAMPLED, SAMPLED_RENDERED);
}

/**
 * The sampled scene said in RIB, which names no sampling and so gets the defaults.
 **/
BOOST_AUTO_TEST_CASE(moya_sampled_reference_from_rib_test) {
    v3d::moya::RIBHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(SAMPLED_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().picture()->image(v3d::moya::Planes::CHANNELS),
        SAMPLED, SAMPLED_RIB_RENDERED);
}

/**
 * Two renders of the sampled scene are equal byte for byte.
 **/
BOOST_AUTO_TEST_CASE(moya_sampled_render_is_repeatable_test) {
    v3d::moya::RenderContext first;
    shadedScene(first);
    first.render();
    v3d::moya::RenderContext second;
    shadedScene(second);
    second.render();

    const v3d::image::Difference difference = v3d::image::compare(
        *first.picture()->image(v3d::moya::Planes::CHANNELS),
        *second.picture()->image(v3d::moya::Planes::CHANNELS), 0);
    BOOST_CHECK_MESSAGE(difference.match, difference.description());
}

/**
 * A primitive straddling a bucket edge renders as it does inside one bucket. The samples
 * belong to the frame rather than to a bucket, and the film filters across bucket edges, so
 * where the buckets fall does not show.
 **/
BOOST_AUTO_TEST_CASE(moya_bucket_edges_do_not_show_test) {
    v3d::moya::RenderContext small;
    small.bucketSize(8, 8);
    shadedScene(small);
    small.render();
    v3d::moya::RenderContext large;
    large.bucketSize(64, 64);
    shadedScene(large);
    large.render();

    const v3d::image::Difference difference = v3d::image::compare(
        *small.picture()->image(v3d::moya::Planes::CHANNELS),
        *large.picture()->image(v3d::moya::Planes::CHANNELS), 0);
    BOOST_CHECK_MESSAGE(difference.match, difference.description());
}

/**
 * Two quads at two depths through a lens focused on the nearer one.
 **/
BOOST_AUTO_TEST_CASE(moya_focus_reference_test) {
    v3d::moya::RenderContext rc;
    focusScene(rc, true);
    rc.render();

    check(rc.picture()->image(v3d::moya::Planes::CHANNELS), FOCUS, FOCUS_RENDERED);
}

BOOST_AUTO_TEST_CASE(moya_focus_reference_from_rib_test) {
    v3d::moya::RIBHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(FOCUS_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().picture()->image(v3d::moya::Planes::CHANNELS),
        FOCUS, FOCUS_RIB_RENDERED);
}

/**
 * The quad on the plane of focus is as sharp through the lens as through a pinhole: a point
 * at the focal distance moves by nothing whatever the lens point. The block is the columns
 * the blue quad's blur does not reach.
 **/
BOOST_AUTO_TEST_CASE(moya_in_focus_is_sharp_test) {
    for (const char* hider : HIDERS) {
        BOOST_TEST_CONTEXT("hider " << hider) {
            v3d::moya::RenderContext pinhole;
            pinhole.hider(hider);
            focusScene(pinhole, false);
            pinhole.render();
            v3d::moya::RenderContext lens;
            lens.hider(hider);
            focusScene(lens, true);
            lens.render();

            BOOST_CHECK_SMALL(largest(*pinhole.picture(), *lens.picture(), {.from = 0, .to = 30}),
                1.0F / 255.0F);
        }
    }
}

/**
 * The quad off the plane of focus spreads its edge over its circle of confusion. Under a one
 * pixel box, a pinhole leaves the edge in a pixel or two. The lens has a radius of a quarter
 * of a unit and is focused four units out. It blurs a point ten units out over
 * 2 * 0.25 * (10 - 4) / 10 of a unit, measured on the plane of focus. That is about five
 * pixels at this field of view. The reyes hider blurs by moving the micropolygon, and the
 * ray hider by moving the ray.
 **/
BOOST_AUTO_TEST_CASE(moya_out_of_focus_spreads_test) {
    for (const char* hider : HIDERS) {
        BOOST_TEST_CONTEXT("hider " << hider) {
            v3d::moya::RenderContext pinhole;
            pinhole.hider(hider);
            focusScene(pinhole, false);
            pinhole.sampling().filter = v3d::render::offline::Filter::Box;
            pinhole.sampling().width = glm::vec2(1.0F);
            pinhole.render();
            v3d::moya::RenderContext lens;
            lens.hider(hider);
            focusScene(lens, true);
            lens.sampling().filter = v3d::render::offline::Filter::Box;
            lens.sampling().width = glm::vec2(1.0F);
            lens.render();

            const unsigned int coverage = v3d::moya::Planes::COVERAGE;
            const unsigned int sharp = partial(*pinhole.picture(), coverage, {.row = 24, .from = 30, .to = 45});
            const unsigned int blurred = partial(*lens.picture(), coverage, {.row = 24, .from = 30, .to = 45});
            BOOST_CHECK_LE(sharp, 2U);
            BOOST_CHECK_GE(blurred, 4U);
            BOOST_CHECK_LE(blurred, 7U);
        }
    }
}

/**
 * A quad sliding and a quad turning while the shutter is open.
 **/
BOOST_AUTO_TEST_CASE(moya_motion_reference_test) {
    v3d::moya::RenderContext rc;
    motionScene(rc);
    rc.render();

    check(rc.picture()->image(v3d::moya::Planes::CHANNELS), MOTION, MOTION_RENDERED);
}

BOOST_AUTO_TEST_CASE(moya_motion_reference_from_rib_test) {
    v3d::moya::RIBHandler handler;
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());

    BOOST_REQUIRE(reader.read(MOTION_SCENE, &handler));
    BOOST_CHECK_EQUAL(reader.error(), "");

    check(handler.context().picture()->image(v3d::moya::Planes::CHANNELS),
        MOTION, MOTION_RIB_RENDERED);
}

/**
 * A quad translated across the shutter spreads over the distance it moved. Its coverage rises
 * and falls linearly along that distance. The reyes hider moves the micropolygons to a
 * sample's time, and the ray hider carries the ray back to where the quad was then.
 **/
BOOST_AUTO_TEST_CASE(moya_motion_spreads_linearly_test) {
    for (const char* hider : HIDERS) {
        BOOST_TEST_CONTEXT("hider " << hider) {
            v3d::moya::RenderContext rc;
            rc.hider(hider);
            slidingQuad(rc);
            rc.render();

            checkRamp(*rc.picture(), v3d::moya::Planes::COVERAGE, 24);
        }
    }
}

/**
 * A quad showing a texture through paintedplastic, under each hider.
 *
 * The image's 2 by 2 texel blocks are each one colour, so a pixel well inside a block reads
 * that colour exactly whether it is sampled at the pixel or a micropolygon away from it. The
 * case checks a grid of pixels under both hiders: the same s and t, the same way up, and the
 * same texel at each. Under the reyes hider the quad is larger than a grid, so it splits, and
 * its pieces carry their st with them.
 **/
BOOST_AUTO_TEST_CASE(moya_textured_quad_test) {
    for (const char* hider : HIDERS) {
        BOOST_TEST_CONTEXT("hider " << hider) {
            const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = read({.file = "data/textured.rib", .hider = hider});

            // red counts the block across and green the block down, and blue is the same in each
            for (unsigned int across = 0; across < 4; across++) {
                for (unsigned int down = 0; down < 4; down++) {
                    const unsigned int column = 20 + (8 * across);
                    const unsigned int row = 12 + (8 * down);
                    BOOST_CHECK_SMALL(planes->value(v3d::moya::Planes::RED, column, row) - (85.0F * across / 255.0F),
                        1.0F / 255.0F);
                    BOOST_CHECK_SMALL(planes->value(v3d::moya::Planes::GREEN, column, row) - (85.0F * down / 255.0F),
                        1.0F / 255.0F);
                    BOOST_CHECK_SMALL(planes->value(v3d::moya::Planes::BLUE, column, row) - (128.0F / 255.0F),
                        1.0F / 255.0F);
                }
            }
            // and outside the quad is the background
            BOOST_CHECK_EQUAL(planes->value(v3d::moya::Planes::BLUE, 4, 4), 0.0F);
        }
    }
}

/**
 * A plastic panel casting a shadow across a matte floor under the reyes hider. The shadow is
 * traced through the same scene the ray hider uses, so it falls where the ray hider's does.
 **/
BOOST_AUTO_TEST_CASE(moya_shadow_reference_from_rib_test) {
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = read({.file = SHADOW_SCENE});
    check(planes->image(v3d::moya::Planes::CHANNELS), SHADOW, SHADOW_RIB_RENDERED);

    /*
        The two hiders also agree on the same file away from the edges, where the reyes hider's
        micropolygons are flat. The regions checked are a shadow of both lights, the point
        light's shadow alone, a region lit by both, and the imager's background. An edge moves
        by up to a micropolygon, so only the insides are checked.
    */
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> traced = read({.file = SHADOW_SCENE, .hider = "raytrace"});
    const unsigned int pixels[][2] = { { 40, 30 }, { 24, 40 }, { 56, 26 }, { 8, 8 }, { 2, 2 } };
    for (const auto & pixel : pixels) {
        BOOST_TEST_CONTEXT("pixel " << pixel[0] << ", " << pixel[1]) {
            for (unsigned int channel = 0; channel < 3; channel++) {
                BOOST_CHECK_SMALL(planes->value(channel, pixel[0], pixel[1]) - traced->value(channel, pixel[0], pixel[1]),
                    1.5F / 255.0F);
            }
        }
    }
}

/**
 * A moving grid that sweeps more samples than the motion cache holds works out each
 * micropolygon's motion afresh, and draws the same picture as one that caches them.
 **/
BOOST_AUTO_TEST_CASE(reference_motion_without_a_cache_test) {
    v3d::moya::RenderContext cached;
    motionScene(cached);
    cached.render();

    v3d::moya::RenderContext uncached;
    uncached.motionCache(0);
    motionScene(uncached);
    uncached.render();

    const v3d::render::offline::FrameBuffer & cachedImage = *cached.picture();
    const v3d::render::offline::FrameBuffer & uncachedImage = *uncached.picture();
    BOOST_CHECK_EQUAL(largest(cachedImage, uncachedImage, {.from = 0, .to = cachedImage.width() - 1}), 0.0F);
}

};  // namespace
};  // namespace v3d::moya::reference
