#include <boost/test/unit_test.hpp>

#include "../3dtypes/Quaternion.h"

BOOST_AUTO_TEST_CASE( quaternion_test )
{
	v3D::Quaternion quat;

	BOOST_CHECK_EQUAL(quat[0], 0.0f);
	BOOST_CHECK_EQUAL(quat[1], 0.0f);
	BOOST_CHECK_EQUAL(quat[2], 0.0f);
	BOOST_CHECK_EQUAL(quat[3], 1.0f);

	v3D::Quaternion quat2(2.0f, 3.0f, 4.0f, 5.0f);
	BOOST_CHECK_EQUAL(quat2[0], 2.0f);
	BOOST_CHECK_EQUAL(quat2[1], 3.0f);
	BOOST_CHECK_EQUAL(quat2[2], 4.0f);
	BOOST_CHECK_EQUAL(quat2[3], 5.0f);

	float q[] = 
	{
		6.0f,
		7.0f,
		8.0f,
		9.0f
	};
	v3D::Quaternion  quat3(q);
	BOOST_CHECK_EQUAL(quat3[0], 6.0f);
	BOOST_CHECK_EQUAL(quat3[1], 7.0f);
	BOOST_CHECK_EQUAL(quat3[2], 8.0f);
	BOOST_CHECK_EQUAL(quat3[3], 9.0f);

	v3D::Quaternion quat4(quat);
	BOOST_CHECK_EQUAL(quat4[0], 0.0f);
	BOOST_CHECK_EQUAL(quat4[1], 0.0f);
	BOOST_CHECK_EQUAL(quat4[2], 0.0f);
	BOOST_CHECK_EQUAL(quat4[3], 1.0f);

	quat4 = quat2;
	BOOST_CHECK_EQUAL(quat4[0], 2.0f);
	BOOST_CHECK_EQUAL(quat4[1], 3.0f);
	BOOST_CHECK_EQUAL(quat4[2], 4.0f);
	BOOST_CHECK_EQUAL(quat4[3], 5.0f);
}
