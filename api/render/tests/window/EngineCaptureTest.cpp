/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/image/reader/Png.h>

#include <cstddef>
#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec4.hpp>

#include "Windowed.h"

namespace {

/**
 * How many texels of an image are not the RGBA given.
 **/
std::size_t mismatches(const boost::shared_ptr<v3d::image::Image>& image, unsigned char r, unsigned char g,
    unsigned char b, unsigned char a) {
    const unsigned char* pixels = image->data();
    std::size_t count = 0;
    const std::size_t texels = static_cast<std::size_t>(image->width()) * image->height();
    for (std::size_t i = 0; i < texels; ++i) {
        const unsigned char* texel = pixels + i * 4;
        if (texel[0] != r || texel[1] != g || texel[2] != b || texel[3] != a) {
            ++count;
        }
    }
    return count;
}

};  // namespace

/**
 * A capture holds the frame it was requested for, not the one before it.
 *
 * The first frame is cleared red and the captured one green. A copy taken before the frame is
 * recorded reads the image as the previous frame using it left it, which is red or undefined,
 * and never green.
 **/
BOOST_AUTO_TEST_CASE(engine3d_capture_writes_the_frame_it_presents_test) {
    v3d::test::Windowed windowed;
    const std::string path = "data_out/engine3d_capture.png";
    boost::filesystem::remove(path);

    windowed.engine->clearColour(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    windowed.engine->renderFrame();

    windowed.engine->clearColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    BOOST_REQUIRE(windowed.engine->capture(path));
    windowed.engine->renderFrame();

    v3d::image::reader::Png png(windowed.logger);
    const boost::shared_ptr<v3d::image::Image> written = png.read(path);
    BOOST_REQUIRE_MESSAGE(written != nullptr, "the frame wrote no readable " + path);
    BOOST_TEST(written->width() > 0u);
    BOOST_TEST(written->height() > 0u);
    BOOST_TEST(mismatches(written, 0, 255, 0, 255) == 0u);
    BOOST_TEST(windowed.silent());
}

/**
 * A request is answered by one frame, and the frame after it writes nothing.
 **/
BOOST_AUTO_TEST_CASE(engine3d_capture_is_cleared_by_the_frame_that_writes_it_test) {
    v3d::test::Windowed windowed;
    const std::string path = "data_out/engine3d_capture_once.png";
    boost::filesystem::remove(path);

    BOOST_REQUIRE(windowed.engine->capture(path));
    windowed.engine->renderFrame();
    BOOST_REQUIRE(boost::filesystem::exists(path));

    boost::filesystem::remove(path);
    windowed.engine->renderFrame();
    BOOST_TEST(!boost::filesystem::exists(path));
    BOOST_TEST(windowed.silent());
}
