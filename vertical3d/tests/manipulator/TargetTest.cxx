/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Topology.h>
#include <api/animation/Curve.h>
#include <api/brep/Vertex.h>
#include <vertical3d/src/command/VertexCommand.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/SelectMask.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace {

/**
 * Where items with no parent are, as the scene's evaluation would place them.
 **/
template <typename T>
v3d::editor::Evaluation unparented(const std::vector<boost::shared_ptr<T>>& items) {
    v3d::editor::Evaluation evaluation;
    for (const boost::shared_ptr<T>& item : items) {
        evaluation.place(item->id(), { .local = item->matrix(), .world = item->matrix() });
    }
    return evaluation;
}

/**
 * The index of the cube's vertex at a corner, or INVALID_ID.
 **/
v3d::brep::Index corner(const v3d::brep::BRep& mesh, const glm::vec3& point) {
    for (v3d::brep::Index id = 0; id < mesh.vertexCount(); id++) {
        if (glm::distance(mesh.vertex(id)->point(), point) < 0.0001F) {
            return id;
        }
    }
    return v3d::brep::INVALID_ID;
}

/**
 * The index of the cube's face whose every corner has z of -0.5, which faces the front view.
 **/
v3d::brep::Index frontFace(const v3d::brep::BRep& mesh) {
    for (v3d::brep::Index face = 0; face < mesh.faceCount(); face++) {
        bool front = true;
        for (const v3d::brep::Index edge : v3d::brep::faceLoop(mesh, face)) {
            front = front && mesh.vertex(mesh.edge(edge)->vertex())->point().z < -0.49F;
        }
        if (front) {
            return face;
        }
    }
    return v3d::brep::INVALID_ID;
}

/**
 * The half edge that runs between two vertices, or INVALID_ID.
 **/
v3d::brep::Index edgeBetween(const v3d::brep::BRep& mesh, v3d::brep::Index from, v3d::brep::Index to) {
    for (v3d::brep::Index face = 0; face < mesh.faceCount(); face++) {
        const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(mesh, face);
        for (std::size_t entry = 0; entry < loop.size(); entry++) {
            const v3d::brep::Index start = mesh.edge(loop[(entry + loop.size() - 1) % loop.size()])->vertex();
            if (start == from && mesh.edge(loop[entry])->vertex() == to) {
                return loop[entry];
            }
        }
    }
    return v3d::brep::INVALID_ID;
}

BOOST_AUTO_TEST_CASE(target_objects_test) {
    const boost::shared_ptr<v3d::editor::Mesh> first = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    const boost::shared_ptr<v3d::editor::Mesh> second = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    first->translation(glm::vec3(1.0F, 0.0F, 0.0F));
    second->translation(glm::vec3(3.0F, 2.0F, 0.0F));

    const v3d::editor::Target target = v3d::editor::Target::objects({first, boost::shared_ptr<v3d::editor::Item>(), second},
        unparented<v3d::editor::Item>({first, second}));
    BOOST_CHECK_EQUAL(target.components(), false);
    BOOST_CHECK_EQUAL(target.items().size(), 2U);
    BOOST_CHECK_SMALL(glm::distance(target.pivot(), glm::vec3(2.0F, 1.0F, 0.0F)), 0.0001F);

    BOOST_CHECK(v3d::editor::Target().empty());
    BOOST_CHECK_SMALL(glm::length(v3d::editor::Target().pivot()), 0.0001F);
}

BOOST_AUTO_TEST_CASE(target_components_test) {
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());

    // nothing selected is nothing to move
    BOOST_CHECK(v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Face, unparented<v3d::editor::Mesh>({cube})).empty());

    // a face brings its four corners, and every face but the opposite one touches them
    const v3d::brep::Index face = frontFace(*cube->geometry());
    BOOST_REQUIRE(face != v3d::brep::INVALID_ID);
    cube->geometry()->face(face)->selected(true);
    const v3d::editor::Target faces = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Face, unparented<v3d::editor::Mesh>({cube}));
    BOOST_REQUIRE_EQUAL(faces.parts().size(), 1U);
    BOOST_CHECK_EQUAL(faces.parts()[0].vertices.size(), 4U);
    BOOST_CHECK_EQUAL(faces.parts()[0].faces.size(), 5U);
    BOOST_CHECK_SMALL(glm::distance(faces.pivot(), glm::vec3(0.0F, 0.0F, -0.5F)), 0.0001F);
    cube->geometry()->deselectComponents();

    // two edges that share a corner bring three vertices, the shared one once
    const v3d::brep::Index low = corner(*cube->geometry(), glm::vec3(-0.5F, -0.5F, -0.5F));
    const v3d::brep::Index right = corner(*cube->geometry(), glm::vec3(0.5F, -0.5F, -0.5F));
    const v3d::brep::Index up = corner(*cube->geometry(), glm::vec3(-0.5F, 0.5F, -0.5F));
    const v3d::brep::Index along = edgeBetween(*cube->geometry(), low, right);
    const v3d::brep::Index across = edgeBetween(*cube->geometry(), up, low);
    BOOST_REQUIRE(along != v3d::brep::INVALID_ID);
    BOOST_REQUIRE(across != v3d::brep::INVALID_ID);
    cube->geometry()->edge(along)->selected(true);
    cube->geometry()->edge(across)->selected(true);
    const v3d::editor::Target edges = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Edge, unparented<v3d::editor::Mesh>({cube}));
    const std::vector<v3d::brep::Index> expected = {low, right, up};
    std::vector<v3d::brep::Index> sorted = expected;
    std::sort(sorted.begin(), sorted.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(edges.parts()[0].vertices.begin(), edges.parts()[0].vertices.end(),
        sorted.begin(), sorted.end());
}

BOOST_AUTO_TEST_CASE(target_components_span_objects_test) {
    const boost::shared_ptr<v3d::editor::Mesh> first = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    const boost::shared_ptr<v3d::editor::Mesh> second = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    second->translation(glm::vec3(4.0F, 0.0F, 0.0F));
    first->geometry()->vertex(corner(*first->geometry(), glm::vec3(0.5F, 0.5F, 0.5F)))->selected(true);
    second->geometry()->vertex(corner(*second->geometry(), glm::vec3(0.5F, 0.5F, 0.5F)))->selected(true);

    // the pivot is the mean of the vertices in world space, across both objects
    const v3d::editor::Target target = v3d::editor::Target::components({first, second}, v3d::editor::SelectMask::Vertex, unparented<v3d::editor::Mesh>({first, second}));
    BOOST_CHECK_EQUAL(target.parts().size(), 2U);
    BOOST_CHECK_SMALL(glm::distance(target.pivot(), glm::vec3(2.5F, 0.5F, 0.5F)), 0.0001F);

    // the scene's selection makes the same target under the mask, and objects in object mode
    v3d::editor::Scene scene;
    first->selected(true);
    second->selected(true);
    scene.add(first);
    scene.add(second);
    BOOST_CHECK_EQUAL(v3d::editor::Target::selected(scene, v3d::editor::SelectMask::Vertex).components(), true);
    BOOST_CHECK_EQUAL(v3d::editor::Target::selected(scene, v3d::editor::SelectMask::Vertex).parts().size(), 2U);
    BOOST_CHECK_EQUAL(v3d::editor::Target::selected(scene, v3d::editor::SelectMask::Object).components(), false);
}

BOOST_AUTO_TEST_CASE(target_move_components_test) {
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    // the object is stretched along x, so a world offset is half as far in its own space
    cube->scale(glm::vec3(2.0F, 1.0F, 1.0F));
    const v3d::brep::Index low = corner(*cube->geometry(), glm::vec3(-0.5F, -0.5F, -0.5F));
    const v3d::brep::Index right = corner(*cube->geometry(), glm::vec3(0.5F, -0.5F, -0.5F));
    const v3d::brep::Index far = corner(*cube->geometry(), glm::vec3(0.5F, 0.5F, 0.5F));
    cube->geometry()->edge(edgeBetween(*cube->geometry(), low, right))->selected(true);
    cube->geometry()->edge(edgeBetween(*cube->geometry(), right, corner(*cube->geometry(), glm::vec3(0.5F, 0.5F, -0.5F))))->selected(true);

    v3d::editor::Target target = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Edge, unparented<v3d::editor::Mesh>({cube}));
    v3d::editor::Motion motion;
    motion.offset = glm::vec3(1.0F, 0.0F, 0.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));

    // the shared corner moves once, and the transform is left alone
    BOOST_CHECK_SMALL(glm::distance(cube->geometry()->vertex(right)->point(), glm::vec3(1.0F, -0.5F, -0.5F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(cube->geometry()->vertex(low)->point(), glm::vec3(0.0F, -0.5F, -0.5F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(cube->geometry()->vertex(far)->point(), glm::vec3(0.5F, 0.5F, 0.5F)), 0.0001F);
    BOOST_CHECK(cube->scale() == glm::vec3(2.0F, 1.0F, 1.0F));
    BOOST_CHECK(cube->translation() == glm::vec3(0.0F));

    // every face round a moved corner has its normal from its points again
    for (const v3d::brep::Index face : target.parts()[0].faces) {
        BOOST_CHECK_SMALL(glm::distance(cube->geometry()->face(face)->normal(), v3d::brep::newellNormal(*cube->geometry(), face)), 0.0001F);
    }
}

BOOST_AUTO_TEST_CASE(target_turn_components_test) {
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    const v3d::brep::Index face = frontFace(*cube->geometry());
    cube->geometry()->face(face)->selected(true);
    const v3d::brep::Index right = corner(*cube->geometry(), glm::vec3(0.5F, -0.5F, -0.5F));
    const v3d::brep::Index back = corner(*cube->geometry(), glm::vec3(0.5F, -0.5F, 0.5F));
    v3d::editor::Target target = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Face, unparented<v3d::editor::Mesh>({cube}));

    // a quarter turn about z turns the face about its own middle, so it stays in its plane
    // and each corner moves round to the next
    v3d::editor::Motion motion;
    motion.turn = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0F, 0.0F, 1.0F));
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    BOOST_CHECK_SMALL(glm::distance(cube->geometry()->vertex(right)->point(), glm::vec3(0.5F, 0.5F, -0.5F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(cube->geometry()->vertex(back)->point(), glm::vec3(0.5F, -0.5F, 0.5F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(cube->geometry()->face(face)->normal(), glm::vec3(0.0F, 0.0F, -1.0F)), 0.0001F);

    // a stretch past zero stops at the floor rather than turning the face inside out
    motion = v3d::editor::Motion();
    motion.stretch = glm::vec3(-3.0F, 1.0F, 1.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    BOOST_CHECK_SMALL(glm::distance(cube->geometry()->face(face)->normal(), glm::vec3(0.0F, 0.0F, -1.0F)), 0.0001F);
}

BOOST_AUTO_TEST_CASE(target_mask_kinds_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    light->translation(glm::vec3(4.0F, 0.0F, 0.0F));
    cube->selected(true);
    light->selected(true);
    scene.add(cube);
    scene.add(light);

    // the object mask moves both, and the light mask the light alone, at its own pivot
    BOOST_CHECK_EQUAL(v3d::editor::Target::selected(scene, v3d::editor::SelectMask::Object).items().size(), 2U);
    const v3d::editor::Target lights = v3d::editor::Target::selected(scene, v3d::editor::SelectMask::Light);
    BOOST_REQUIRE_EQUAL(lights.items().size(), 1U);
    BOOST_CHECK(lights.items()[0] == light);
    BOOST_CHECK_SMALL(glm::distance(lights.pivot(), glm::vec3(4.0F, 0.0F, 0.0F)), 0.0001F);
    BOOST_CHECK(v3d::editor::Target::selected(scene, v3d::editor::SelectMask::Camera).empty());
}

BOOST_AUTO_TEST_CASE(target_move_counts_a_change_test) {
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    cube->geometry()->vertex(0)->selected(true);
    v3d::editor::Target target = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Vertex, unparented<v3d::editor::Mesh>({cube}));

    // a component move changes the mesh's parts in place, and counts it, so a save sees it
    const std::uint64_t before = cube->revision();
    v3d::editor::Motion motion;
    motion.offset = glm::vec3(1.0F, 0.0F, 0.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    BOOST_CHECK_GT(cube->revision(), before);
    // so does putting a shape back, which a vertex command's undo and redo do
    const v3d::editor::VertexCommand::Shape shape = v3d::editor::VertexCommand::Shape::of(target.parts()[0]);
    const std::uint64_t moved = cube->revision();
    shape.applyTo(target.parts()[0]);
    BOOST_CHECK_GT(cube->revision(), moved);
}

BOOST_AUTO_TEST_CASE(target_beneath_a_parent_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Group> group = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    group->translation(glm::vec3(0.0F, 2.0F, 0.0F));
    group->rotation(glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0F, 0.0F, 1.0F)));
    group->scale(glm::vec3(2.0F));
    light->translation(glm::vec3(1.0F, 0.0F, 0.0F));
    scene.add(group);
    scene.add(light);
    scene.parent(light->id(), group->id());
    light->selected(true);

    // the pivot and the handles' axes are where the light is in the world
    v3d::editor::Target target = v3d::editor::Target::selected(scene, v3d::editor::SelectMask::Light);
    BOOST_CHECK_SMALL(glm::distance(target.pivot(), glm::vec3(0.0F, 4.0F, 0.0F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(target.axes() * glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F)), 0.0001F);

    // a move along world x moves the light that far in the world, which is half as far
    // along the group's turned y axis
    v3d::editor::Motion motion;
    motion.offset = glm::vec3(1.0F, 0.0F, 0.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    const glm::vec3 moved(scene.evaluate().placed(light->id()).world[3]);
    BOOST_CHECK_SMALL(glm::distance(moved, glm::vec3(1.0F, 4.0F, 0.0F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(light->translation(), glm::vec3(1.0F, -0.5F, 0.0F)), 0.0001F);

    // a turn about world z turns the light about world z too
    motion = v3d::editor::Motion();
    motion.turn = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0F, 0.0F, 1.0F));
    v3d::editor::Target again = v3d::editor::Target::selected(scene, v3d::editor::SelectMask::Light);
    again.move(motion, again.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    const glm::mat4 world = scene.evaluate().placed(light->id()).world;
    BOOST_CHECK_SMALL(glm::distance(glm::vec3(world[3]), glm::vec3(1.0F, 4.0F, 0.0F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(glm::normalize(glm::vec3(world[0])), glm::vec3(-1.0F, 0.0F, 0.0F)), 0.0001F);
}

BOOST_AUTO_TEST_CASE(target_starts_where_shown_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Light> light = boost::make_shared<v3d::editor::Light>();
    scene.add(light);
    v3d::animation::Curve curve;
    curve.set(1.0F, 4.0F);
    scene.curve(light->id(), "translation.x", curve);
    scene.frame(1);
    light->selected(true);

    // a curve puts the light at x 4, though it holds 0, and the handles are where it is shown
    v3d::editor::Target target = v3d::editor::Target::selected(scene, v3d::editor::SelectMask::Light);
    BOOST_CHECK_SMALL(glm::distance(target.pivot(), glm::vec3(4.0F, 0.0F, 0.0F)), 0.0001F);

    // a drag moves it on from there, and the edit shows over the curve until the frame is set
    v3d::editor::Motion motion;
    motion.offset = glm::vec3(0.0F, 1.0F, 0.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    BOOST_CHECK_SMALL(glm::distance(target.pivot(), glm::vec3(4.0F, 1.0F, 0.0F)), 0.0001F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    BOOST_CHECK_SMALL(glm::distance(light->translation(), glm::vec3(4.0F, 2.0F, 0.0F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(glm::vec3(scene.evaluate().placed(light->id()).world[3]), glm::vec3(4.0F, 2.0F, 0.0F)), 0.0001F);
}

BOOST_AUTO_TEST_CASE(target_moves_the_top_once_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Group> group = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Mesh> cube = boost::make_shared<v3d::editor::Mesh>(v3d::editor::create_poly_cube());
    scene.add(group);
    scene.add(cube);
    scene.parent(cube->id(), group->id());
    group->selected(true);
    cube->selected(true);

    // an item selected with its parent moves with the parent, and not again by itself
    v3d::editor::Target target = v3d::editor::Target::selected(scene, v3d::editor::SelectMask::Object);
    BOOST_REQUIRE_EQUAL(target.items().size(), 1U);
    BOOST_CHECK(target.items()[0] == group);
    v3d::editor::Motion motion;
    motion.offset = glm::vec3(2.0F, 0.0F, 0.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    BOOST_CHECK(cube->translation() == glm::vec3(0.0F));
    BOOST_CHECK_SMALL(glm::distance(glm::vec3(scene.evaluate().placed(cube->id()).world[3]), glm::vec3(2.0F, 0.0F, 0.0F)), 0.0001F);

    // a mask that takes only meshes moves the cube, its parent not being one it takes
    const v3d::editor::Target meshes = v3d::editor::Target::selected(scene, v3d::editor::SelectMask::Mesh);
    BOOST_REQUIRE_EQUAL(meshes.items().size(), 1U);
    BOOST_CHECK(meshes.items()[0] == cube);
}

};  // namespace
