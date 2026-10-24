/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampling.h>
#include <moya/libmoya/FrameBuffer.h>
#include <moya/libmoya/reyes/Samples.h>

#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

typedef v3d::moya::reyes::Samples Samples;

/** A frame of one pixel and one sample at its centre, which the film gives back exactly. **/
Samples one() {
    v3d::render::offline::Sampling sampling;
    sampling.samples = glm::uvec2(1, 1);
    sampling.filter = v3d::render::offline::Filter::Box;
    sampling.width = glm::vec2(1.0F, 1.0F);
    return Samples(glm::uvec2(1, 1), sampling);
}

/** What one pixel resolves to, red, green and blue. **/
glm::vec3 resolved(const Samples & samples) {
    v3d::render::offline::FrameBuffer planes(1, 1, v3d::moya::FrameBuffer::COVERAGE + 1);
    samples.resolve(&planes, v3d::moya::FrameBuffer::RED, v3d::moya::FrameBuffer::COVERAGE, v3d::moya::FrameBuffer::DEPTH);
    return { planes.value(v3d::moya::FrameBuffer::RED, 0, 0), planes.value(v3d::moya::FrameBuffer::RED + 1, 0, 0),
        planes.value(v3d::moya::FrameBuffer::RED + 2, 0, 0) };
}

/** A white surface of opacity one half, which as Ci is premultiplied to a half. **/
Samples::Layer pane(float depth, unsigned int surface) {
    return {.colour = glm::vec3(0.5F), .opacity = glm::vec3(0.5F), .depth = depth, .surface = surface};
}

/** An opaque red surface. **/
Samples::Layer red(float depth, unsigned int surface) {
    return {.colour = glm::vec3(1.0F, 0.0F, 0.0F), .opacity = glm::vec3(1.0F), .depth = depth, .surface = surface};
}

/**
 * A sample under a surface of opacity one half over an opaque red one resolves to RI's
 * composite: the front's Ci, and the back's Ci scaled by one minus the front's Oi. That is
 * 0.5 + (0.5 * red), or (1, 0.5, 0.5).
 **/
BOOST_AUTO_TEST_CASE(samples_composite_front_to_back_test) {
    Samples samples = one();
    samples.at(0, 0, 0).layers.add(pane(2.0F, 0));
    samples.at(0, 0, 0).layers.add(red(5.0F, 1));
    const glm::vec3 colour = resolved(samples);
    BOOST_CHECK_CLOSE(colour.r, 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(colour.g, 0.5F, 0.01F);
    BOOST_CHECK_CLOSE(colour.b, 0.5F, 0.01F);
}

/**
 * The surfaces are sorted before they are composited, so the order they arrive in does not
 * change the result.
 **/
BOOST_AUTO_TEST_CASE(samples_composite_in_any_order_test) {
    Samples nearFirst = one();
    nearFirst.at(0, 0, 0).layers.add(pane(2.0F, 0));
    nearFirst.at(0, 0, 0).layers.add(pane(3.0F, 1));
    nearFirst.at(0, 0, 0).layers.add(red(5.0F, 2));
    Samples farFirst = one();
    farFirst.at(0, 0, 0).layers.add(red(5.0F, 2));
    farFirst.at(0, 0, 0).layers.add(pane(3.0F, 1));
    farFirst.at(0, 0, 0).layers.add(pane(2.0F, 0));
    const glm::vec3 fromFront = resolved(nearFirst);
    const glm::vec3 fromBack = resolved(farFirst);
    BOOST_CHECK_CLOSE(fromFront.r, fromBack.r, 0.01F);
    BOOST_CHECK_CLOSE(fromFront.g, fromBack.g, 0.01F);
    // a half, then a quarter of white, then a quarter of red
    BOOST_CHECK_CLOSE(fromFront.g, 0.75F, 0.01F);
    BOOST_CHECK_CLOSE(fromFront.r, 1.0F, 0.01F);
}

/**
 * An opaque surface in front drops everything behind it, whether it arrives first or last, and
 * a surface behind an opaque one is not kept.
 **/
BOOST_AUTO_TEST_CASE(samples_an_opaque_surface_hides_what_is_behind_test) {
    Samples last = one();
    Samples::Layers & sample = last.at(0, 0, 0).layers;
    sample.add(pane(4.0F, 0));
    sample.add(pane(6.0F, 1));
    sample.add(red(3.0F, 2));
    BOOST_REQUIRE_EQUAL(sample.all().size(), 1U);
    BOOST_CHECK_EQUAL(sample.opaque(), 3.0F);
    sample.add(pane(5.0F, 3));
    BOOST_CHECK_EQUAL(sample.all().size(), 1U);
    const glm::vec3 colour = resolved(last);
    BOOST_CHECK_CLOSE(colour.r, 1.0F, 0.01F);
    BOOST_CHECK_SMALL(colour.g, 1.0e-6F);
}

/**
 * Two micropolygons of one grid that share an edge both cover a sample on it. The sample keeps
 * one layer from the grid, the nearer, so a translucent surface is not composited twice along
 * its micropolygons' edges.
 **/
BOOST_AUTO_TEST_CASE(samples_keep_one_layer_from_each_grid_test) {
    Samples samples = one();
    Samples::Layers & sample = samples.at(0, 0, 0).layers;
    sample.add(pane(2.5F, 7));
    sample.add(pane(2.4F, 7));
    sample.add(pane(2.6F, 7));
    BOOST_REQUIRE_EQUAL(sample.all().size(), 1U);
    BOOST_CHECK_EQUAL(sample.all()[0].depth, 2.4F);
    const glm::vec3 colour = resolved(samples);
    BOOST_CHECK_CLOSE(colour.g, 0.5F, 0.01F);
}

};  // namespace
