#include <boost/test/unit_test.hpp>

#include "../font/Font2D.h"

BOOST_AUTO_TEST_CASE( font_test )
{
	v3D::Font2D font;
	font.size(12);

	int size = font.size();
	BOOST_CHECK_EQUAL(size, 12);

	font.style(v3D::Font2D::STYLE_BOLD);
	unsigned int style = font.style();
	BOOST_CHECK_EQUAL(style, v3D::Font2D::STYLE_BOLD);

	std::string fontface("Comic Sans");
	font.typeface(fontface);
	std::string checkface = font.typeface();
	BOOST_CHECK_EQUAL((fontface == checkface), true);

	bool result = font.build();
	BOOST_CHECK_EQUAL(result, false);

	v3D::Font2D font2("/usr/share/fonts/corefonts/arial.ttf", 12);
	result = font2.build();
	BOOST_CHECK_EQUAL(result, true);

	std::string text("hello");
	unsigned int w = font2.width(text);
	BOOST_CHECK_EQUAL(w, 27);

	BOOST_CHECK_EQUAL(font2.height(), 13);
	BOOST_CHECK_EQUAL(font2.textureHeight(), .1015625f);

	unsigned char letter = 'w';
	BOOST_CHECK_EQUAL(font2.width(letter), 9);
	
}