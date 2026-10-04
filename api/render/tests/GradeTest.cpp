/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/render/realtime/Grade.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

using v3d::render::realtime::Grade;

namespace {

/**
 * The strip the identity table is baked to: slice b at b * 16 across, red across each slice
 * and green down it, each entry k of fifteen.
 **/
boost::shared_ptr<v3d::image::Image> identityStrip(uint8_t bits) {
    const uint32_t size = Grade::SIZE;
    boost::shared_ptr<v3d::image::Image> strip = boost::make_shared<v3d::image::Image>(size * size, size, bits);
    const std::size_t channels = bits / 8;
    for (uint32_t y = 0; y < size; y++) {
        for (uint32_t x = 0; x < size * size; x++) {
            unsigned char* at = strip->data() + (static_cast<std::size_t>(y) * size * size + x) * channels;
            at[0] = static_cast<unsigned char>((x % size) * 17);
            at[1] = static_cast<unsigned char>(y * 17);
            at[2] = static_cast<unsigned char>((x / size) * 17);
            if (channels == 4) {
                at[3] = 0;
            }
        }
    }
    return strip;
}

/**
 * The four bytes of one entry of a table.
 **/
std::vector<uint8_t> entry(const std::vector<uint8_t>& table, std::size_t index) {
    return std::vector<uint8_t>(table.begin() + static_cast<std::ptrdiff_t>(index * 4),
        table.begin() + static_cast<std::ptrdiff_t>(index * 4 + 4));
}

};  // namespace

BOOST_AUTO_TEST_SUITE(grade_test)

/**
 * The identity's entries are their own positions, red fastest: the first is black, the
 * sixteenth is full red alone, and the last is white.
 **/
BOOST_AUTO_TEST_CASE(the_identity_is_each_entry_at_its_own_position) {
    const std::vector<uint8_t> table = Grade::identity();
    BOOST_REQUIRE_EQUAL(table.size(), static_cast<std::size_t>(16 * 16 * 16 * 4));
    BOOST_CHECK(entry(table, 0) == (std::vector<uint8_t>{0, 0, 0, 255}));
    BOOST_CHECK(entry(table, 15) == (std::vector<uint8_t>{255, 0, 0, 255}));
    BOOST_CHECK(entry(table, 16) == (std::vector<uint8_t>{0, 17, 0, 255}));
    BOOST_CHECK(entry(table, 16 * 16 * 16 - 1) == (std::vector<uint8_t>{255, 255, 255, 255}));
}

/**
 * A strip baked from the identity reads back as the identity, in RGB and in RGBA, which is
 * what pins slice, row and column to blue, green and red. The strip's alpha is not the table's.
 **/
BOOST_AUTO_TEST_CASE(a_strip_is_read_slice_by_slice) {
    BOOST_CHECK(Grade::table(identityStrip(24)) == Grade::identity());
    BOOST_CHECK(Grade::table(identityStrip(32)) == Grade::identity());
}

/**
 * A strip of any other shape, or none, is no table, and the grade falls back to the identity.
 **/
BOOST_AUTO_TEST_CASE(a_strip_of_the_wrong_shape_is_no_table) {
    BOOST_CHECK(Grade::table(boost::shared_ptr<v3d::image::Image>()).empty());
    BOOST_CHECK(Grade::table(boost::make_shared<v3d::image::Image>(16, 16, 24)).empty());
    BOOST_CHECK(Grade::table(boost::make_shared<v3d::image::Image>(256, 16, 8)).empty());
}

BOOST_AUTO_TEST_SUITE_END()
