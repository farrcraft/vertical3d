/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/Area.h>
#include <api/render/offline/Uniform.h>
#include <api/type/Random.h>

#include <cmath>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

/** How many points each case samples. **/
constexpr unsigned int COUNT = 4000;

/**
 * A point sampled on a quad lies on the quad, and the mean of many is near its centre. The
 * quad is two triangles of unequal shape, so a sampler that weighted the triangles equally
 * rather than by area would still land on the quad and its mean would move off the centre.
 **/
BOOST_AUTO_TEST_CASE(area_points_on_a_quad_test) {
    v3d::render::offline::Area quad("1");
    // a quad from (0, 0) to (4, 1) at z = 2, split along a diagonal into two triangles
    quad.add(glm::vec3(0.0F, 0.0F, 2.0F), glm::vec3(4.0F, 0.0F, 2.0F), glm::vec3(4.0F, 1.0F, 2.0F));
    quad.add(glm::vec3(0.0F, 0.0F, 2.0F), glm::vec3(4.0F, 1.0F, 2.0F), glm::vec3(0.0F, 1.0F, 2.0F));
    BOOST_CHECK_CLOSE(quad.area(), 4.0F, 0.01F);

    v3d::type::Random random(3);
    glm::vec3 sum(0.0F);
    unsigned int off = 0;
    for (unsigned int k = 0; k < COUNT; k++) {
        const glm::vec3 point = quad.sample(glm::vec2(random.unit(), random.unit()));
        if (point.x < -1.0e-4F || point.x > 4.0001F || point.y < -1.0e-4F || point.y > 1.0001F ||
            std::abs(point.z - 2.0F) > 1.0e-4F) {
            off++;
        }
        sum += point;
    }
    BOOST_TEST(off == 0U);
    const glm::vec3 mean = sum / static_cast<float>(COUNT);
    BOOST_CHECK_SMALL(mean.x - 2.0F, 0.08F);
    BOOST_CHECK_SMALL(mean.y - 0.5F, 0.03F);
}

/**
 * A point sampled on a sphere lies on it, around its centre at its radius, and the points
 * spread to every side, so their mean is near the centre.
 **/
BOOST_AUTO_TEST_CASE(area_points_on_a_sphere_test) {
    v3d::render::offline::Area ball("2");
    const glm::vec3 centre(1.0F, -2.0F, 3.0F);
    ball.add({.placement = glm::translate(glm::mat4x4(1.0F), centre), .radius = 0.5F});
    BOOST_CHECK_CLOSE(ball.area(), 3.14159265F, 0.01F);

    v3d::type::Random random(5);
    glm::vec3 sum(0.0F);
    for (unsigned int k = 0; k < COUNT; k++) {
        const glm::vec3 point = ball.sample(glm::vec2(random.unit(), random.unit()));
        BOOST_CHECK_SMALL(glm::length(point - centre) - 0.5F, 1.0e-4F);
        sum += point;
    }
    BOOST_CHECK_SMALL(glm::length((sum / static_cast<float>(COUNT)) - centre), 0.03F);
}

/**
 * An area light with nothing in it has no area and samples the origin. Two lights with
 * different handles have different keys, so their samples at one point differ.
 **/
BOOST_AUTO_TEST_CASE(area_empty_and_keyed_test) {
    const v3d::render::offline::Area none("1");
    BOOST_CHECK(none.empty());
    BOOST_CHECK_EQUAL(none.area(), 0.0F);
    BOOST_CHECK(none.sample(glm::vec2(0.5F)) == glm::vec3(0.0F));
    BOOST_CHECK(v3d::render::offline::Area("1").key() == none.key());
    BOOST_CHECK(v3d::render::offline::Area("2").key() != none.key());

    v3d::render::offline::Area counted("3");
    BOOST_CHECK_EQUAL(counted.samples(), 16U);
    counted.samples(0);
    BOOST_CHECK_EQUAL(counted.samples(), 1U);
}

/**
 * The uniform maps land where they should: the disc's corners of the square on its rim and
 * its middle at the centre, a triangle's points inside it, and a sphere's on its surface.
 **/
BOOST_AUTO_TEST_CASE(uniform_maps_test) {
    BOOST_CHECK_SMALL(glm::length(v3d::render::offline::uniform::disc(glm::vec2(0.5F))), 1.0e-6F);
    BOOST_CHECK_CLOSE(glm::length(v3d::render::offline::uniform::disc(glm::vec2(1.0F, 0.5F))), 1.0F, 0.01F);
    const glm::vec3 first(0.0F);
    const glm::vec3 second(1.0F, 0.0F, 0.0F);
    const glm::vec3 third(0.0F, 1.0F, 0.0F);
    // the first number at zero is the first corner, and at one is the opposite edge
    BOOST_CHECK_SMALL(glm::length(v3d::render::offline::uniform::triangle(first, second, third, glm::vec2(0.0F, 0.3F))), 1.0e-6F);
    const glm::vec3 edge = v3d::render::offline::uniform::triangle(first, second, third, glm::vec2(1.0F, 0.3F));
    BOOST_CHECK_CLOSE(edge.x + edge.y, 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(glm::length(v3d::render::offline::uniform::sphere(glm::vec2(0.3F, 0.7F))), 1.0F, 0.01F);
}

};  // namespace
