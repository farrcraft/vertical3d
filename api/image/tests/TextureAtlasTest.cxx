/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/TextureAtlas.h>
#include <api/image/Image.h>

#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

const unsigned int size = 64;

unsigned char texel(v3d::image::TextureAtlas* atlas, int x, int y) {
    return atlas->image()->data()[static_cast<size_t>(y) * size + x];
}

};  // namespace

BOOST_AUTO_TEST_CASE(textureatlas_region_is_the_size_asked_for) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::TextureAtlas atlas(size, size, 1, logger);

    const glm::ivec4 region = atlas.region(8, 6);

    // the gutter is the atlas's business, so what comes back is what was asked for and
    // not what was reserved behind it
    BOOST_CHECK_EQUAL(region.z, 8);
    BOOST_CHECK_EQUAL(region.w, 6);

    // and it is clear of the sheet edge by the border plus the gutter
    BOOST_CHECK(region.x >= 2);
    BOOST_CHECK(region.y >= 2);
    BOOST_CHECK(region.x + region.z <= static_cast<int>(size) - 2);
    BOOST_CHECK(region.y + region.w <= static_cast<int>(size) - 2);
}

/**
 * The invariant the gutter exists for: nothing a caller writes lands in a texel that
 * borders another region, so a sampler reading a region's edge cannot reach a neighbour.
 *
 * Filling each region with 0xff and then reading the ring around it says this without
 * knowing where the packer put anything - a gutter that is still zero was written by
 * nobody, and the atlas clears its image at construction.
 **/
BOOST_AUTO_TEST_CASE(textureatlas_regions_do_not_touch) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::TextureAtlas atlas(size, size, 1, logger);

    const unsigned int side = 8;
    std::vector<unsigned char> filled(static_cast<size_t>(side) * side, 0xff);
    std::vector<glm::ivec4> placed;

    for (unsigned int i = 0; i < 12; ++i) {
        const glm::ivec4 region = atlas.region(side, side);
        BOOST_REQUIRE(region.x >= 0);
        atlas.region(region.x, region.y, side, side, filled.data(), side);
        placed.push_back(region);
    }

    for (const glm::ivec4& region : placed) {
        for (int x = region.x - 1; x <= region.x + region.z; ++x) {
            BOOST_CHECK_EQUAL(static_cast<int>(texel(&atlas, x, region.y - 1)), 0);
            BOOST_CHECK_EQUAL(static_cast<int>(texel(&atlas, x, region.y + region.w)), 0);
        }
        for (int y = region.y - 1; y <= region.y + region.w; ++y) {
            BOOST_CHECK_EQUAL(static_cast<int>(texel(&atlas, region.x - 1, y)), 0);
            BOOST_CHECK_EQUAL(static_cast<int>(texel(&atlas, region.x + region.z, y)), 0);
        }
    }

    // the fill did land, so the ring above is a gutter rather than an empty atlas
    BOOST_CHECK_EQUAL(static_cast<int>(texel(&atlas, placed[0].x, placed[0].y)), 0xff);
}

BOOST_AUTO_TEST_CASE(textureatlas_refuses_what_does_not_fit) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::TextureAtlas atlas(16, 16, 1, logger);

    // the gutter is charged against the sheet, so a 16 square holds a 12 region and not a 14
    BOOST_CHECK(atlas.region(12, 12).x >= 0);

    v3d::image::TextureAtlas tight(16, 16, 1, logger);
    BOOST_CHECK_EQUAL(tight.region(14, 14).x, -1);
}
