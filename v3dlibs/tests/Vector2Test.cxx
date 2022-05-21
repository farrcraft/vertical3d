#include <boost/test/unit_test.hpp>

#include "../3dtypes/Vector2.h"

float get_vector_value(const v3D::Vector2 vector)
{
	return vector[0];
}

BOOST_AUTO_TEST_CASE( vector2_test )
{
	// test default constructor
	v3D::Vector2 vector;
	BOOST_CHECK_EQUAL(vector[0], 0.0f);
	BOOST_CHECK_EQUAL(vector[1], 0.0f);

	// test second constructor
	v3D::Vector2 vector2(5.0f, 9.0f);
	BOOST_CHECK_EQUAL(vector2[0], 5.0f);
	BOOST_CHECK_EQUAL(vector2[1], 9.0f);

	// test comparison
	BOOST_CHECK_EQUAL((vector == vector2), false);

	// test assignment
	vector[0] = 5.0f;
	vector[1] = 9.0f;
	BOOST_CHECK_EQUAL((vector == vector2), true);

	// test addition
	vector += vector2;
	BOOST_CHECK_EQUAL(vector[0], 10.0f);
	BOOST_CHECK_EQUAL(vector[1], 18.0f);

	// test scaling
	vector2 *= 3.0f;
	BOOST_CHECK_EQUAL(vector2[0], 15.0f);
	BOOST_CHECK_EQUAL(vector2[1], 27.0f);

	// test subtraction
	vector2 -= vector;
	BOOST_CHECK_EQUAL(vector2[0], 5.0f);
	BOOST_CHECK_EQUAL(vector2[1], 9.0f);

	// test negate
	vector = -vector2;
	BOOST_CHECK_EQUAL(vector[0], -5.0f);
	BOOST_CHECK_EQUAL(vector[1], -9.0f);

	// test const accessor
	float fval = get_vector_value(vector);
	BOOST_CHECK_EQUAL(fval, -5.0f);

	std::string val("-5.0, -9.0");
	v3D::Vector2 vector3(val);
	BOOST_CHECK_EQUAL((vector3 == vector), true);

	std::string val2("5.0, bad");
	v3D::Vector2 vector4(val2);
	BOOST_CHECK_EQUAL(vector4[0], 5.0f);
	BOOST_CHECK_EQUAL(vector4[1], 0.0f);
}
