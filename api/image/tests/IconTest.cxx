/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Icon.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/log/Logger.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>

namespace {

/**
 * A square RGBA image of a side, every pixel the same opaque grey.
 **/
boost::shared_ptr<v3d::image::Image> square(unsigned int side) {
    boost::shared_ptr<v3d::image::Image> made = boost::make_shared<v3d::image::Image>(glm::uvec2(side, side), 32);
    for (unsigned int index = 0; index < side * side * 4; index++) {
        (*made)[index] = (index % 4) == 3 ? 255 : 128;
    }
    return made;
}

/**
 * @return the number of a size stored at an offset, least significant byte first
 **/
std::uint32_t number(const std::string& bytes, std::size_t at, std::size_t size) {
    std::uint32_t value = 0;
    for (std::size_t index = 0; index < size; index++) {
        value |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[at + index])) << (index * 8);
    }
    return value;
}

std::optional<std::string> encode(const std::vector<boost::shared_ptr<v3d::image::Image>>& images) {
    return v3d::image::encodeIcon(images, boost::make_shared<v3d::log::Logger>());
}

BOOST_AUTO_TEST_SUITE(icon_test)

/**
 * An icon file is a header, a directory of one entry for each image, and the images in the
 * order given. An entry states its image's side, with 256 written as 0, its size, and where its
 * image lies, each right after the last.
 **/
BOOST_AUTO_TEST_CASE(an_icon_file_lists_each_image) {
    const std::optional<std::string> bytes = encode({ square(16), square(256) });
    BOOST_REQUIRE(bytes);
    BOOST_CHECK_EQUAL(number(*bytes, 0, 2), 0U);
    BOOST_CHECK_EQUAL(number(*bytes, 2, 2), 1U);
    BOOST_REQUIRE_EQUAL(number(*bytes, 4, 2), 2U);

    const unsigned int sides[] = { 16, 256 };
    std::size_t expected = 6 + (2 * 16);
    for (std::size_t index = 0; index < 2; index++) {
        const std::size_t entry = 6 + (index * 16);
        BOOST_CHECK_EQUAL(number(*bytes, entry, 1), sides[index] % 256);
        BOOST_CHECK_EQUAL(number(*bytes, entry + 1, 1), sides[index] % 256);
        BOOST_CHECK_EQUAL(number(*bytes, entry + 4, 2), 1U);
        BOOST_CHECK_EQUAL(number(*bytes, entry + 6, 2), 32U);
        BOOST_CHECK_EQUAL(number(*bytes, entry + 12, 4), expected);
        expected += number(*bytes, entry + 8, 4);
    }
    BOOST_CHECK_EQUAL(bytes->size(), expected);
}

/**
 * An image 256 pixels a side is stored as a png, which reads back at its side.
 **/
BOOST_AUTO_TEST_CASE(the_largest_image_is_a_png) {
    const std::optional<std::string> bytes = encode({ square(256) });
    BOOST_REQUIRE(bytes);
    v3d::image::reader::Png png(boost::make_shared<v3d::log::Logger>());
    const boost::shared_ptr<v3d::image::Image> read =
        png.read(reinterpret_cast<const unsigned char*>(bytes->data()) + number(*bytes, 18, 4), number(*bytes, 14, 4));
    BOOST_REQUIRE(read);
    BOOST_CHECK_EQUAL(read->width(), 256U);
}

/**
 * A smaller image is stored as a bitmap. Its 40 byte header states twice its height. Its pixels
 * follow as blue, green, red and alpha from the bottom row up. A mask follows them, with a bit set
 * for each clear pixel, and each row of it padded to four bytes.
 **/
BOOST_AUTO_TEST_CASE(a_smaller_image_is_a_bitmap_with_a_mask) {
    // red along the top row, and clear below it
    const boost::shared_ptr<v3d::image::Image> image = boost::make_shared<v3d::image::Image>(glm::uvec2(16, 16), 32);
    for (unsigned int index = 0; index < 16 * 16 * 4; index++) {
        (*image)[index] = 0;
    }
    for (unsigned int column = 0; column < 16; column++) {
        (*image)[(column * 4)] = 255;
        (*image)[(column * 4) + 3] = 255;
    }
    const std::optional<std::string> bytes = encode({ image });
    BOOST_REQUIRE(bytes);
    const std::size_t at = number(*bytes, 18, 4);
    BOOST_CHECK_EQUAL(number(*bytes, 14, 4), 40U + (16U * 16U * 4U) + (4U * 16U));
    BOOST_CHECK_EQUAL(number(*bytes, at, 4), 40U);
    BOOST_CHECK_EQUAL(number(*bytes, at + 4, 4), 16U);
    BOOST_CHECK_EQUAL(number(*bytes, at + 8, 4), 32U);
    BOOST_CHECK_EQUAL(number(*bytes, at + 14, 2), 32U);

    // the bottom row comes first and is clear, and the top row comes last and is red
    const std::size_t pixels = at + 40;
    BOOST_CHECK_EQUAL(number(*bytes, pixels, 4), 0U);
    BOOST_CHECK_EQUAL(number(*bytes, pixels + (15U * 16U * 4U), 4), 0xFFFF0000U);
    const std::size_t mask = pixels + (16U * 16U * 4U);
    BOOST_CHECK_EQUAL(number(*bytes, mask, 4), 0x0000FFFFU);
    BOOST_CHECK_EQUAL(number(*bytes, mask + (15U * 4U), 4), 0U);
}

/**
 * No images, an image that is not square, one past 256 pixels, one that is not RGBA, and two of
 * one size are each refused.
 **/
BOOST_AUTO_TEST_CASE(an_icon_file_refuses_what_it_cannot_hold) {
    BOOST_CHECK(!encode({}));
    BOOST_CHECK(!encode({ boost::make_shared<v3d::image::Image>(glm::uvec2(16, 8), 32) }));
    BOOST_CHECK(!encode({ square(512) }));
    BOOST_CHECK(!encode({ boost::make_shared<v3d::image::Image>(glm::uvec2(16, 16), 24) }));
    BOOST_CHECK(!encode({ square(32), square(32) }));
}

/**
 * The sides an app's icon is drawn at run from small to large, end at the largest an icon file
 * holds, and include the 32 pixel icon a window shows at the usual scale.
 **/
BOOST_AUTO_TEST_CASE(an_apps_icon_is_drawn_at_each_side_an_icon_file_holds) {
    BOOST_CHECK_EQUAL(v3d::image::ICON_SIDES.back(), 256U);
    BOOST_CHECK(std::ranges::is_sorted(v3d::image::ICON_SIDES));
    BOOST_CHECK(std::ranges::find(v3d::image::ICON_SIDES, 32U) != v3d::image::ICON_SIDES.end());
    BOOST_CHECK(encode({ square(16), square(24), square(32), square(48), square(64), square(128), square(256) }));
}

/**
 * An app's icon is drawn from its SVG at each side, smallest first, and the drawings encode as
 * one icon file. A document that cannot be read draws nothing.
 **/
BOOST_AUTO_TEST_CASE(an_apps_icon_is_drawn_from_its_svg) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    const std::vector<boost::shared_ptr<v3d::image::Image>> drawn = v3d::image::drawIcon(
        R"(<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64"><rect width="64" height="64" fill="#4d8ce6"/></svg>)", logger);
    BOOST_REQUIRE_EQUAL(drawn.size(), v3d::image::ICON_SIDES.size());
    for (std::size_t index = 0; index < drawn.size(); index++) {
        BOOST_CHECK_EQUAL(drawn[index]->width(), v3d::image::ICON_SIDES[index]);
        BOOST_CHECK_EQUAL(drawn[index]->height(), v3d::image::ICON_SIDES[index]);
        BOOST_CHECK(drawn[index]->format() == v3d::image::Image::Format::RGBA);
    }
    BOOST_CHECK(v3d::image::encodeIcon(drawn, logger));
    BOOST_CHECK(v3d::image::drawIcon("not an svg", logger).empty());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
