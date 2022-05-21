#include <boost/test/unit_test.hpp>

#include "../3dtypes/Color4.h"

BOOST_AUTO_TEST_CASE( color4_test )
{
	v3D::Color4 color;

	// test default constructor
	BOOST_CHECK_EQUAL(color[0], 0.0f);
	BOOST_CHECK_EQUAL(color[1], 0.0f);
	BOOST_CHECK_EQUAL(color[2], 0.0f);
	BOOST_CHECK_EQUAL(color[3], 0.0f);

	// test constants map to correct indices
	BOOST_CHECK_EQUAL(v3D::Color4::RED, 0);
	BOOST_CHECK_EQUAL(v3D::Color4::GREEN, 1);
	BOOST_CHECK_EQUAL(v3D::Color4::BLUE, 2);
	BOOST_CHECK_EQUAL(v3D::Color4::ALPHA, 3);

	v3D::Color4 color2(3.0f, 7.0f, 11.0f, 15.0f);

	// test 2nd constructor form
	BOOST_CHECK_EQUAL(color2[v3D::Color4::RED], 3.0f);
	BOOST_CHECK_EQUAL(color2[v3D::Color4::GREEN], 7.0f);
	BOOST_CHECK_EQUAL(color2[v3D::Color4::BLUE], 11.0f);
	BOOST_CHECK_EQUAL(color2[v3D::Color4::ALPHA], 15.0f);

	float values[4] = { 9.0f, 17.0f, 2.0f, 5.0f };

	v3D::Color4 color3(values);

	// test 3rd constructor form
	BOOST_CHECK_EQUAL(color3[v3D::Color4::RED], 9.0f);
	BOOST_CHECK_EQUAL(color3[v3D::Color4::GREEN], 17.0f);
	BOOST_CHECK_EQUAL(color3[v3D::Color4::BLUE], 2.0f);
	BOOST_CHECK_EQUAL(color3[v3D::Color4::ALPHA], 5.0f);

	color3.scale(3.0f);
	// test scaling
	BOOST_CHECK_EQUAL(color3[v3D::Color4::RED], 27.0f);
	BOOST_CHECK_EQUAL(color3[v3D::Color4::GREEN], 51.0f);
	BOOST_CHECK_EQUAL(color3[v3D::Color4::BLUE], 6.0f);
	BOOST_CHECK_EQUAL(color3[v3D::Color4::ALPHA], 15.0f);
}
