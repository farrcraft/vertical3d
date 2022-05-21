#include <boost/test/unit_test.hpp>

#include "../image/ImageFactory.h"

BOOST_AUTO_TEST_CASE( imagewriter_test )
{
	boost::shared_ptr<v3D::Image> img24(new v3D::Image(2, 2, 24));
	(*img24)[0] = 0;
	(*img24)[1] = 0;
	(*img24)[2] = 0xff;

	(*img24)[3] = 0;
	(*img24)[4] = 0;
	(*img24)[5] = 0xff;

	(*img24)[6] = 0;
	(*img24)[7] = 0;
	(*img24)[8] = 0xff;

	(*img24)[9] = 0;
	(*img24)[10] = 0;
	(*img24)[11] = 0xff;

	v3D::ImageFactory factory;

	std::string bmpFilename("data_out/test_write.bmp");
	bool result = factory.write(bmpFilename, img24);
	BOOST_CHECK_EQUAL(result, true);

	// verify write
	boost::shared_ptr<v3D::Image> image = factory.read(bmpFilename);
	BOOST_CHECK_EQUAL(image->width(), 2);
	BOOST_CHECK_EQUAL(image->height(), 2);
	BOOST_CHECK_EQUAL(image->bpp(), 24);
	BOOST_CHECK_EQUAL((*image)[0], 0);
	BOOST_CHECK_EQUAL((*image)[1], 0);
	BOOST_CHECK_EQUAL((*image)[2], 0xff);

	std::string tgaFilename("data_out/test_write.tga");
	result = factory.write(tgaFilename, img24);
	BOOST_CHECK_EQUAL(result, true);

	// verify write
	boost::shared_ptr<v3D::Image> imageTga = factory.read(tgaFilename);
	BOOST_CHECK_EQUAL(imageTga->width(), 2);
	BOOST_CHECK_EQUAL(imageTga->height(), 2);
	BOOST_CHECK_EQUAL(imageTga->bpp(), 24);
	BOOST_CHECK_EQUAL((*imageTga)[0], 0);
	BOOST_CHECK_EQUAL((*imageTga)[1], 0);
	BOOST_CHECK_EQUAL((*imageTga)[2], 0xff);

	std::string pngFilename("data_out/test_write.png");
	result = factory.write(pngFilename, img24);
	BOOST_CHECK_EQUAL(result, true);

	// verify write
	boost::shared_ptr<v3D::Image> imagePng = factory.read(pngFilename);
	BOOST_CHECK_EQUAL(imagePng->width(), 2);
	BOOST_CHECK_EQUAL(imagePng->height(), 2);
	BOOST_CHECK_EQUAL(imagePng->bpp(), 24);
	BOOST_CHECK_EQUAL((*imagePng)[0], 0);
	BOOST_CHECK_EQUAL((*imagePng)[1], 0);
	BOOST_CHECK_EQUAL((*imagePng)[2], 0xff);

	std::string jpgFilename("data_out/test_write.jpg");
	result = factory.write(jpgFilename, img24);
	BOOST_CHECK_EQUAL(result, true);

	// verify write
	boost::shared_ptr<v3D::Image> imageJpg = factory.read(jpgFilename);
	BOOST_CHECK_EQUAL(imageJpg->width(), 2);
	BOOST_CHECK_EQUAL(imageJpg->height(), 2);
	BOOST_CHECK_EQUAL(imageJpg->bpp(), 24);
	BOOST_CHECK_EQUAL((*imageJpg)[0], 0);
	BOOST_CHECK_EQUAL((*imageJpg)[1], 0);
	BOOST_CHECK_EQUAL((*imageJpg)[2], 0xfe);

}
