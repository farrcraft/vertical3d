#include <boost/test/unit_test.hpp>

#include "../3dtypes/Vector3.h"

BOOST_AUTO_TEST_CASE( vector3_test )
{
	// test default constructor
	v3D::Vector3 vector;
	BOOST_CHECK_EQUAL(vector[0], 0.0f);
	BOOST_CHECK_EQUAL(vector[1], 0.0f);
	BOOST_CHECK_EQUAL(vector[2], 0.0f);

	v3D::Vector3 vector2(3.0f);
	BOOST_CHECK_EQUAL(vector2[0], 3.0f);
	BOOST_CHECK_EQUAL(vector2[1], 3.0f);
	BOOST_CHECK_EQUAL(vector2[2], 3.0f);

	// test comparison operators
	BOOST_CHECK_EQUAL((vector != vector2), true);

	// test addition
	vector += vector2;

	BOOST_CHECK_EQUAL((vector == vector2), true);

	// test contructor 3rd form
	v3D::Vector3 vector3(3.0f, 7.0f, 11.0f);
	BOOST_CHECK_EQUAL(vector3[0], 3.0f);
	BOOST_CHECK_EQUAL(vector3[1], 7.0f);
	BOOST_CHECK_EQUAL(vector3[2], 11.0f);

	// test scaling
	vector3 *= 3.0f;
	BOOST_CHECK_EQUAL(vector3[0], 9.0f);
	BOOST_CHECK_EQUAL(vector3[1], 21.0f);
	BOOST_CHECK_EQUAL(vector3[2], 33.0f);

	// test negate
	vector2 = -vector3;
	BOOST_CHECK_EQUAL(vector2[0], -9.0f);
	BOOST_CHECK_EQUAL(vector2[1], -21.0f);
	BOOST_CHECK_EQUAL(vector2[2], -33.0f);

	// test division
	vector3 /= 3.0f;
	BOOST_CHECK_EQUAL(vector3[0], 3.0f);
	BOOST_CHECK_EQUAL(vector3[1], 7.0f);
	BOOST_CHECK_EQUAL(vector3[2], 11.0f);

	// test reset
	vector2.reset();
	BOOST_CHECK_EQUAL(vector2[0], 0.0f);
	BOOST_CHECK_EQUAL(vector2[1], 0.0f);
	BOOST_CHECK_EQUAL(vector2[2], 0.0f);

	// test scalar addition
	vector2 = vector3 + 10.0f;
	BOOST_CHECK_EQUAL(vector2[0], 13.0f);
	BOOST_CHECK_EQUAL(vector2[1], 17.0f);
	BOOST_CHECK_EQUAL(vector2[2], 21.0f);

	// test subtraction
	vector = vector2 - vector3;
	BOOST_CHECK_EQUAL(vector[0], 10.0f);
	BOOST_CHECK_EQUAL(vector[1], 10.0f);
	BOOST_CHECK_EQUAL(vector[2], 10.0f);

	// test vector addition
	vector3 = vector + vector2;
	BOOST_CHECK_EQUAL(vector3[0], 23.0f);
	BOOST_CHECK_EQUAL(vector3[1], 27.0f);
	BOOST_CHECK_EQUAL(vector3[2], 31.0f);

	// test scalar multiplication
	vector2 = vector3 * 10.0f;
	BOOST_CHECK_EQUAL(vector2[0], 230.0f);
	BOOST_CHECK_EQUAL(vector2[1], 270.0f);
	BOOST_CHECK_EQUAL(vector2[2], 310.0f);

	// test scalar division
	vector3 = vector2 / 10.0f;
	BOOST_CHECK_EQUAL(vector3[0], 23.0f);
	BOOST_CHECK_EQUAL(vector3[1], 27.0f);
	BOOST_CHECK_EQUAL(vector3[2], 31.0f);

	float dot = vector3.dot(vector);
	BOOST_CHECK_EQUAL(dot, 810.0f);

	float dot2 = vector3.squaredLength();
	BOOST_CHECK_EQUAL(dot2, 2219.0f);

	std::string val("23.0, 27.0, 31.0");
	v3D::Vector3 vector4(val);
	BOOST_CHECK_EQUAL((vector4 == vector3), true);

	// test string conversion
	std::string in_val("23, 27, 31");
	std::string check_val = vector4.str();
	BOOST_CHECK_EQUAL((in_val == check_val), true);

	std::string val2("5.0, 2.0, bad");
	v3D::Vector3 vector5(val2);
	BOOST_CHECK_EQUAL(vector5[0], 5.0f);
	BOOST_CHECK_EQUAL(vector5[1], 2.0f);
	BOOST_CHECK_EQUAL(vector5[2], 0.0f);
}
