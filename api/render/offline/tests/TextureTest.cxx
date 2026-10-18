/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/offline/SearchPath.h>
#include <api/render/offline/Texture.h>
#include <api/render/offline/Textures.h>

#include <cmath>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

namespace {

/**
 * Two by two texels: red and green across the top, blue and white across the bottom.
 **/
v3d::render::offline::Texture quartered() {
    v3d::image::Image image(glm::uvec2(2, 2), 24);
    const unsigned char texels[12] = {
        255, 0, 0,   0, 255, 0,
        0, 0, 255,   255, 255, 255
    };
    for (unsigned int i = 0; i < 12; i++) {
        image[i] = texels[i];
    }
    return v3d::render::offline::Texture(image);
}

void same(const glm::vec3 & found, const glm::vec3 & expected) {
    BOOST_CHECK_SMALL(found.r - expected.r, 0.0001F);
    BOOST_CHECK_SMALL(found.g - expected.g, 0.0001F);
    BOOST_CHECK_SMALL(found.b - expected.b, 0.0001F);
}

/**
 * A texel's centre samples to the texel, and s runs across and t down, so (0, 0) is the
 * image's upper left corner as RI has it.
 **/
BOOST_AUTO_TEST_CASE(texture_texel_centres_test) {
    const v3d::render::offline::Texture texture = quartered();
    BOOST_CHECK_EQUAL(texture.width(), 2U);
    same(texture.sample(0.25F, 0.25F), glm::vec3(1.0F, 0.0F, 0.0F));
    same(texture.sample(0.75F, 0.25F), glm::vec3(0.0F, 1.0F, 0.0F));
    same(texture.sample(0.25F, 0.75F), glm::vec3(0.0F, 0.0F, 1.0F));
    same(texture.sample(0.75F, 0.75F), glm::vec3(1.0F));
}

/**
 * Between two centres is their blend, and the middle of all four is the mean of all four.
 **/
BOOST_AUTO_TEST_CASE(texture_bilinear_test) {
    const v3d::render::offline::Texture texture = quartered();
    same(texture.sample(0.5F, 0.25F), glm::vec3(0.5F, 0.5F, 0.0F));
    same(texture.sample(0.25F, 0.5F), glm::vec3(0.5F, 0.0F, 0.5F));
    same(texture.sample(0.5F, 0.5F), glm::vec3(0.5F, 0.5F, 0.5F));
}

/**
 * RI's default wrap is periodic: one past the edge is the start again, and so is one before
 * it, and an edge blends the last texel with the first.
 **/
BOOST_AUTO_TEST_CASE(texture_periodic_wrap_test) {
    const v3d::render::offline::Texture texture = quartered();
    same(texture.sample(1.25F, 0.25F), texture.sample(0.25F, 0.25F));
    same(texture.sample(-0.75F, 1.75F), texture.sample(0.25F, 0.75F));
    same(texture.sample(0.0F, 0.25F), glm::vec3(0.5F, 0.5F, 0.0F));
    // a coordinate far from the first repeat is taken into it rather than overflowing an index
    same(texture.sample(1.0e6F + 0.25F, 0.25F), texture.sample(0.25F, 0.25F));
    same(texture.sample(1.0e9F, 0.25F), texture.sample(0.0F, 0.25F));
    // a coordinate that is not finite names no texel
    same(texture.sample(std::nanf(""), 0.25F), glm::vec3(0.0F));
}

/**
 * A grey image is the same value in all three channels.
 **/
BOOST_AUTO_TEST_CASE(texture_grey_test) {
    v3d::image::Image image(glm::uvec2(1, 1), 8);
    image[0] = 51;
    const v3d::render::offline::Texture texture(image);
    same(texture.sample(0.5F, 0.5F), glm::vec3(0.2F));
}

/**
 * A name is found on the search path and read once; a name that cannot be read is null, and
 * is remembered as missing rather than tried again.
 **/
BOOST_AUTO_TEST_CASE(textures_find_test) {
    v3d::render::offline::Textures textures(boost::make_shared<v3d::log::Logger>());
    BOOST_CHECK(textures.find("blocks.png") == nullptr);

    textures.searchpath("nowhere:data");
    const v3d::render::offline::Texture* found = textures.find("data/blocks.png");
    BOOST_REQUIRE(found != nullptr);
    BOOST_CHECK_EQUAL(found->width(), 8U);
    // the second request is the same texture rather than a second read
    BOOST_CHECK(textures.find("data/blocks.png") == found);

    // blocks.png was requested before the path was set, and stays missing for the frame
    BOOST_CHECK(textures.find("blocks.png") == nullptr);
    v3d::render::offline::Textures fresh(boost::make_shared<v3d::log::Logger>());
    fresh.searchpath("nowhere:data");
    const v3d::render::offline::Texture* placed = fresh.find("blocks.png");
    BOOST_REQUIRE(placed != nullptr);
    // the second block across is red 85, and its centre is a quarter and an eighth in
    same(placed->sample(0.375F, 0.125F), glm::vec3(85.0F, 0.0F, 128.0F) / 255.0F);
}

/**
 * A search path is colon separated, a drive letter does not end a directory, and & is
 * whatever the path was before.
 **/
BOOST_AUTO_TEST_CASE(searchpath_test) {
    const std::vector<std::string> first = v3d::render::offline::searchpath("C:/maps:here", {});
    BOOST_REQUIRE_EQUAL(first.size(), 2U);
    BOOST_CHECK_EQUAL(first[0], "C:/maps");
    BOOST_CHECK_EQUAL(first[1], "here");

    const std::vector<std::string> appended = v3d::render::offline::searchpath("there:&", first);
    BOOST_REQUIRE_EQUAL(appended.size(), 3U);
    BOOST_CHECK_EQUAL(appended[0], "there");
    BOOST_CHECK_EQUAL(appended[2], "here");
}

};  // namespace
