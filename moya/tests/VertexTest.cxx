#include <boost/test/unit_test.hpp>

#include "../libmoya/Vertex.h"

BOOST_AUTO_TEST_CASE( vertex_test )
{

    v3D::Moya::Vertex vertex;

    v3D::Vector3 point(7.0f, 12.0f, -13.0f);

    vertex.point(point);

    v3D::Vector3 point2;
    point2 = vertex.point();

    BOOST_CHECK_EQUAL((point == point2), true);

}
