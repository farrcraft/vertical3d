/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <cmath>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

boost::shared_ptr<v3d::editor::Mesh> emptyMesh() {
    return boost::make_shared<v3d::editor::Mesh>(boost::make_shared<v3d::brep::BRep>());
}

/**
 * Where a matrix puts a point.
 **/
glm::vec3 apply(const glm::mat4& matrix, const glm::vec3& point) {
    return glm::vec3(matrix * glm::vec4(point, 1.0F));
}

/**
 * Whether two rotations turn things the same way. A quaternion and its negation do.
 **/
bool sameTurn(const glm::quat& first, const glm::quat& second) {
    return std::abs(glm::dot(first, second)) > 0.9999F;
}

BOOST_AUTO_TEST_CASE(evaluation_origin_test) {
    // an item no evaluation saw is at the origin
    const v3d::editor::Evaluation evaluation;
    const v3d::editor::Placed placed = evaluation.placed(1);
    BOOST_CHECK(placed.parent == glm::mat4(1.0F));
    BOOST_CHECK(placed.local == glm::mat4(1.0F));
    BOOST_CHECK(placed.world == glm::mat4(1.0F));
}

BOOST_AUTO_TEST_CASE(evaluation_root_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> mesh = emptyMesh();
    mesh->translation(glm::vec3(1.0F, 2.0F, 3.0F));
    mesh->scale(glm::vec3(2.0F, 1.0F, 1.0F));
    scene.add(mesh);

    // an item with no parent is where its own placement puts it
    const v3d::editor::Placed placed = scene.evaluate().placed(mesh->id());
    BOOST_CHECK(placed.parent == glm::mat4(1.0F));
    BOOST_CHECK(placed.local == mesh->matrix());
    BOOST_CHECK(placed.world == mesh->matrix());
}

BOOST_AUTO_TEST_CASE(evaluation_compose_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Mesh> leaf = emptyMesh();
    const boost::shared_ptr<v3d::editor::Group> inner = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Group> outer = boost::make_shared<v3d::editor::Group>();
    // the children come before their parents in the scene, which the order of composing
    // must not depend on
    scene.add(leaf);
    scene.add(inner);
    scene.add(outer);
    scene.parent(leaf->id(), inner->id());
    scene.parent(inner->id(), outer->id());

    outer->translation(glm::vec3(10.0F, 0.0F, 0.0F));
    outer->rotation(glm::angleAxis(glm::radians(90.0F), glm::vec3(0.0F, 0.0F, 1.0F)));
    inner->translation(glm::vec3(1.0F, 0.0F, 0.0F));
    inner->scale(glm::vec3(2.0F));
    leaf->translation(glm::vec3(1.0F, 0.0F, 0.0F));

    const v3d::editor::Evaluation evaluation = scene.evaluate();

    // the outer group turns +x onto +y, so the inner group is one unit up from it, and the
    // leaf two more, since the inner group doubles everything beneath it
    BOOST_CHECK_SMALL(glm::distance(apply(evaluation.placed(inner->id()).world, glm::vec3(0.0F)),
        glm::vec3(10.0F, 1.0F, 0.0F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(apply(evaluation.placed(leaf->id()).world, glm::vec3(0.0F)),
        glm::vec3(10.0F, 3.0F, 0.0F)), 0.0001F);

    // the world matrix is the parent's times the local one
    const v3d::editor::Placed placed = evaluation.placed(leaf->id());
    BOOST_CHECK(placed.local == leaf->matrix());
    BOOST_CHECK(placed.parent == evaluation.placed(inner->id()).world);
    BOOST_CHECK(placed.world == placed.parent * placed.local);
}

BOOST_AUTO_TEST_CASE(evaluation_is_a_copy_test) {
    v3d::editor::Scene scene;
    const boost::shared_ptr<v3d::editor::Group> group = boost::make_shared<v3d::editor::Group>();
    const boost::shared_ptr<v3d::editor::Mesh> mesh = emptyMesh();
    scene.add(group);
    scene.add(mesh);
    scene.parent(mesh->id(), group->id());

    // an evaluation keeps what it found. A later edit shows only in a later evaluation
    const v3d::editor::Evaluation before = scene.evaluate();
    group->translation(glm::vec3(0.0F, 5.0F, 0.0F));
    BOOST_CHECK(before.placed(mesh->id()).world == glm::mat4(1.0F));
    BOOST_CHECK_SMALL(glm::distance(apply(scene.evaluate().placed(mesh->id()).world, glm::vec3(0.0F)),
        glm::vec3(0.0F, 5.0F, 0.0F)), 0.0001F);

    // and a link taken away puts the mesh back where its own placement says
    scene.parent(mesh->id(), 0);
    BOOST_CHECK(scene.evaluate().placed(mesh->id()).world == mesh->matrix());
}

BOOST_AUTO_TEST_CASE(evaluation_orientation_test) {
    const glm::quat turn = glm::angleAxis(glm::radians(30.0F), glm::normalize(glm::vec3(1.0F, 2.0F, 3.0F)));
    v3d::editor::Group group;
    group.translation(glm::vec3(4.0F, 5.0F, 6.0F));
    group.rotation(turn);
    group.scale(glm::vec3(3.0F, 0.5F, 2.0F));

    // the rotation is recovered from a matrix that also scales unevenly
    BOOST_CHECK(sameTurn(v3d::editor::orientation(group.matrix()), turn));

    // and without the scale, the matrix keeps the translation and the rotation alone
    const glm::mat4 rigid = v3d::editor::withoutScale(group.matrix());
    BOOST_CHECK_SMALL(glm::distance(apply(rigid, glm::vec3(0.0F)), glm::vec3(4.0F, 5.0F, 6.0F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(glm::vec3(rigid * glm::vec4(1.0F, 0.0F, 0.0F, 0.0F)),
        turn * glm::vec3(1.0F, 0.0F, 0.0F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::length(glm::vec3(rigid[1])) - 1.0F, 0.0001F);

    // a sheared matrix still gives a rotation, with its x axis kept
    glm::mat4 sheared(1.0F);
    sheared[1] = glm::vec4(1.0F, 1.0F, 0.0F, 0.0F);
    const glm::quat square = v3d::editor::orientation(sheared);
    BOOST_CHECK(sameTurn(square, glm::quat(1.0F, 0.0F, 0.0F, 0.0F)));
}

};  // namespace
