/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

// jpeglib.h names FILE in its stdio helpers without including stdio itself

#include <api/image/Factory.h>

#include <stdio.h>
#include <jpeglib.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/operations.hpp>

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

/**
 * The jpeg pair is pinned against libjpeg directly rather than against itself.
 *
 * A round trip cannot see an orientation fault: a writer and a reader that both reverse
 * their rows return the image they were given. So the writer is checked by decoding what it
 * produced, and the reader by handing it a file encoded here, each with the other half of
 * the pair left out of it.
 *
 * The image is two bands rather than two rows because jpeg is lossy and its 8x8 blocks
 * bleed across a boundary; a band per half of an 8 row image leaves the first and last
 * scanlines unambiguous.
 **/
BOOST_FIXTURE_TEST_CASE(imagewriter_jpeg_orientation_test, OutputDirectory) {
    const unsigned int side = 8;
    boost::shared_ptr<v3d::image::Image> image =
        boost::make_shared<v3d::image::Image>(side, side, 24);
    for (unsigned int row = 0; row < side; ++row) {
        for (unsigned int column = 0; column < side; ++column) {
            const unsigned int pixel = (row * side + column) * 3;
            // red over the top half, green over the bottom
            (*image)[pixel + 0] = static_cast<unsigned char>(row < side / 2 ? 0xff : 0);
            (*image)[pixel + 1] = static_cast<unsigned char>(row < side / 2 ? 0 : 0xff);
            (*image)[pixel + 2] = 0;
        }
    }

    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    const char* written = "data_out/test_orientation_written.jpg";
    BOOST_REQUIRE_EQUAL(factory.write(written, image), true);

    // what the writer put in the file: scanline 0 has to be the red band
    {
        FILE* fp = nullptr;
        BOOST_REQUIRE_EQUAL(fopen_s(&fp, written, "rb"), 0);
        jpeg_decompress_struct cinfo;
        jpeg_error_mgr jerr;
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, fp);
        jpeg_read_header(&cinfo, TRUE);
        jpeg_start_decompress(&cinfo);

        std::vector<unsigned char> scanline(static_cast<size_t>(cinfo.output_width) * cinfo.output_components);
        unsigned char* rows[1] = { scanline.data() };
        jpeg_read_scanlines(&cinfo, rows, 1);
        BOOST_CHECK_GT(scanline[0], scanline[1]);

        while (cinfo.output_scanline < cinfo.output_height) {
            jpeg_read_scanlines(&cinfo, rows, 1);
        }
        // and the last one the green band
        BOOST_CHECK_GT(scanline[1], scanline[0]);

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);
        fclose(fp);
    }

    // and what the reader makes of a file whose first scanline is red: row 0 of the buffer
    const char* encoded = "data_out/test_orientation_encoded.jpg";
    {
        FILE* fp = nullptr;
        BOOST_REQUIRE_EQUAL(fopen_s(&fp, encoded, "wb"), 0);
        jpeg_compress_struct cinfo;
        jpeg_error_mgr jerr;
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, fp);
        cinfo.image_width = side;
        cinfo.image_height = side;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
        jpeg_set_defaults(&cinfo);
        jpeg_start_compress(&cinfo, TRUE);

        std::vector<unsigned char> scanline(static_cast<size_t>(side) * 3);
        while (cinfo.next_scanline < cinfo.image_height) {
            const bool top = cinfo.next_scanline < side / 2;
            for (unsigned int column = 0; column < side; ++column) {
                scanline[column * 3 + 0] = static_cast<unsigned char>(top ? 0xff : 0);
                scanline[column * 3 + 1] = static_cast<unsigned char>(top ? 0 : 0xff);
                scanline[column * 3 + 2] = 0;
            }
            unsigned char* rows[1] = { scanline.data() };
            jpeg_write_scanlines(&cinfo, rows, 1);
        }
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        fclose(fp);
    }

    boost::shared_ptr<v3d::image::Image> read = factory.read(encoded);
    BOOST_REQUIRE(read != nullptr);
    BOOST_REQUIRE_EQUAL(read->height(), side);
    // row 0 is the top of the picture, which is the red band
    BOOST_CHECK_GT((*read)[0], (*read)[1]);
    const unsigned int last = (side - 1) * side * 3;
    BOOST_CHECK_GT((*read)[last + 1], (*read)[last]);
}

/**
 * A greyscale jpeg is decoded as RGB.
 *
 * The reader builds a 24 bit Image and copies three bytes a pixel, which is only true of the
 * scanline if the decoder was asked for RGB. Left in its own colour space a greyscale file
 * yields one component per pixel, and the third byte of the last pixel is off the end of the
 * row. The ramp is what makes that visible: reading three bytes out of a single component
 * row takes the next two pixels' greys as green and blue, so the channels come apart.
 **/
BOOST_FIXTURE_TEST_CASE(imagereader_greyscale_jpeg_test, OutputDirectory) {
    const unsigned int side = 8;
    const char* greyscale = "data_out/test_greyscale.jpg";
    {
        FILE* fp = nullptr;
        BOOST_REQUIRE_EQUAL(fopen_s(&fp, greyscale, "wb"), 0);
        jpeg_compress_struct cinfo;
        jpeg_error_mgr jerr;
        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, fp);
        cinfo.image_width = side;
        cinfo.image_height = side;
        cinfo.input_components = 1;
        cinfo.in_color_space = JCS_GRAYSCALE;
        jpeg_set_defaults(&cinfo);
        // lossless enough that a channel that came from the wrong pixel cannot be mistaken
        // for compression noise
        jpeg_set_quality(&cinfo, 100, TRUE);
        jpeg_start_compress(&cinfo, TRUE);

        std::vector<unsigned char> scanline(side);
        while (cinfo.next_scanline < cinfo.image_height) {
            // a ramp across the row, so neighbouring pixels differ
            for (unsigned int column = 0; column < side; ++column) {
                scanline[column] = static_cast<unsigned char>(column * 0x20);
            }
            unsigned char* rows[1] = { scanline.data() };
            jpeg_write_scanlines(&cinfo, rows, 1);
        }
        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        fclose(fp);
    }

    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    boost::shared_ptr<v3d::image::Image> read = factory.read(greyscale);
    BOOST_REQUIRE(read != nullptr);
    BOOST_REQUIRE_EQUAL(read->width(), side);
    BOOST_REQUIRE_EQUAL(read->height(), side);

    // grey replicated into all three channels, for every pixel including the last of a row
    for (unsigned int pixel = 0; pixel < side * side; ++pixel) {
        BOOST_TEST_CONTEXT("pixel " << pixel) {
            BOOST_CHECK_EQUAL((*read)[pixel * 3 + 0], (*read)[pixel * 3 + 1]);
            BOOST_CHECK_EQUAL((*read)[pixel * 3 + 1], (*read)[pixel * 3 + 2]);
        }
    }
}

/**
 * Every writer encodes three channels or four, and each reads a row as though it held that
 * many: the bmp writer takes src[column * channels + 2] per pixel, which on a one channel
 * image is two bytes past where that pixel ends. So a grey image is refused, and refused
 * before the file is opened rather than after an empty one is left behind.
 **/
BOOST_FIXTURE_TEST_CASE(imagewriter_refuses_a_grey_image, OutputDirectory) {
    boost::shared_ptr<v3d::image::Image> grey = boost::make_shared<v3d::image::Image>(2, 2, 8);
    BOOST_REQUIRE((grey->format() == v3d::image::Image::Format::Grey));

    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    const char* const names[] = {
        "data_out/grey.png", "data_out/grey.bmp", "data_out/grey.jpg", "data_out/grey.tga"
    };
    for (const char* const name : names) {
        boost::filesystem::remove(name);
        BOOST_CHECK_EQUAL(factory.write(name, grey), false);
        BOOST_CHECK_EQUAL(boost::filesystem::exists(name), false);
    }
}
