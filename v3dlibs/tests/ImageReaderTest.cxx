#include <boost/test/unit_test.hpp>

#include "../image/ImageFactory.h"

BOOST_AUTO_TEST_CASE( imagereader_test )
{
	v3D::ImageFactory factory;
	boost::shared_ptr<v3D::Image> empty_ptr;

	std::string bmpFilename("data/2x2x24_red.bmp");
	boost::shared_ptr<v3D::Image> image = factory.read(bmpFilename);
	BOOST_CHECK_EQUAL(image->width(), 2);
	BOOST_CHECK_EQUAL(image->height(), 2);
	BOOST_CHECK_EQUAL(image->bpp(), 24);
	BOOST_CHECK_EQUAL((*image)[0], 0xff);
	BOOST_CHECK_EQUAL((*image)[1], 0);
	BOOST_CHECK_EQUAL((*image)[2], 0);

	// test loading bmp file that does not exist
	std::string missingBmpFilename("data/12x13x32_doesnotexist.bmp");
	boost::shared_ptr<v3D::Image> missingBmp = factory.read(missingBmpFilename);
	BOOST_CHECK_EQUAL((missingBmp == empty_ptr), true);

	// test loading non-bmp file as bmp file
	std::string badBmpFilename("data/2x2x24_bad.bmp");
	boost::shared_ptr<v3D::Image> badBmp = factory.read(badBmpFilename);
	BOOST_CHECK_EQUAL((badBmp == empty_ptr), true);

	std::string jpgFilename("data/2x2x24_green.jpg");
	boost::shared_ptr<v3D::Image> jpgImage = factory.read(jpgFilename);
	BOOST_CHECK_EQUAL(jpgImage->width(), 2);
	BOOST_CHECK_EQUAL(jpgImage->height(), 2);
	BOOST_CHECK_EQUAL(jpgImage->bpp(), 24);
	BOOST_CHECK_EQUAL((*jpgImage)[0], 0);
	BOOST_CHECK_EQUAL((*jpgImage)[1], 0xff);
	BOOST_CHECK_EQUAL((*jpgImage)[2], 0x1);

	// test loading jpeg file that does not exist
	std::string missingJpgFilename("data/12x13x32_doesnotexist.jpg");
	boost::shared_ptr<v3D::Image> missingJpg = factory.read(missingJpgFilename);
	BOOST_CHECK_EQUAL((missingJpg == empty_ptr), true);

	// test loading non-jpg file as jpg file
	std::string badJpgFilename("data/2x2x24_bad.jpg");
	boost::shared_ptr<v3D::Image> badJpg = factory.read(badJpgFilename);
	BOOST_CHECK_EQUAL((badJpg == empty_ptr), true);

	// test loading unrecognized file format
	std::string badFormatFilename("data/8x6x16.qwe");
	boost::shared_ptr<v3D::Image> unrecognizedFormat = factory.read(badFormatFilename);
	BOOST_CHECK_EQUAL((unrecognizedFormat == empty_ptr), true);

	// test loading png file that does not exist
	std::string missingPngFilename("data/12x13x32_doesnotexist.png");
	boost::shared_ptr<v3D::Image> missingPng = factory.read(missingPngFilename);
	BOOST_CHECK_EQUAL((missingPng == empty_ptr), true);

	// test loading non-png file as png file
	std::string badPngFilename("data/2x2x24_bad.png");
	boost::shared_ptr<v3D::Image> badPng = factory.read(badPngFilename);
	BOOST_CHECK_EQUAL((badPng == empty_ptr), true);

	std::string pngFilename("data/2x2x24_blue.png");
	boost::shared_ptr<v3D::Image> pngImage = factory.read(pngFilename);
	BOOST_CHECK_EQUAL(pngImage->width(), 2);
	BOOST_CHECK_EQUAL(pngImage->height(), 2);
	BOOST_CHECK_EQUAL(pngImage->bpp(), 24);
	BOOST_CHECK_EQUAL((*pngImage)[0], 0);
	BOOST_CHECK_EQUAL((*pngImage)[1], 0x0);
	BOOST_CHECK_EQUAL((*pngImage)[2], 0xff);

	// test loading tga file that does not exist
	std::string missingTgaFilename("data/12x13x32_doesnotexist.tga");
	boost::shared_ptr<v3D::Image> missingTga = factory.read(missingTgaFilename);
	BOOST_CHECK_EQUAL((missingTga == empty_ptr), true);

	// test loading non-tga file as tga file
	std::string badTgaFilename("data/2x2x24_bad.tga");
	boost::shared_ptr<v3D::Image> badTga = factory.read(badTgaFilename);
	BOOST_CHECK_EQUAL((badTga == empty_ptr), true);

	std::string tgaFilename("data/2x2x24_blue.tga");
	boost::shared_ptr<v3D::Image> tgaImage = factory.read(tgaFilename);
	BOOST_CHECK_EQUAL(tgaImage->width(), 2);
	BOOST_CHECK_EQUAL(tgaImage->height(), 2);
	BOOST_CHECK_EQUAL(tgaImage->bpp(), 24);
	BOOST_CHECK_EQUAL((*tgaImage)[0], 0);
	BOOST_CHECK_EQUAL((*tgaImage)[1], 0x0);
	BOOST_CHECK_EQUAL((*tgaImage)[2], 0xff);
}
