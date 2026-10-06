/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Compare.h>
#include <api/image/Factory.h>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * A fixture as its encoded bytes, the form in which an image embedded in another file
 * arrives. There is no path to give a reader.
 **/
std::vector<unsigned char> bytes(const std::string& path) {
    std::ifstream file(path.c_str(), std::ifstream::in | std::ifstream::binary);
    file.seekg(0, std::ifstream::end);
    const std::streamoff length = file.tellg();
    file.seekg(0, std::ifstream::beg);
    std::vector<unsigned char> encoded(length > 0 ? static_cast<std::size_t>(length) : 0u);
    if (!encoded.empty()) {
        file.read(reinterpret_cast<char*>(encoded.data()), length);
    }
    return encoded;
}

};  // namespace

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

/**
 * The path is written in terms of the buffer, so the two have to agree pixel for pixel in
 * every format - an image embedded in a .glb decodes to what the same bytes on disk would.
 **/
BOOST_AUTO_TEST_CASE(imagereader_buffer_matches_the_path) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    const std::string fixtures[][2] = {
        { "data/2x2x24_red.bmp", "bmp" },
        { "data/2x2x24_green.jpg", "jpg" },
        { "data/2x2x24_blue.png", "png" },
        { "data/2x2x24_blue.tga", "tga" },
        { "data/2x2x24_rows.tga", "tga" },
    };

    for (const std::string* fixture : fixtures) {
        const std::string& path = fixture[0];
        const std::string& kind = fixture[1];

        boost::shared_ptr<v3d::image::Image> fromPath = factory.read(path);
        BOOST_REQUIRE_MESSAGE(fromPath, "no image read from " + path);

        const std::vector<unsigned char> encoded = bytes(path);
        BOOST_REQUIRE(!encoded.empty());
        boost::shared_ptr<v3d::image::Image> fromBuffer = factory.read(encoded.data(), encoded.size(), kind);
        BOOST_REQUIRE_MESSAGE(fromBuffer, "no image read from the bytes of " + path);

        const v3d::image::Difference difference = v3d::image::compare(*fromPath, *fromBuffer, 0);
        BOOST_CHECK_MESSAGE(difference.match, path + ": " + difference.description());
    }
}

/**
 * Bytes that are not the format they were said to be decode to nothing rather than to
 * whatever was next to them - a reader walking a buffer has no end of file to stop at.
 **/
BOOST_AUTO_TEST_CASE(imagereader_rejects_bytes_that_are_not_the_format) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    const std::vector<unsigned char> png = bytes("data/2x2x24_blue.png");
    BOOST_REQUIRE(!png.empty());

    BOOST_CHECK(!factory.read(png.data(), png.size(), "tga"));
    BOOST_CHECK(!factory.read(png.data(), png.size(), "qwe"));
    BOOST_CHECK(!factory.read(nullptr, 0, "png"));

    // and a png cut short is refused rather than read as far as it goes
    BOOST_CHECK(!factory.read(png.data(), 16, "png"));
}

BOOST_AUTO_TEST_CASE(imagereader_bmp_refuses_rather_than_throws) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    // a file cut short, at each of the places a length is checked
    const std::vector<unsigned char> bmp = bytes("data/2x2x24_red.bmp");
    BOOST_REQUIRE(bmp.size() > 60);
    BOOST_CHECK(!factory.read(bmp.data(), 8, "bmp"));
    BOOST_CHECK(!factory.read(bmp.data(), 30, "bmp"));
    BOOST_CHECK(!factory.read(bmp.data(), bmp.size() - 4, "bmp"));
    BOOST_CHECK(factory.read(bmp.data(), bmp.size(), "bmp"));
}

BOOST_AUTO_TEST_CASE(imagereader_format_is_the_whole_extension) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    // a name shorter than a three-character extension
    BOOST_CHECK(!factory.read("a"));
    BOOST_CHECK(!factory.read(""));
    // and a name with no extension at all
    BOOST_CHECK(!factory.read("data/2x2x24_red"));
}

/**
 * A 32 bit bmp packed by the masks its header gives, as an external tool writes one, reads
 * with its alpha. This fixture's masks put green in the second byte and leave alpha unused,
 * so it is opaque green.
 **/
BOOST_AUTO_TEST_CASE(imagereader_bmp_reads_32_bits_through_its_masks) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    const boost::shared_ptr<v3d::image::Image> image = factory.read("data/2x2x32_green.bmp");
    BOOST_REQUIRE(image);
    BOOST_CHECK_EQUAL(image->bpp(), 32u);
    for (unsigned int pixel = 0; pixel < 4; ++pixel) {
        BOOST_TEST_CONTEXT("pixel " << pixel) {
            BOOST_CHECK_EQUAL((*image)[pixel * 4], 0);
            BOOST_CHECK_EQUAL((*image)[pixel * 4 + 1], 0xFF);
            BOOST_CHECK_EQUAL((*image)[pixel * 4 + 2], 0);
            BOOST_CHECK_EQUAL((*image)[pixel * 4 + 3], 0xFF);
        }
    }
}

/**
 * A bmp is stored bottom up unless its height is negative. The files are built here byte by
 * byte rather than by the writer, so a reader and a writer that both turned rows over cannot
 * pass by agreeing. The top row is red and the bottom green, at 24 bits and through a palette
 * at 8.
 **/
BOOST_AUTO_TEST_CASE(imagereader_bmp_orientation) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    const auto put16 = [](std::vector<unsigned char>* out, uint32_t value) {
        out->push_back(static_cast<unsigned char>(value));
        out->push_back(static_cast<unsigned char>(value >> 8));
    };
    const auto put32 = [&put16](std::vector<unsigned char>* out, uint32_t value) {
        put16(out, value);
        put16(out, value >> 16);
    };
    // one pixel wide and two high, so every row is a single pixel padded to four bytes
    const auto file = [&](int bits, bool bottomUp, const std::vector<unsigned char>& palette,
        const std::vector<unsigned char>& top, const std::vector<unsigned char>& bottom) {
        std::vector<unsigned char> out;
        const uint32_t offset = 14 + 40 + static_cast<uint32_t>(palette.size());
        put16(&out, 19778);
        put32(&out, offset + 8);
        put32(&out, 0);
        put32(&out, offset);
        put32(&out, 40);
        put32(&out, 1);
        put32(&out, static_cast<uint32_t>(bottomUp ? 2 : -2));
        put16(&out, 1);
        put16(&out, static_cast<uint32_t>(bits));
        for (int field = 0; field < 6; ++field) {
            put32(&out, field == 4 && !palette.empty() ? static_cast<uint32_t>(palette.size() / 4) : 0);
        }
        out.insert(out.end(), palette.begin(), palette.end());
        const std::vector<unsigned char>& first = bottomUp ? bottom : top;
        const std::vector<unsigned char>& second = bottomUp ? top : bottom;
        for (const std::vector<unsigned char>* row : { &first, &second }) {
            std::vector<unsigned char> padded = *row;
            padded.resize(4, 0);
            out.insert(out.end(), padded.begin(), padded.end());
        }
        return out;
    };
    // blue, green, red on disk; and a palette of red at 0 and green at 1
    const std::vector<unsigned char> red24 = { 0, 0, 0xFF };
    const std::vector<unsigned char> green24 = { 0, 0xFF, 0 };
    const std::vector<unsigned char> palette = { 0, 0, 0xFF, 0, 0, 0xFF, 0, 0 };

    for (const bool bottomUp : { true, false }) {
        for (const int bits : { 24, 8 }) {
            BOOST_TEST_CONTEXT((bottomUp ? "bottom up" : "top down") << " at " << bits << " bits") {
                const std::vector<unsigned char> bytes = bits == 24
                    ? file(24, bottomUp, {}, red24, green24)
                    : file(8, bottomUp, palette, { 0 }, { 1 });
                const boost::shared_ptr<v3d::image::Image> image = factory.read(bytes.data(), bytes.size(), "bmp");
                BOOST_REQUIRE(image);
                BOOST_CHECK_EQUAL((*image)[0], 0xFF);
                BOOST_CHECK_EQUAL((*image)[1], 0);
                BOOST_CHECK_EQUAL((*image)[3], 0);
                BOOST_CHECK_EQUAL((*image)[4], 0xFF);
            }
        }
    }
}

namespace {

/**
 * A bmp built byte by byte: one row per entry of rows, top row first, stored bottom up as a
 * positive height says. Each row is padded to a dword boundary here. Masks, when given, follow
 * the 40 byte header and the compression is BI_BITFIELDS.
 **/
std::vector<unsigned char> bitmap(int width, int bits, const std::vector<uint32_t> & masks,
    const std::vector<std::vector<unsigned char>> & rows) {
    std::vector<unsigned char> out;
    const auto put16 = [&out](uint32_t value) {
        out.push_back(static_cast<unsigned char>(value));
        out.push_back(static_cast<unsigned char>(value >> 8));
    };
    const auto put32 = [&put16](uint32_t value) {
        put16(value);
        put16(value >> 16);
    };
    const uint32_t pad = (static_cast<uint32_t>(width * bits / 8) + 3) / 4 * 4;
    const uint32_t offset = 14 + 40 + static_cast<uint32_t>(masks.size()) * 4;
    put16(19778);
    put32(offset + pad * static_cast<uint32_t>(rows.size()));
    put32(0);
    put32(offset);
    put32(40);
    put32(static_cast<uint32_t>(width));
    put32(static_cast<uint32_t>(rows.size()));
    put16(1);
    put16(static_cast<uint32_t>(bits));
    put32(masks.empty() ? 0 : 3);
    for (int field = 0; field < 5; ++field) {
        put32(0);
    }
    for (const uint32_t mask : masks) {
        put32(mask);
    }
    for (auto row = rows.rbegin(); row != rows.rend(); ++row) {
        std::vector<unsigned char> padded = *row;
        padded.resize(pad, 0);
        out.insert(out.end(), padded.begin(), padded.end());
    }
    return out;
}

};  // namespace

/**
 * 16 bits uncompressed is five bits a channel, and widens so that a full channel is 255. With
 * BI_BITFIELDS the masks the file gives are used instead, here five, six and five bits.
 **/
BOOST_AUTO_TEST_CASE(imagereader_bmp_reads_16_bits) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    // red, green and blue at full, little endian, in one row of three
    const std::vector<unsigned char> plain = bitmap(3, 16, {}, { { 0x00, 0x7C, 0xE0, 0x03, 0x1F, 0x00 } });
    const boost::shared_ptr<v3d::image::Image> image = factory.read(plain.data(), plain.size(), "bmp");
    BOOST_REQUIRE(image);
    const unsigned char expected[9] = { 255, 0, 0, 0, 255, 0, 0, 0, 255 };
    for (unsigned int index = 0; index < 9; ++index) {
        BOOST_CHECK_EQUAL((*image)[index], expected[index]);
    }

    // a green of six bits at full, and a red of five bits at half
    const std::vector<unsigned char> masked =
        bitmap(2, 16, { 0xF800u, 0x07E0u, 0x001Fu }, { { 0xE0, 0x07, 0x00, 0x80 } });
    const boost::shared_ptr<v3d::image::Image> packed = factory.read(masked.data(), masked.size(), "bmp");
    BOOST_REQUIRE(packed);
    BOOST_CHECK_EQUAL((*packed)[0], 0);
    BOOST_CHECK_EQUAL((*packed)[1], 255);
    BOOST_CHECK_EQUAL((*packed)[2], 0);
    BOOST_CHECK_EQUAL((*packed)[3], 132);
}

/**
 * A 32 bit file keeps its alpha when any pixel uses it, so a pixel of alpha zero beside one of
 * alpha 0x80 stays transparent rather than being made opaque.
 **/
BOOST_AUTO_TEST_CASE(imagereader_bmp_keeps_a_used_alpha) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    // blue, green, red and alpha on disk
    const std::vector<unsigned char> bytes = bitmap(2, 32, {}, { { 0, 0, 255, 0x80, 255, 0, 0, 0 } });
    const boost::shared_ptr<v3d::image::Image> image = factory.read(bytes.data(), bytes.size(), "bmp");
    BOOST_REQUIRE(image);
    BOOST_CHECK_EQUAL(image->bpp(), 32u);
    BOOST_CHECK_EQUAL((*image)[0], 255);
    BOOST_CHECK_EQUAL((*image)[3], 0x80);
    BOOST_CHECK_EQUAL((*image)[6], 255);
    BOOST_CHECK_EQUAL((*image)[7], 0);
}

/**
 * Rows wider than one pixel are padded on disk and not in the image. Every channel of every
 * pixel comes back where it was. The image is two rows of three at 24 bits, each pixel a
 * different colour.
 **/
BOOST_AUTO_TEST_CASE(imagereader_bmp_rows_wider_than_a_pixel) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);

    // blue, green, red on disk; each pixel's three bytes are its index times ten, plus 1, 2, 3
    std::vector<std::vector<unsigned char>> rows(2);
    for (unsigned int row = 0; row < 2; ++row) {
        for (unsigned int column = 0; column < 3; ++column) {
            const unsigned char base = static_cast<unsigned char>((row * 3 + column) * 10);
            rows[row].push_back(static_cast<unsigned char>(base + 3));
            rows[row].push_back(static_cast<unsigned char>(base + 2));
            rows[row].push_back(static_cast<unsigned char>(base + 1));
        }
    }
    const std::vector<unsigned char> bytes = bitmap(3, 24, {}, rows);
    const boost::shared_ptr<v3d::image::Image> image = factory.read(bytes.data(), bytes.size(), "bmp");
    BOOST_REQUIRE(image);
    for (unsigned int pixel = 0; pixel < 6; ++pixel) {
        BOOST_TEST_CONTEXT("pixel " << pixel) {
            BOOST_CHECK_EQUAL((*image)[pixel * 3], pixel * 10 + 1);
            BOOST_CHECK_EQUAL((*image)[pixel * 3 + 1], pixel * 10 + 2);
            BOOST_CHECK_EQUAL((*image)[pixel * 3 + 2], pixel * 10 + 3);
        }
    }
}
