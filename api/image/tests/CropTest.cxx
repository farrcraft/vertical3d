/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Crop.h>

#include <api/image/Compare.h>
#include <api/image/TextureAtlas.h>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * An image whose every byte is a different value, so a crop that reads the wrong row or
 * the wrong column cannot land on the right answer by coincidence.
 **/
boost::shared_ptr<v3d::image::Image> painted(unsigned int width, unsigned int height, unsigned char seed) {
    boost::shared_ptr<v3d::image::Image> image = boost::make_shared<v3d::image::Image>(width, height, 24);
    for (unsigned int i = 0; i < width * height * 3; ++i) {
        (*image)[i] = static_cast<unsigned char>(seed + i);
    }
    return image;
}

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

};  // namespace

BOOST_AUTO_TEST_CASE(crop_cuts_the_rectangle_asked_for) {
    const boost::shared_ptr<v3d::image::Image> source = painted(4, 4, 0);
    const boost::shared_ptr<v3d::image::Image> cut = v3d::image::crop(*source, 1, 1, 2, 2);

    BOOST_REQUIRE(cut);
    BOOST_CHECK_EQUAL(cut->width(), 2u);
    BOOST_CHECK_EQUAL(cut->height(), 2u);
    BOOST_CHECK_EQUAL(cut->bpp(), source->bpp());

    for (unsigned int row = 0; row < 2; ++row) {
        for (unsigned int byte = 0; byte < 2 * 3; ++byte) {
            const unsigned int from = ((1 + row) * 4 + 1) * 3 + byte;
            BOOST_CHECK_EQUAL(static_cast<int>((*cut)[row * 2 * 3 + byte]),
                static_cast<int>((*source)[from]));
        }
    }
}

BOOST_AUTO_TEST_CASE(crop_of_the_whole_image_is_the_image) {
    const boost::shared_ptr<v3d::image::Image> source = painted(5, 3, 40);
    const boost::shared_ptr<v3d::image::Image> cut = v3d::image::crop(*source, 0, 0, 5, 3);

    BOOST_REQUIRE(cut);
    BOOST_CHECK(v3d::image::compare(*cut, *source, 0).match);
}

/**
 * A rectangle that is not wholly inside the source is refused rather than clamped: reading
 * past the end of a row returns the start of the next one, which is a picture rather than
 * an error, and clamping would hand back an image of a size nobody asked for.
 **/
BOOST_AUTO_TEST_CASE(crop_refuses_what_it_cannot_cut) {
    const boost::shared_ptr<v3d::image::Image> source = painted(4, 4, 0);

    BOOST_CHECK(!v3d::image::crop(*source, 3, 0, 2, 1));
    BOOST_CHECK(!v3d::image::crop(*source, 0, 3, 1, 2));
    BOOST_CHECK(!v3d::image::crop(*source, 4, 0, 1, 1));
    BOOST_CHECK(!v3d::image::crop(*source, 0, 0, 0, 1));
    BOOST_CHECK(!v3d::image::crop(*source, 0, 0, 1, 0));

    // an offset near the top of the range must not wrap its way into fitting
    BOOST_CHECK(!v3d::image::crop(*source, 0xFFFFFFFF, 0, 2, 1));
    BOOST_CHECK(!v3d::image::crop(*source, 0, 0, 0xFFFFFFFF, 1));
}

/**
 * The whole point: a sprite blitted into an atlas comes back out of it unchanged, so a sheet
 * packed by the tree can be unpacked by the tree. This is the case an app migrating a
 * hand-packed sheet to a packer depends on.
 **/
BOOST_AUTO_TEST_CASE(crop_is_the_inverse_of_an_atlas_blit) {
    v3d::image::TextureAtlas atlas(64, 64, 3, logger());
    const boost::shared_ptr<v3d::image::Image> sprite = painted(8, 6, 17);

    const glm::ivec4 region = atlas.region(8, 6);
    BOOST_REQUIRE(region.x >= 0);
    atlas.region(region.x, region.y, 8, 6, sprite->data(), 8 * 3);

    const boost::shared_ptr<v3d::image::Image> back =
        v3d::image::crop(*atlas.image(), region.x, region.y, 8, 6);
    BOOST_REQUIRE(back);
    BOOST_CHECK(v3d::image::compare(*back, *sprite, 0).match);
}
