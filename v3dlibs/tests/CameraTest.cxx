#include <boost/test/unit_test.hpp>

#include "../3dtypes/Camera.h"

BOOST_AUTO_TEST_CASE( camera_test )
{
    v3D::Camera camera;

    // orthographic
    camera.orthographic(true);
    bool ortho = camera.orthographic();
    BOOST_CHECK_EQUAL(ortho, true);
    camera.orthographic(false);
    ortho = camera.orthographic();
    BOOST_CHECK_EQUAL(ortho, false);

    // unproject

    // project

    // createProjection

    // createView

    // projection

    // view

    // pan

    // tilt

    // zoom

    // dolly

    // truck

    // pedestal

    // orthoFactor

    // rotate


}