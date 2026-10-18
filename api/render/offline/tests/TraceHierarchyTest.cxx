/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/trace/Hierarchy.h>
#include <api/render/offline/trace/Hit.h>
#include <api/render/offline/trace/Intersection.h>
#include <api/render/offline/trace/Pose.h>
#include <api/render/offline/trace/Primitive.h>
#include <api/render/offline/trace/Scene.h>
#include <api/render/offline/trace/Sphere.h>
#include <api/render/offline/trace/Triangle.h>
#include <api/type/Random.h>
#include <api/type/geometry/Ray.h>

#include <cstddef>
#include <limits>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace {

typedef v3d::render::offline::trace::Scene Scene;
typedef v3d::render::offline::trace::Primitive Primitive;

/** How far along a ray every case starts looking. **/
constexpr float FROM = 0.001F;

/**
 * What the oracle finds: the nearest primitive and how far along the ray it is.
 **/
struct Found {
    const Primitive* primitive = nullptr;
    float distance = 0.0F;
};

/**
 * The nearest hit by testing every primitive in the order it was added, keeping the first of
 * two at one distance. This is the search the hierarchy replaces, kept here as its oracle.
 **/
Found linear(const Scene & scene, const v3d::type::geometry::Ray & ray, float time) {
    const Scene::Poses poses = scene.poses(time);
    Found nearest;
    nearest.distance = std::numeric_limits<float>::max();
    for (const boost::shared_ptr<const Primitive> & primitive : scene.primitives()) {
        v3d::render::offline::trace::Pose pose;
        const int motion = primitive->motion();
        if (motion >= 0) {
            pose.ahead = &poses.ahead[static_cast<std::size_t>(motion)];
            pose.backward = &poses.backward[static_cast<std::size_t>(motion)];
        }
        v3d::render::offline::trace::Intersection found;
        if (primitive->intersect(ray, FROM, pose, &found) && found.distance < nearest.distance) {
            nearest.primitive = primitive.get();
            nearest.distance = found.distance;
        }
    }
    return nearest;
}

/**
 * A scene of small triangles and spheres scattered through a box twenty units across, beside
 * a ground quad two hundred units wide, so the scene mixes scales. With `moving`, each
 * primitive slides and turns across a shutter from 0 to 1.
 **/
void scatter(Scene* scene, bool moving, unsigned int count) {
    v3d::type::Random random(7);
    const glm::vec3 low(-10.0F);
    const glm::vec3 high(10.0F);

    // the ground, which is still in every scene
    scene->add(v3d::render::offline::trace::Triangle({glm::vec3(-100.0F, -11.0F, -100.0F),
        glm::vec3(100.0F, -11.0F, -100.0F), glm::vec3(100.0F, -11.0F, 100.0F)}, glm::vec3(1.0F)));
    scene->add(v3d::render::offline::trace::Triangle({glm::vec3(-100.0F, -11.0F, -100.0F),
        glm::vec3(100.0F, -11.0F, 100.0F), glm::vec3(-100.0F, -11.0F, 100.0F)}, glm::vec3(1.0F)));

    for (unsigned int i = 0; i < count; i++) {
        const glm::vec3 at = random.inside(low, high);
        const glm::mat4x4 open = glm::translate(glm::mat4x4(1.0F), at);
        v3d::render::offline::MovingTransform placed(open);
        if (moving) {
            const glm::vec3 slide = 0.2F * random.inside(low, high);
            const float angle = random.range(-1.5F, 1.5F);
            const glm::vec3 axis = glm::normalize(random.inside(low, high) + glm::vec3(0.01F));
            placed = v3d::render::offline::MovingTransform();
            placed.begin({ 0.0F, 1.0F });
            placed.concat(open);
            placed.concat(glm::rotate(glm::translate(glm::mat4x4(1.0F), at + slide), angle, axis));
            placed.end();
        }
        const float extent = random.range(0.05F, 0.6F);
        if (i % 4 == 0) {
            scene->add(v3d::render::offline::trace::Sphere({.radius = extent, .zmin = -extent, .zmax = extent,
                .thetamax = 360.0F}, placed.reference(), glm::vec3(1.0F)), placed);
            continue;
        }
        // a little out of the plane at each corner, so the triangles face every way
        const glm::vec3 depths = 0.05F * random.inside(low, high);
        const glm::mat4x4 & reference = placed.reference();
        const glm::vec3 first(reference * glm::vec4(-extent, -extent, depths.x, 1.0F));
        const glm::vec3 second(reference * glm::vec4(extent, -extent, depths.y, 1.0F));
        const glm::vec3 third(reference * glm::vec4(0.0F, extent, depths.z, 1.0F));
        scene->add(v3d::render::offline::trace::Triangle({first, second, third}, glm::vec3(1.0F)), placed);
    }
}

/**
 * Rays from points around the scattered box toward points inside it, so most of them pass
 * through the primitives and some reach the ground.
 **/
std::vector<v3d::type::geometry::Ray> rays(unsigned int count) {
    v3d::type::Random random(11);
    std::vector<v3d::type::geometry::Ray> cast;
    for (unsigned int i = 0; i < count; i++) {
        const glm::vec3 origin = random.inside(glm::vec3(-30.0F), glm::vec3(30.0F));
        const glm::vec3 target = random.inside(glm::vec3(-10.0F), glm::vec3(10.0F));
        cast.emplace_back(v3d::type::geometry::Ray({.origin = origin, .direction = target - origin}));
    }
    return cast;
}

/**
 * Every ray finds what the oracle finds: the same primitive, at the same distance, or no hit.
 **/
void agree(const Scene & scene, const std::vector<v3d::type::geometry::Ray> & cast, float time) {
    const Scene::Poses poses = scene.poses(time);
    unsigned int hits = 0;
    unsigned int wrong = 0;
    for (const v3d::type::geometry::Ray & ray : cast) {
        const Found expected = linear(scene, ray, time);
        v3d::render::offline::trace::Hit hit;
        const bool met = scene.nearest(ray, FROM, &hit, poses);
        if (met != (expected.primitive != nullptr) ||
            (met && (hit.primitive != expected.primitive || hit.distance != expected.distance))) {
            wrong++;
        }
        hits += met ? 1U : 0U;
    }
    BOOST_TEST(wrong == 0U);
    // enough of the rays hit something for the comparison to mean something
    BOOST_TEST(hits > static_cast<unsigned int>(cast.size() / 2));
}

/**
 * For two thousand still triangles and spheres beside a large ground quad, the hierarchy's
 * nearest hit is the oracle's for every one of a thousand rays.
 **/
BOOST_AUTO_TEST_CASE(trace_the_hierarchy_finds_what_every_primitive_finds_test) {
    Scene scene;
    scatter(&scene, false, 2000);
    agree(scene, rays(1000), 0.0F);
}

/**
 * The same for a thousand primitives that each slide and turn, at the open end, halfway, and
 * at the close end of the shutter. A primitive's box holds it across the whole motion.
 **/
BOOST_AUTO_TEST_CASE(trace_the_hierarchy_holds_moving_primitives_test) {
    Scene scene;
    scatter(&scene, true, 1000);
    const std::vector<v3d::type::geometry::Ray> cast = rays(600);
    for (const float time : { 0.0F, 0.5F, 1.0F }) {
        BOOST_TEST_CONTEXT("at time " << time) {
            agree(scene, cast, time);
        }
    }
}

/**
 * An empty scene meets nothing. A scene of one primitive meets it, and a ray that passes
 * beside every box meets nothing.
 **/
BOOST_AUTO_TEST_CASE(trace_the_hierarchy_edge_cases_test) {
    const v3d::type::geometry::Ray down({.origin = glm::vec3(0.0F, 0.0F, 5.0F), .direction = glm::vec3(0.0F, 0.0F, -1.0F)});
    v3d::render::offline::trace::Hit hit;

    const Scene empty;
    BOOST_CHECK(!empty.nearest(down, FROM, &hit));

    Scene one;
    one.add(v3d::render::offline::trace::Triangle({glm::vec3(-1.0F, -1.0F, 0.0F), glm::vec3(1.0F, -1.0F, 0.0F),
        glm::vec3(0.0F, 1.0F, 0.0F)}, glm::vec3(1.0F)));
    BOOST_REQUIRE(one.nearest(down, FROM, &hit));
    BOOST_CHECK_CLOSE(hit.distance, 5.0F, 0.001F);

    Scene scattered;
    scatter(&scattered, false, 500);
    // far above the scattered box and parallel to the ground, so it reaches no box at all
    const v3d::type::geometry::Ray above({.origin = glm::vec3(-50.0F, 40.0F, 0.0F), .direction = glm::vec3(1.0F, 0.0F, 0.0F)});
    BOOST_CHECK(!scattered.nearest(above, FROM, &hit));
    BOOST_CHECK(linear(scattered, above, 0.0F).primitive == nullptr);
}

/**
 * Two triangles at one distance resolve to the one added first, as a search in order would,
 * whichever leaf of the hierarchy each lands in. A ray down the diagonal two triangles of a
 * quad share meets both.
 **/
BOOST_AUTO_TEST_CASE(trace_the_hierarchy_breaks_a_tie_by_order_test) {
    Scene scene;
    scatter(&scene, false, 200);
    const v3d::render::offline::trace::Triangle first({glm::vec3(20.0F, 20.0F, 0.0F), glm::vec3(22.0F, 20.0F, 0.0F),
        glm::vec3(22.0F, 22.0F, 0.0F)}, glm::vec3(1.0F, 0.0F, 0.0F));
    const v3d::render::offline::trace::Triangle second({glm::vec3(20.0F, 20.0F, 0.0F), glm::vec3(22.0F, 22.0F, 0.0F),
        glm::vec3(20.0F, 22.0F, 0.0F)}, glm::vec3(0.0F, 1.0F, 0.0F));
    scene.add(first);
    scene.add(second);
    const v3d::type::geometry::Ray diagonal({.origin = glm::vec3(21.0F, 21.0F, 5.0F), .direction = glm::vec3(0.0F, 0.0F, -1.0F)});
    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(scene.nearest(diagonal, FROM, &hit));
    BOOST_CHECK(hit.primitive == linear(scene, diagonal, 0.0F).primitive);
    BOOST_CHECK_EQUAL(hit.colour.r, 1.0F);
}

/**
 * A primitive added after a ray was traced is seen by the next ray, so the hierarchy is built
 * again.
 **/
BOOST_AUTO_TEST_CASE(trace_the_hierarchy_is_built_again_after_an_add_test) {
    Scene scene;
    scene.add(v3d::render::offline::trace::Triangle({glm::vec3(-1.0F, -1.0F, 0.0F), glm::vec3(1.0F, -1.0F, 0.0F),
        glm::vec3(0.0F, 1.0F, 0.0F)}, glm::vec3(1.0F)));
    const v3d::type::geometry::Ray down({.origin = glm::vec3(0.0F, 0.0F, 5.0F), .direction = glm::vec3(0.0F, 0.0F, -1.0F)});
    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(scene.nearest(down, FROM, &hit));
    BOOST_CHECK_CLOSE(hit.distance, 5.0F, 0.001F);

    scene.add(v3d::render::offline::trace::Triangle({glm::vec3(-1.0F, -1.0F, 2.0F), glm::vec3(1.0F, -1.0F, 2.0F),
        glm::vec3(0.0F, 1.0F, 2.0F)}, glm::vec3(1.0F)));
    BOOST_REQUIRE(scene.nearest(down, FROM, &hit));
    BOOST_CHECK_CLOSE(hit.distance, 3.0F, 0.001F);
}

/**
 * Boxes merge and grow as their points do, and an empty box has no area.
 **/
BOOST_AUTO_TEST_CASE(trace_a_hierarchy_box_test) {
    v3d::render::offline::trace::Hierarchy::Box box;
    BOOST_CHECK(box.empty());
    BOOST_CHECK_EQUAL(box.area(), 0.0F);
    box.extend(glm::vec3(0.0F));
    box.extend(glm::vec3(1.0F, 2.0F, 3.0F));
    BOOST_CHECK(!box.empty());
    // half the surface of a 1 by 2 by 3 box
    BOOST_CHECK_CLOSE(box.area(), 11.0F, 0.001F);
    BOOST_CHECK_CLOSE(box.centre().z, 1.5F, 0.001F);
}

};  // namespace
