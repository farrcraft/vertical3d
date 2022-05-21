#include <boost/test/unit_test.hpp>

#include "../brep/Vertex.h"

BOOST_AUTO_TEST_CASE( vertex_test )
{
	v3D::Vertex vertex;
	v3D::Vector3 vector(1.0f, 2.0f, 3.0f);
	v3D::Vector3 vector2(7.0f, 9.0f, 4.0f);
	vertex.point(vector);
	v3D::Vector3 checkVector = vertex.point();
	BOOST_CHECK_EQUAL((checkVector == vector), true);
	BOOST_CHECK_EQUAL((vertex == vector), true);
	BOOST_CHECK_EQUAL((vertex == vector2), false);

	v3D::Vertex vertex2(vector);
	v3D::Vector3 checkPoint = vertex2.point();
	BOOST_CHECK_EQUAL((checkPoint == vector), true);
	BOOST_CHECK_EQUAL((vertex == vertex2), true);
	v3D::Vertex vertex3(vector2);
	BOOST_CHECK_EQUAL((vertex2 == vertex3), false);

	vertex3.edge(11);
	BOOST_CHECK_EQUAL(vertex3.edge(), 11);
}