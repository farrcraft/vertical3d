#include <boost/test/unit_test.hpp>

#include <sstream>

#include "../3dtypes/Matrix4.h"

BOOST_AUTO_TEST_CASE( matrix4_test )
{
	// test default constructor
	v3D::Matrix4 matrix;
	BOOST_CHECK_EQUAL(matrix[0], 1.0f);
	BOOST_CHECK_EQUAL(matrix[1], 0.0f);
	BOOST_CHECK_EQUAL(matrix[2], 0.0f);
	BOOST_CHECK_EQUAL(matrix[3], 0.0f);

	BOOST_CHECK_EQUAL(matrix[4], 0.0f);
	BOOST_CHECK_EQUAL(matrix[5], 1.0f);
	BOOST_CHECK_EQUAL(matrix[6], 0.0f);
	BOOST_CHECK_EQUAL(matrix[7], 0.0f);

	BOOST_CHECK_EQUAL(matrix[8], 0.0f);
	BOOST_CHECK_EQUAL(matrix[9], 0.0f);
	BOOST_CHECK_EQUAL(matrix[10], 1.0f);
	BOOST_CHECK_EQUAL(matrix[11], 0.0f);

	BOOST_CHECK_EQUAL(matrix[12], 0.0f);
	BOOST_CHECK_EQUAL(matrix[13], 0.0f);
	BOOST_CHECK_EQUAL(matrix[14], 0.0f);
	BOOST_CHECK_EQUAL(matrix[15], 1.0f);

	// test zero
	matrix.zero();
	BOOST_CHECK_EQUAL(matrix[0], 0.0f);
	BOOST_CHECK_EQUAL(matrix[1], 0.0f);
	BOOST_CHECK_EQUAL(matrix[2], 0.0f);
	BOOST_CHECK_EQUAL(matrix[3], 0.0f);

	BOOST_CHECK_EQUAL(matrix[4], 0.0f);
	BOOST_CHECK_EQUAL(matrix[5], 0.0f);
	BOOST_CHECK_EQUAL(matrix[6], 0.0f);
	BOOST_CHECK_EQUAL(matrix[7], 0.0f);

	BOOST_CHECK_EQUAL(matrix[8], 0.0f);
	BOOST_CHECK_EQUAL(matrix[9], 0.0f);
	BOOST_CHECK_EQUAL(matrix[10], 0.0f);
	BOOST_CHECK_EQUAL(matrix[11], 0.0f);

	BOOST_CHECK_EQUAL(matrix[12], 0.0f);
	BOOST_CHECK_EQUAL(matrix[13], 0.0f);
	BOOST_CHECK_EQUAL(matrix[14], 0.0f);
	BOOST_CHECK_EQUAL(matrix[15], 0.0f);

	// test single value constructor
	v3D::Matrix4 matrix2(0.0f);
	BOOST_CHECK_EQUAL(matrix2[0], 0.0f);
	BOOST_CHECK_EQUAL(matrix2[1], 0.0f);
	BOOST_CHECK_EQUAL(matrix2[2], 0.0f);
	BOOST_CHECK_EQUAL(matrix2[3], 0.0f);

	BOOST_CHECK_EQUAL(matrix2[4], 0.0f);
	BOOST_CHECK_EQUAL(matrix2[5], 0.0f);
	BOOST_CHECK_EQUAL(matrix2[6], 0.0f);
	BOOST_CHECK_EQUAL(matrix2[7], 0.0f);

	BOOST_CHECK_EQUAL(matrix2[8], 0.0f);
	BOOST_CHECK_EQUAL(matrix2[9], 0.0f);
	BOOST_CHECK_EQUAL(matrix2[10], 0.0f);
	BOOST_CHECK_EQUAL(matrix2[11], 0.0f);

	BOOST_CHECK_EQUAL(matrix2[12], 0.0f);
	BOOST_CHECK_EQUAL(matrix2[13], 0.0f);
	BOOST_CHECK_EQUAL(matrix2[14], 0.0f);
	BOOST_CHECK_EQUAL(matrix2[15], 0.0f);

	// test comparison - zero == zero
	BOOST_CHECK_EQUAL((matrix2 == matrix), true);

	// test comparison - identity != zero
	matrix.identity();
	BOOST_CHECK_EQUAL((matrix != matrix2), true);

	// test default constructor == identity matrix
	v3D::Matrix4 matrix3;
	BOOST_CHECK_EQUAL((matrix == matrix3), true);

	// test assignment - assign identity to zero matrix
	matrix2 = matrix3;
	BOOST_CHECK_EQUAL((matrix2 == matrix3), true);

	float test_values[] = 
	{
		1.0f, 2.0f, 3.0f, 4.0f,
		5.0f, 6.0f, 7.0f, 8.0f,
		9.0f, 10.0f, 11.0f, 12.0f,
		13.0f, 14.0f, 15.0f, 16.0f
	};

	// test 1-dimensional array constructor
	v3D::Matrix4 matrix4(test_values);
	BOOST_CHECK_EQUAL(matrix4[0], 1.0f);
	BOOST_CHECK_EQUAL(matrix4[1], 2.0f);
	BOOST_CHECK_EQUAL(matrix4[2], 3.0f);
	BOOST_CHECK_EQUAL(matrix4[3], 4.0f);

	BOOST_CHECK_EQUAL(matrix4[4], 5.0f);
	BOOST_CHECK_EQUAL(matrix4[5], 6.0f);
	BOOST_CHECK_EQUAL(matrix4[6], 7.0f);
	BOOST_CHECK_EQUAL(matrix4[7], 8.0f);

	BOOST_CHECK_EQUAL(matrix4[8], 9.0f);
	BOOST_CHECK_EQUAL(matrix4[9], 10.0f);
	BOOST_CHECK_EQUAL(matrix4[10], 11.0f);
	BOOST_CHECK_EQUAL(matrix4[11], 12.0f);

	BOOST_CHECK_EQUAL(matrix4[12], 13.0f);
	BOOST_CHECK_EQUAL(matrix4[13], 14.0f);
	BOOST_CHECK_EQUAL(matrix4[14], 15.0f);
	BOOST_CHECK_EQUAL(matrix4[15], 16.0f);

	// test unary negate operator
	matrix4 = -matrix4;
	BOOST_CHECK_EQUAL(matrix4[0], -1.0f);
	BOOST_CHECK_EQUAL(matrix4[1], -2.0f);
	BOOST_CHECK_EQUAL(matrix4[2], -3.0f);
	BOOST_CHECK_EQUAL(matrix4[3], -4.0f);

	BOOST_CHECK_EQUAL(matrix4[4], -5.0f);
	BOOST_CHECK_EQUAL(matrix4[5], -6.0f);
	BOOST_CHECK_EQUAL(matrix4[6], -7.0f);
	BOOST_CHECK_EQUAL(matrix4[7], -8.0f);

	BOOST_CHECK_EQUAL(matrix4[8], -9.0f);
	BOOST_CHECK_EQUAL(matrix4[9], -10.0f);
	BOOST_CHECK_EQUAL(matrix4[10], -11.0f);
	BOOST_CHECK_EQUAL(matrix4[11], -12.0f);

	BOOST_CHECK_EQUAL(matrix4[12], -13.0f);
	BOOST_CHECK_EQUAL(matrix4[13], -14.0f);
	BOOST_CHECK_EQUAL(matrix4[14], -15.0f);
	BOOST_CHECK_EQUAL(matrix4[15], -16.0f);

	// test negate method
	v3D::Matrix4 matrix5(test_values);
	matrix5.neg();
	BOOST_CHECK_EQUAL((matrix4 == matrix5), true);


	// test copy constructor
	v3D::Matrix4 matrix6(matrix5);
	BOOST_CHECK_EQUAL((matrix6 == matrix5), true);

	// test clone
	matrix6.clone(matrix4);
	BOOST_CHECK_EQUAL((matrix6 == matrix4), true);

	// test subtraction operator
	matrix6 = matrix5 - matrix4;
	matrix5.zero();
	BOOST_CHECK_EQUAL((matrix6 == matrix5), true);

	// test big constructor
	v3D::Matrix4 matrix7(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f);
	v3D::Matrix4 matrix8(test_values);
	BOOST_CHECK_EQUAL((matrix7 == matrix8), true);

	// test float multiplication operator
	matrix7 *= 3.0f;
	BOOST_CHECK_EQUAL(matrix7[0], 3.0f);
	BOOST_CHECK_EQUAL(matrix7[1], 6.0f);
	BOOST_CHECK_EQUAL(matrix7[2], 9.0f);
	BOOST_CHECK_EQUAL(matrix7[3], 12.0f);

	BOOST_CHECK_EQUAL(matrix7[4], 15.0f);
	BOOST_CHECK_EQUAL(matrix7[5], 18.0f);
	BOOST_CHECK_EQUAL(matrix7[6], 21.0f);
	BOOST_CHECK_EQUAL(matrix7[7], 24.0f);

	BOOST_CHECK_EQUAL(matrix7[8], 27.0f);
	BOOST_CHECK_EQUAL(matrix7[9], 30.0f);
	BOOST_CHECK_EQUAL(matrix7[10], 33.0f);
	BOOST_CHECK_EQUAL(matrix7[11], 36.0f);

	BOOST_CHECK_EQUAL(matrix7[12], 39.0f);
	BOOST_CHECK_EQUAL(matrix7[13], 42.0f);
	BOOST_CHECK_EQUAL(matrix7[14], 45.0f);
	BOOST_CHECK_EQUAL(matrix7[15], 48.0f);

	// test multiplication operator
	matrix5 = matrix8 * 3.0f;
	BOOST_CHECK_EQUAL((matrix5 == matrix7), true);

	// test scale
	matrix7.scale(3.0f, 2.0f, 5.0f);
	BOOST_CHECK_EQUAL(matrix7[0], 9.0f);
	BOOST_CHECK_EQUAL(matrix7[1], 6.0f);
	BOOST_CHECK_EQUAL(matrix7[2], 9.0f);
	BOOST_CHECK_EQUAL(matrix7[3], 12.0f);

	BOOST_CHECK_EQUAL(matrix7[4], 15.0f);
	BOOST_CHECK_EQUAL(matrix7[5], 36.0f);
	BOOST_CHECK_EQUAL(matrix7[6], 21.0f);
	BOOST_CHECK_EQUAL(matrix7[7], 24.0f);

	BOOST_CHECK_EQUAL(matrix7[8], 27.0f);
	BOOST_CHECK_EQUAL(matrix7[9], 30.0f);
	BOOST_CHECK_EQUAL(matrix7[10], 165.0f);
	BOOST_CHECK_EQUAL(matrix7[11], 36.0f);

	BOOST_CHECK_EQUAL(matrix7[12], 39.0f);
	BOOST_CHECK_EQUAL(matrix7[13], 42.0f);
	BOOST_CHECK_EQUAL(matrix7[14], 45.0f);
	BOOST_CHECK_EQUAL(matrix7[15], 48.0f);

	// test abs
	matrix7[1] = -21.0f;
	matrix7[13] = -33.0f;
	matrix7[5] = -12.0f;
	matrix7.abs();
	BOOST_CHECK_EQUAL(matrix7[0], 9.0f);
	BOOST_CHECK_EQUAL(matrix7[1], 21.0f);
	BOOST_CHECK_EQUAL(matrix7[2], 9.0f);
	BOOST_CHECK_EQUAL(matrix7[3], 12.0f);

	BOOST_CHECK_EQUAL(matrix7[4], 15.0f);
	BOOST_CHECK_EQUAL(matrix7[5], 12.0f);
	BOOST_CHECK_EQUAL(matrix7[6], 21.0f);
	BOOST_CHECK_EQUAL(matrix7[7], 24.0f);

	BOOST_CHECK_EQUAL(matrix7[8], 27.0f);
	BOOST_CHECK_EQUAL(matrix7[9], 30.0f);
	BOOST_CHECK_EQUAL(matrix7[10], 165.0f);
	BOOST_CHECK_EQUAL(matrix7[11], 36.0f);

	BOOST_CHECK_EQUAL(matrix7[12], 39.0f);
	BOOST_CHECK_EQUAL(matrix7[13], 33.0f);
	BOOST_CHECK_EQUAL(matrix7[14], 45.0f);
	BOOST_CHECK_EQUAL(matrix7[15], 48.0f);

	// test addition operator
	matrix8 = matrix7 + matrix5;
	BOOST_CHECK_EQUAL(matrix8[0], 12.0f);
	BOOST_CHECK_EQUAL(matrix8[1], 27.0f);
	BOOST_CHECK_EQUAL(matrix8[2], 18.0f);
	BOOST_CHECK_EQUAL(matrix8[3], 24.0f);

	BOOST_CHECK_EQUAL(matrix8[4], 30.0f);
	BOOST_CHECK_EQUAL(matrix8[5], 30.0f);
	BOOST_CHECK_EQUAL(matrix8[6], 42.0f);
	BOOST_CHECK_EQUAL(matrix8[7], 48.0f);

	BOOST_CHECK_EQUAL(matrix8[8], 54.0f);
	BOOST_CHECK_EQUAL(matrix8[9], 60.0f);
	BOOST_CHECK_EQUAL(matrix8[10], 198.0f);
	BOOST_CHECK_EQUAL(matrix8[11], 72.0f);

	BOOST_CHECK_EQUAL(matrix8[12], 78.0f);
	BOOST_CHECK_EQUAL(matrix8[13], 75.0f);
	BOOST_CHECK_EQUAL(matrix8[14], 90.0f);
	BOOST_CHECK_EQUAL(matrix8[15], 96.0f);

	// test stringifying
	std::stringstream stream;
	std::string string_rep;
	stream << 
		"[12, 27, 18, 24]" << std::endl << 
		"[30, 30, 42, 48]" << std::endl << 
		"[54, 60, 198, 72]" << std::endl << 
		"[78, 75, 90, 96]" << std::endl;
	string_rep = stream.str();

	std::string check_str = matrix8.str();
	BOOST_CHECK_EQUAL((string_rep == check_str), true);
}
