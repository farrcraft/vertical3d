/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>

#include "../Factory.h"

namespace {
    /**
     * Writes land in a directory of their own beside the executable, created here rather
     * than committed, so a run never depends on what the last one left behind.
     **/
    struct OutputDirectory {
        OutputDirectory() {
            boost::filesystem::create_directory("data_out");
        }
    };
};  // namespace

BOOST_FIXTURE_TEST_CASE(imagewriter_test, OutputDirectory) {
    boost::shared_ptr<v3d::image::Image> img24 = boost::make_shared<v3d::image::Image>(2, 2, 24);
    for (unsigned int pixel = 0; pixel < 4; ++pixel) {
        (*img24)[pixel * 3 + 0] = 0;
        (*img24)[pixel * 3 + 1] = 0;
        (*img24)[pixel * 3 + 2] = 0xff;
    }

    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    // each lossless format round trips exactly
    const char* lossless[] = { "data_out/test_write.bmp", "data_out/test_write.tga", "data_out/test_write.png" };
    for (const char* filename : lossless) {
        BOOST_TEST_CONTEXT(filename) {
            BOOST_CHECK_EQUAL(factory.write(filename, img24), true);

            boost::shared_ptr<v3d::image::Image> image = factory.read(filename);
            BOOST_REQUIRE(image != nullptr);
            BOOST_CHECK_EQUAL(image->width(), 2u);
            BOOST_CHECK_EQUAL(image->height(), 2u);
            BOOST_CHECK_EQUAL(image->bpp(), 24u);
            BOOST_CHECK_EQUAL((*image)[0], 0);
            BOOST_CHECK_EQUAL((*image)[1], 0);
            BOOST_CHECK_EQUAL((*image)[2], 0xff);
        }
    }

    // jpeg is lossy, so the blue comes back a shade off
    std::string jpgFilename("data_out/test_write.jpg");
    BOOST_CHECK_EQUAL(factory.write(jpgFilename, img24), true);
    boost::shared_ptr<v3d::image::Image> imageJpg = factory.read(jpgFilename);
    BOOST_REQUIRE(imageJpg != nullptr);
    BOOST_CHECK_EQUAL(imageJpg->width(), 2u);
    BOOST_CHECK_EQUAL(imageJpg->height(), 2u);
    BOOST_CHECK_EQUAL(imageJpg->bpp(), 24u);
    BOOST_CHECK_EQUAL((*imageJpg)[0], 0);
    BOOST_CHECK_EQUAL((*imageJpg)[1], 0);
    BOOST_CHECK_GE((*imageJpg)[2], 0xfd);

    // an extension with no writer bound to it is refused rather than guessed at
    BOOST_CHECK_EQUAL(factory.write("data_out/test_write.qwe", img24), false);
}

/**
 * Which way up an image is, once it has been through a writer and a reader.
 *
 * v3d::image::Image holds its rows top down - row 0 is the top of the picture - and the
 * canvas, the texture factory and every consumer downstream of them read it that way. A
 * format whose file layout is bottom up has to be flipped by its reader and its writer, so
 * that the pair agree with Image rather than only with each other. The rows here differ
 * because an image of one colour round trips through a flip unchanged.
 **/
BOOST_FIXTURE_TEST_CASE(imagewriter_orientation_test, OutputDirectory) {
    // two rows of one pixel: red on top, green below
    boost::shared_ptr<v3d::image::Image> image = boost::make_shared<v3d::image::Image>(1, 2, 24);
    (*image)[0] = 0xff; (*image)[1] = 0; (*image)[2] = 0;
    (*image)[3] = 0; (*image)[4] = 0xff; (*image)[5] = 0;

    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    const char* lossless[] = { "data_out/test_orientation.tga", "data_out/test_orientation.png" };
    for (const char* filename : lossless) {
        BOOST_TEST_CONTEXT(filename) {
            BOOST_REQUIRE_EQUAL(factory.write(filename, image), true);

            boost::shared_ptr<v3d::image::Image> read = factory.read(filename);
            BOOST_REQUIRE(read != nullptr);
            BOOST_REQUIRE_EQUAL(read->height(), 2u);
            // the top row is still the red one
            BOOST_CHECK_EQUAL((*read)[0], 0xff);
            BOOST_CHECK_EQUAL((*read)[1], 0);
            BOOST_CHECK_EQUAL((*read)[4], 0xff);
        }
    }
}
