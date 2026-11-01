/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/image/reader/Svg.h>
#include <api/log/Logger.h>

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/vec2.hpp>

namespace {

/**
 * Four pixels wide and two tall, with the left half filled red.
 **/
const char halfRed[] = R"(<svg xmlns="http://www.w3.org/2000/svg" width="4" height="2">
    <rect x="0" y="0" width="2" height="2" fill="#ff0000"/>
</svg>)";

boost::shared_ptr<v3d::image::Image> read(const std::string& text) {
    v3d::image::reader::Svg svg(boost::make_shared<v3d::log::Logger>());
    return svg.read(reinterpret_cast<const unsigned char*>(text.data()), text.size());
}

boost::shared_ptr<v3d::image::Image> read(const std::string& text, const glm::uvec2& into) {
    v3d::image::reader::Svg svg(boost::make_shared<v3d::log::Logger>());
    return svg.read(reinterpret_cast<const unsigned char*>(text.data()), text.size(), into);
}

/**
 * @return the red and alpha channels of the pixel at x and y
 **/
glm::uvec2 redAndAlpha(const v3d::image::Image& image, unsigned int x, unsigned int y) {
    const unsigned int at = ((y * image.width()) + x) * 4;
    return glm::uvec2(image[at], image[at + 3]);
}

BOOST_AUTO_TEST_SUITE(svg_reader_test)

/**
 * A document read without a size is drawn at the size it gives itself, in four channels, and
 * transparent where nothing is drawn.
 **/
BOOST_AUTO_TEST_CASE(a_document_is_drawn_at_its_own_size) {
    const boost::shared_ptr<v3d::image::Image> image = read(halfRed);

    BOOST_REQUIRE(image);
    BOOST_CHECK_EQUAL(image->width(), 4U);
    BOOST_CHECK_EQUAL(image->height(), 2U);
    BOOST_CHECK(image->format() == v3d::image::Image::Format::RGBA);
    BOOST_CHECK(redAndAlpha(*image, 0, 0) == glm::uvec2(255, 255));
    BOOST_CHECK(redAndAlpha(*image, 3, 1) == glm::uvec2(0, 0));
}

/**
 * A document read into a size is scaled to fit inside it, keeping its proportions, and centred.
 * Four by two into eight by eight doubles it to eight by four, two rows down.
 **/
BOOST_AUTO_TEST_CASE(a_document_is_scaled_to_fit_and_centred) {
    const boost::shared_ptr<v3d::image::Image> image = read(halfRed, glm::uvec2(8, 8));

    BOOST_REQUIRE(image);
    BOOST_CHECK_EQUAL(image->width(), 8U);
    BOOST_CHECK_EQUAL(image->height(), 8U);
    BOOST_CHECK(redAndAlpha(*image, 1, 3) == glm::uvec2(255, 255));
    BOOST_CHECK(redAndAlpha(*image, 1, 0) == glm::uvec2(0, 0));
    BOOST_CHECK(redAndAlpha(*image, 1, 7) == glm::uvec2(0, 0));
    BOOST_CHECK(redAndAlpha(*image, 6, 3) == glm::uvec2(0, 0));
}

/**
 * Text that is not SVG, a document with no size, and an empty size asked for each give nothing.
 **/
BOOST_AUTO_TEST_CASE(what_cannot_be_drawn_gives_nothing) {
    BOOST_CHECK(!read("not an svg document"));
    BOOST_CHECK(!read(R"(<svg xmlns="http://www.w3.org/2000/svg"></svg>)"));
    BOOST_CHECK(!read(halfRed, glm::uvec2(0, 8)));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
