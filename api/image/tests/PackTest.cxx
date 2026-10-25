/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Pack.h>

#include <api/image/Compare.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/vec2.hpp>

namespace {

/**
 * An image whose bytes all differ from their neighbours', and from another image's at the
 * same place, so a sprite cut from the wrong place cannot match by coincidence. Each image
 * starts its bytes where the last one painted left off.
 **/
boost::shared_ptr<v3d::image::Image> painted(const glm::uvec2 & size, std::uint8_t bpp) {
    static unsigned int next = 0;
    boost::shared_ptr<v3d::image::Image> image = boost::make_shared<v3d::image::Image>(size, bpp);
    const unsigned int bytes = size.x * size.y * (bpp / 8U);
    for (unsigned int i = 0; i < bytes; ++i) {
        (*image)[i] = static_cast<unsigned char>(next * 7U);
        ++next;
    }
    return image;
}

boost::shared_ptr<v3d::log::Logger> logger() {
    return boost::make_shared<v3d::log::Logger>();
}

/** Pack, then unpack what was packed, and check every sprite comes back as it went in. **/
void roundTrip(const std::vector<v3d::image::Sprite> & sprites) {
    const v3d::image::Packing packing = v3d::image::pack(sprites, 256, logger());
    BOOST_REQUIRE_MESSAGE(packing.sheet, packing.refusal);
    BOOST_CHECK(packing.refusal.empty());
    BOOST_REQUIRE_EQUAL(packing.placements.size(), sprites.size());

    const v3d::image::Unpacking unpacking = v3d::image::unpack(*packing.sheet, packing.placements);
    BOOST_REQUIRE_MESSAGE(unpacking.refusal.empty(), unpacking.refusal);
    BOOST_REQUIRE_EQUAL(unpacking.sprites.size(), sprites.size());
    for (std::size_t i = 0; i < sprites.size(); ++i) {
        BOOST_TEST_CONTEXT("sprite " << sprites[i].name) {
            BOOST_CHECK_EQUAL(packing.placements[i].name, sprites[i].name);
            BOOST_CHECK_EQUAL(unpacking.sprites[i].name, sprites[i].name);
            const v3d::image::Difference difference = v3d::image::compare(*sprites[i].image, *unpacking.sprites[i].image, 0);
            BOOST_CHECK_MESSAGE(difference.match, difference.description());
        }
    }
}

/**
 * Sprites of several sizes come back unchanged after a pack and an unpack, in the order they
 * were given, though the tallest are placed first.
 **/
BOOST_AUTO_TEST_CASE(pack_then_unpack_returns_every_sprite_test) {
    roundTrip({
        {.name = "small", .image = painted(glm::uvec2(5, 3), 32)},
        {.name = "tall", .image = painted(glm::uvec2(4, 20), 32)},
        {.name = "wide", .image = painted(glm::uvec2(30, 6), 32)},
        {.name = "square", .image = painted(glm::uvec2(9, 9), 32)},
    });
}

/** A grey sheet packs and unpacks as a colour one does. **/
BOOST_AUTO_TEST_CASE(pack_then_unpack_a_grey_sheet_test) {
    roundTrip({
        {.name = "a", .image = painted(glm::uvec2(7, 7), 8)},
        {.name = "b", .image = painted(glm::uvec2(3, 11), 8)},
    });
}

/**
 * The sheet is the smallest size that holds the sprites. A 16 pixel sheet keeps a texel round
 * its edge and round each sprite, so it holds a 12 by 12 sprite and not a 13 by 13 one.
 **/
BOOST_AUTO_TEST_CASE(pack_chooses_the_smallest_sheet_test) {
    const v3d::image::Packing fits = v3d::image::pack({{.name = "a", .image = painted(glm::uvec2(12, 12), 24)}}, 256, logger());
    BOOST_REQUIRE(fits.sheet);
    BOOST_CHECK_EQUAL(fits.sheet->width(), 16U);
    BOOST_CHECK_EQUAL(fits.sheet->height(), 16U);

    const v3d::image::Packing grows = v3d::image::pack({{.name = "a", .image = painted(glm::uvec2(13, 13), 24)}}, 256, logger());
    BOOST_REQUIRE(grows.sheet);
    BOOST_CHECK_EQUAL(grows.sheet->width(), 32U);
}

/** A largest side that is not a power of two is tried itself, after the powers below it. **/
BOOST_AUTO_TEST_CASE(pack_tries_a_largest_side_that_is_not_a_power_of_two_test) {
    const v3d::image::Packing packing = v3d::image::pack({{.name = "a", .image = painted(glm::uvec2(40, 40), 24)}}, 48, logger());
    BOOST_REQUIRE_MESSAGE(packing.sheet, packing.refusal);
    BOOST_CHECK_EQUAL(packing.sheet->width(), 48U);
}

/** Sprites that do not fit in the largest sheet are refused with a reason, and nothing is packed. **/
BOOST_AUTO_TEST_CASE(pack_refuses_sprites_that_do_not_fit_test) {
    const v3d::image::Packing packing = v3d::image::pack({
        {.name = "a", .image = painted(glm::uvec2(20, 20), 32)},
        {.name = "b", .image = painted(glm::uvec2(20, 20), 32)},
    }, 32, logger());
    BOOST_CHECK(!packing.sheet);
    BOOST_CHECK(packing.placements.empty());
    BOOST_CHECK_EQUAL(packing.refusal, "the sprites do not fit in a sheet 32 pixels a side");
}

/** Sprites a sheet cannot hold at any size are refused before anything is packed. **/
BOOST_AUTO_TEST_CASE(pack_refuses_sprites_no_sheet_can_hold_test) {
    BOOST_CHECK_EQUAL(v3d::image::pack({}, 256, logger()).refusal, "there are no sprites to pack");
    BOOST_CHECK_EQUAL(v3d::image::pack({{.name = "a", .image = nullptr}}, 256, logger()).refusal,
        "the sprite \"a\" has no pixels");
    BOOST_CHECK_EQUAL(v3d::image::pack({
        {.name = "a", .image = painted(glm::uvec2(2, 2), 32)},
        {.name = "a", .image = painted(glm::uvec2(2, 2), 32)},
    }, 256, logger()).refusal, "two sprites are named \"a\"");
    BOOST_CHECK_EQUAL(v3d::image::pack({
        {.name = "a", .image = painted(glm::uvec2(2, 2), 32)},
        {.name = "b", .image = painted(glm::uvec2(2, 2), 24)},
    }, 256, logger()).refusal, "the sprite \"b\" is 24 bits a pixel and \"a\" is 32");
    BOOST_CHECK_EQUAL(v3d::image::pack({{.name = "a", .image = painted(glm::uvec2(2, 2), 16)}}, 256, logger()).refusal,
        "a sheet holds 8, 24 or 32 bits a pixel, not 16");
    BOOST_CHECK_EQUAL(v3d::image::pack({{.name = "a", .image = painted(glm::uvec2(2, 2), 32)}}, 8, logger()).refusal,
        "a sheet is at least 16 pixels a side, not 8");
}

/** A placement outside the sheet is refused with a reason, and nothing is cut. **/
BOOST_AUTO_TEST_CASE(unpack_refuses_a_placement_outside_the_sheet_test) {
    const boost::shared_ptr<v3d::image::Image> sheet = painted(glm::uvec2(16, 16), 32);
    const v3d::image::Unpacking unpacking = v3d::image::unpack(*sheet, {
        {.name = "inside", .area = glm::uvec4(0, 0, 4, 4)},
        {.name = "outside", .area = glm::uvec4(14, 0, 4, 4)},
    });
    BOOST_CHECK(unpacking.sprites.empty());
    BOOST_CHECK_EQUAL(unpacking.refusal, "the sprite \"outside\" is not a rectangle inside the sheet");
}

};  // namespace
