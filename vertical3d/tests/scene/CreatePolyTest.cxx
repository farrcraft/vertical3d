/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/AABBox.h>
#include <vertical3d/src/scene/CreatePoly.h>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * Every primitive is a unit across and centred on the origin, so one placed by its
 * transform lands where the transform says rather than where its geometry was built.
 **/
void checkUnitCentred(const boost::shared_ptr<v3d::brep::BRep>& mesh, bool flat) {
    v3d::type::AABBox bound = mesh->bound();
    glm::vec3 min = bound.min();
    glm::vec3 max = bound.max();

    BOOST_CHECK_SMALL(min.x + max.x, 0.0001f);
    BOOST_CHECK_SMALL(min.y + max.y, 0.0001f);
    BOOST_CHECK_SMALL(min.z + max.z, 0.0001f);

    BOOST_CHECK_CLOSE(max.x - min.x, 1.0f, 1.0f);
    BOOST_CHECK_CLOSE(max.z - min.z, 1.0f, 1.0f);
    if (flat) {
        BOOST_CHECK_SMALL(max.y - min.y, 0.0001f);
    } else {
        BOOST_CHECK_CLOSE(max.y - min.y, 1.0f, 1.0f);
    }
}

};  // namespace

BOOST_AUTO_TEST_CASE(create_poly_cube_test) {
    boost::shared_ptr<v3d::brep::BRep> cube = v3d::editor::create_poly_cube();

    BOOST_CHECK_EQUAL(cube->faceCount(), 6u);
    BOOST_CHECK_EQUAL(cube->vertexCount(), 8u);
    BOOST_CHECK_EQUAL(cube->edgeCount(), 24u);
    checkUnitCentred(cube, false);
}

BOOST_AUTO_TEST_CASE(create_poly_plane_test) {
    boost::shared_ptr<v3d::brep::BRep> plane = v3d::editor::create_poly_plane();

    BOOST_CHECK_EQUAL(plane->faceCount(), 1u);
    BOOST_CHECK_EQUAL(plane->vertexCount(), 4u);
    // the plane lies in xz, because up is +y everywhere in the editor
    checkUnitCentred(plane, true);
}

BOOST_AUTO_TEST_CASE(create_poly_cylinder_test) {
    boost::shared_ptr<v3d::brep::BRep> cylinder = v3d::editor::create_poly_cylinder();

    // eight walls and no caps, which is what the extrude and split operations expect
    BOOST_CHECK_EQUAL(cylinder->faceCount(), 8u);
    // the ring closes without repeating its first point
    BOOST_CHECK_EQUAL(cylinder->vertexCount(), 16u);
    checkUnitCentred(cylinder, false);
}

BOOST_AUTO_TEST_CASE(create_poly_cone_test) {
    boost::shared_ptr<v3d::brep::BRep> cone = v3d::editor::create_poly_cone();

    BOOST_CHECK_EQUAL(cone->faceCount(), 8u);
    // eight around the base and the apex
    BOOST_CHECK_EQUAL(cone->vertexCount(), 9u);
    checkUnitCentred(cone, false);
}
