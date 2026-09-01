/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../Factory.h"

BOOST_AUTO_TEST_CASE(imagereader_test) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);
    boost::shared_ptr<v3d::image::Image> empty_ptr;

    std::string bmpFilename("data/2x2x24_red.bmp");
    boost::shared_ptr<v3d::image::Image> image = factory.read(bmpFilename);
    BOOST_REQUIRE(image != empty_ptr);
    BOOST_CHECK_EQUAL(image->width(), 2u);
    BOOST_CHECK_EQUAL(image->height(), 2u);
    BOOST_CHECK_EQUAL(image->bpp(), 24u);
    BOOST_CHECK_EQUAL((*image)[0], 0xff);
    BOOST_CHECK_EQUAL((*image)[1], 0);
    BOOST_CHECK_EQUAL((*image)[2], 0);

    // test loading bmp file that does not exist
    std::string missingBmpFilename("data/12x13x32_doesnotexist.bmp");
    boost::shared_ptr<v3d::image::Image> missingBmp = factory.read(missingBmpFilename);
    BOOST_CHECK_EQUAL((missingBmp == empty_ptr), true);

    // test loading non-bmp file as bmp file
    std::string badBmpFilename("data/2x2x24_bad.bmp");
    boost::shared_ptr<v3d::image::Image> badBmp = factory.read(badBmpFilename);
    BOOST_CHECK_EQUAL((badBmp == empty_ptr), true);

    std::string jpgFilename("data/2x2x24_green.jpg");
    boost::shared_ptr<v3d::image::Image> jpgImage = factory.read(jpgFilename);
    BOOST_REQUIRE(jpgImage != empty_ptr);
    BOOST_CHECK_EQUAL(jpgImage->width(), 2u);
    BOOST_CHECK_EQUAL(jpgImage->height(), 2u);
    BOOST_CHECK_EQUAL(jpgImage->bpp(), 24u);
    BOOST_CHECK_EQUAL((*jpgImage)[0], 0);
    BOOST_CHECK_EQUAL((*jpgImage)[1], 0xff);
    BOOST_CHECK_EQUAL((*jpgImage)[2], 0x1);

    // test loading jpeg file that does not exist
    std::string missingJpgFilename("data/12x13x32_doesnotexist.jpg");
    boost::shared_ptr<v3d::image::Image> missingJpg = factory.read(missingJpgFilename);
    BOOST_CHECK_EQUAL((missingJpg == empty_ptr), true);

    // test loading non-jpg file as jpg file
    std::string badJpgFilename("data/2x2x24_bad.jpg");
    boost::shared_ptr<v3d::image::Image> badJpg = factory.read(badJpgFilename);
    BOOST_CHECK_EQUAL((badJpg == empty_ptr), true);

    // test loading unrecognized file format
    std::string badFormatFilename("data/8x6x16.qwe");
    boost::shared_ptr<v3d::image::Image> unrecognizedFormat = factory.read(badFormatFilename);
    BOOST_CHECK_EQUAL((unrecognizedFormat == empty_ptr), true);

    // test loading png file that does not exist
    std::string missingPngFilename("data/12x13x32_doesnotexist.png");
    boost::shared_ptr<v3d::image::Image> missingPng = factory.read(missingPngFilename);
    BOOST_CHECK_EQUAL((missingPng == empty_ptr), true);

    // test loading non-png file as png file
    std::string badPngFilename("data/2x2x24_bad.png");
    boost::shared_ptr<v3d::image::Image> badPng = factory.read(badPngFilename);
    BOOST_CHECK_EQUAL((badPng == empty_ptr), true);

    std::string pngFilename("data/2x2x24_blue.png");
    boost::shared_ptr<v3d::image::Image> pngImage = factory.read(pngFilename);
    BOOST_REQUIRE(pngImage != empty_ptr);
    BOOST_CHECK_EQUAL(pngImage->width(), 2u);
    BOOST_CHECK_EQUAL(pngImage->height(), 2u);
    BOOST_CHECK_EQUAL(pngImage->bpp(), 24u);
    BOOST_CHECK_EQUAL((*pngImage)[0], 0);
    BOOST_CHECK_EQUAL((*pngImage)[1], 0x0);
    BOOST_CHECK_EQUAL((*pngImage)[2], 0xff);

    // test loading tga file that does not exist
    std::string missingTgaFilename("data/12x13x32_doesnotexist.tga");
    boost::shared_ptr<v3d::image::Image> missingTga = factory.read(missingTgaFilename);
    BOOST_CHECK_EQUAL((missingTga == empty_ptr), true);

    // test loading non-tga file as tga file
    std::string badTgaFilename("data/2x2x24_bad.tga");
    boost::shared_ptr<v3d::image::Image> badTga = factory.read(badTgaFilename);
    BOOST_CHECK_EQUAL((badTga == empty_ptr), true);

    std::string tgaFilename("data/2x2x24_blue.tga");
    boost::shared_ptr<v3d::image::Image> tgaImage = factory.read(tgaFilename);
    BOOST_REQUIRE(tgaImage != empty_ptr);
    BOOST_CHECK_EQUAL(tgaImage->width(), 2u);
    BOOST_CHECK_EQUAL(tgaImage->height(), 2u);
    BOOST_CHECK_EQUAL(tgaImage->bpp(), 24u);
    BOOST_CHECK_EQUAL((*tgaImage)[0], 0);
    BOOST_CHECK_EQUAL((*tgaImage)[1], 0x0);
    BOOST_CHECK_EQUAL((*tgaImage)[2], 0xff);
}

BOOST_AUTO_TEST_CASE(imagereader_tga_orientation_test) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    // the fixture is red over green with the origin at the bottom left, so the file holds
    // the green row first. A reader that hands its rows on in file order gets this upside
    // down, and a uniformly coloured fixture cannot tell the difference
    boost::shared_ptr<v3d::image::Image> image = factory.read(std::string("data/2x2x24_rows.tga"));
    BOOST_REQUIRE(image);
    BOOST_CHECK_EQUAL(image->width(), 2u);
    BOOST_CHECK_EQUAL(image->height(), 2u);

    // first row red
    BOOST_CHECK_EQUAL((*image)[0], 0xff);
    BOOST_CHECK_EQUAL((*image)[1], 0);
    BOOST_CHECK_EQUAL((*image)[2], 0);
    // second row green
    BOOST_CHECK_EQUAL((*image)[6], 0);
    BOOST_CHECK_EQUAL((*image)[7], 0xff);
    BOOST_CHECK_EQUAL((*image)[8], 0);
}
