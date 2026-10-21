/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/HalfEdge.h>
#include <api/brep/Index.h>
#include <api/brep/Topology.h>
#include <api/brep/Vertex.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/scene/CreatePoly.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/tool/SelectMask.h>

#include <algorithm>
#include <cstddef>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace {

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
    const boost::shared_ptr<v3d::brep::BRep> first = v3d::editor::create_poly_cube();
    const boost::shared_ptr<v3d::brep::BRep> second = v3d::editor::create_poly_cube();
    first->translation(glm::vec3(1.0F, 0.0F, 0.0F));
    second->translation(glm::vec3(3.0F, 2.0F, 0.0F));

    const v3d::editor::Target target = v3d::editor::Target::objects({first, boost::shared_ptr<v3d::brep::BRep>(), second});
    BOOST_CHECK_EQUAL(target.components(), false);
    BOOST_CHECK_EQUAL(target.meshes().size(), 2U);
    BOOST_CHECK_SMALL(glm::distance(target.pivot(), glm::vec3(2.0F, 1.0F, 0.0F)), 0.0001F);

    BOOST_CHECK(v3d::editor::Target().empty());
    BOOST_CHECK_SMALL(glm::length(v3d::editor::Target().pivot()), 0.0001F);
}

BOOST_AUTO_TEST_CASE(target_components_test) {
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();

    // nothing selected is nothing to move
    BOOST_CHECK(v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Face).empty());

    // a face brings its four corners, and every face but the opposite one touches them
    const v3d::brep::Index face = frontFace(*cube);
    BOOST_REQUIRE(face != v3d::brep::INVALID_ID);
    cube->face(face)->selected(true);
    const v3d::editor::Target faces = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Face);
    BOOST_REQUIRE_EQUAL(faces.parts().size(), 1U);
    BOOST_CHECK_EQUAL(faces.parts()[0].vertices.size(), 4U);
    BOOST_CHECK_EQUAL(faces.parts()[0].faces.size(), 5U);
    BOOST_CHECK_SMALL(glm::distance(faces.pivot(), glm::vec3(0.0F, 0.0F, -0.5F)), 0.0001F);
    cube->deselectComponents();

    // two edges that share a corner bring three vertices, the shared one once
    const v3d::brep::Index low = corner(*cube, glm::vec3(-0.5F, -0.5F, -0.5F));
    const v3d::brep::Index right = corner(*cube, glm::vec3(0.5F, -0.5F, -0.5F));
    const v3d::brep::Index up = corner(*cube, glm::vec3(-0.5F, 0.5F, -0.5F));
    const v3d::brep::Index along = edgeBetween(*cube, low, right);
    const v3d::brep::Index across = edgeBetween(*cube, up, low);
    BOOST_REQUIRE(along != v3d::brep::INVALID_ID);
    BOOST_REQUIRE(across != v3d::brep::INVALID_ID);
    cube->edge(along)->selected(true);
    cube->edge(across)->selected(true);
    const v3d::editor::Target edges = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Edge);
    const std::vector<v3d::brep::Index> expected = {low, right, up};
    std::vector<v3d::brep::Index> sorted = expected;
    std::sort(sorted.begin(), sorted.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(edges.parts()[0].vertices.begin(), edges.parts()[0].vertices.end(),
        sorted.begin(), sorted.end());
}

BOOST_AUTO_TEST_CASE(target_components_span_objects_test) {
    const boost::shared_ptr<v3d::brep::BRep> first = v3d::editor::create_poly_cube();
    const boost::shared_ptr<v3d::brep::BRep> second = v3d::editor::create_poly_cube();
    second->translation(glm::vec3(4.0F, 0.0F, 0.0F));
    first->vertex(corner(*first, glm::vec3(0.5F, 0.5F, 0.5F)))->selected(true);
    second->vertex(corner(*second, glm::vec3(0.5F, 0.5F, 0.5F)))->selected(true);

    // the pivot is the mean of the vertices in world space, across both objects
    const v3d::editor::Target target = v3d::editor::Target::components({first, second}, v3d::editor::SelectMask::Vertex);
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
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    // the object is stretched along x, so a world offset is half as far in its own space
    cube->scale(glm::vec3(2.0F, 1.0F, 1.0F));
    const v3d::brep::Index low = corner(*cube, glm::vec3(-0.5F, -0.5F, -0.5F));
    const v3d::brep::Index right = corner(*cube, glm::vec3(0.5F, -0.5F, -0.5F));
    const v3d::brep::Index far = corner(*cube, glm::vec3(0.5F, 0.5F, 0.5F));
    cube->edge(edgeBetween(*cube, low, right))->selected(true);
    cube->edge(edgeBetween(*cube, right, corner(*cube, glm::vec3(0.5F, 0.5F, -0.5F))))->selected(true);

    const v3d::editor::Target target = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Edge);
    v3d::editor::Motion motion;
    motion.offset = glm::vec3(1.0F, 0.0F, 0.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));

    // the shared corner moves once, and the transform is left alone
    BOOST_CHECK_SMALL(glm::distance(cube->vertex(right)->point(), glm::vec3(1.0F, -0.5F, -0.5F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(cube->vertex(low)->point(), glm::vec3(0.0F, -0.5F, -0.5F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(cube->vertex(far)->point(), glm::vec3(0.5F, 0.5F, 0.5F)), 0.0001F);
    BOOST_CHECK(cube->scale() == glm::vec3(2.0F, 1.0F, 1.0F));
    BOOST_CHECK(cube->translation() == glm::vec3(0.0F));

    // every face round a moved corner has its normal from its points again
    for (const v3d::brep::Index face : target.parts()[0].faces) {
        BOOST_CHECK_SMALL(glm::distance(cube->face(face)->normal(), v3d::brep::newellNormal(*cube, face)), 0.0001F);
    }
}

BOOST_AUTO_TEST_CASE(target_turn_components_test) {
    const boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();
    const v3d::brep::Index face = frontFace(*cube);
    cube->face(face)->selected(true);
    const v3d::brep::Index right = corner(*cube, glm::vec3(0.5F, -0.5F, -0.5F));
    const v3d::brep::Index back = corner(*cube, glm::vec3(0.5F, -0.5F, 0.5F));
    const v3d::editor::Target target = v3d::editor::Target::components({cube}, v3d::editor::SelectMask::Face);

    // a quarter turn about z turns the face about its own middle, so it stays in its plane
    // and each corner moves round to the next
    v3d::editor::Motion motion;
    motion.turn = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0F, 0.0F, 1.0F));
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    BOOST_CHECK_SMALL(glm::distance(cube->vertex(right)->point(), glm::vec3(0.5F, 0.5F, -0.5F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(cube->vertex(back)->point(), glm::vec3(0.5F, -0.5F, 0.5F)), 0.0001F);
    BOOST_CHECK_SMALL(glm::distance(cube->face(face)->normal(), glm::vec3(0.0F, 0.0F, -1.0F)), 0.0001F);

    // a stretch past zero stops at the floor rather than turning the face inside out
    motion = v3d::editor::Motion();
    motion.stretch = glm::vec3(-3.0F, 1.0F, 1.0F);
    target.move(motion, target.pivot(), glm::quat(1.0F, 0.0F, 0.0F, 0.0F));
    BOOST_CHECK_SMALL(glm::distance(cube->face(face)->normal(), glm::vec3(0.0F, 0.0F, -1.0F)), 0.0001F);
}

};  // namespace
