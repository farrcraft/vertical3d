#include <boost/test/unit_test.hpp>

#include "../image/Image.h"

BOOST_AUTO_TEST_CASE( image_test )
{
	v3D::Image img24(2, 4, 24);
	BOOST_CHECK_EQUAL((img24.format() == v3D::Image::FORMAT_RGB), true);

	v3D::Image img32(1, 5, 32);
	BOOST_CHECK_EQUAL((img32.format() == v3D::Image::FORMAT_RGBA), true);

	BOOST_CHECK_EQUAL((img24.width() == 2), true);
	BOOST_CHECK_EQUAL((img24.height() == 4), true);
	BOOST_CHECK_EQUAL((img24.bpp() == 24), true);

	img32.width(3);
	BOOST_CHECK_EQUAL((img32.width() == 3), true);

	img32.height(7);
	BOOST_CHECK_EQUAL((img32.height() == 7), true);

	img32.bpp(24);
	BOOST_CHECK_EQUAL((img32.bpp() == 24), true);

	v3D::Image img(12);
	img.width(2);
	img.height(2);
	img.bpp(24);

	img[0] = 3;
	BOOST_CHECK_EQUAL(img[0], 3);
	unsigned char pixel = img[3];
	BOOST_CHECK_EQUAL(pixel, 0);
}
