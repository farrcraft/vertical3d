/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/TextureFont.h>
#include <api/font/TextureFontCache.h>
#include <api/image/Image.h>
#include <api/image/TextureAtlas.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

const char* const kInter = "data/fonts/Inter-Regular.ttf";
const char* const kInterSemiBold = "data/fonts/Inter-SemiBold.ttf";
const char* const kNoto = "data/fonts/NotoSans-Regular.ttf";

const float kBase = 48.0F;
const unsigned int kSpread = 8;

// U+01C4, a capital DZ with a caron, as UTF-8. Noto Sans has it and Inter does not
const char* const kDzCaron = "\xC7\x84";
const char32_t kDzCaronCode = 0x01C4;

/**
 * A face packing into a shared atlas.
 **/
boost::shared_ptr<v3d::font::TextureFont> face(const char* file, v3d::font::TextureFontCache* cache,
    const boost::shared_ptr<v3d::log::Logger>& logger) {
    const boost::shared_ptr<v3d::font::TextureFont> made =
        boost::make_shared<v3d::font::TextureFont>(file, kBase, logger, kSpread);
    made->atlas(cache->atlas());
    return made;
}

BOOST_AUTO_TEST_SUITE(texture_font_test)

/**
 * A code point the face lacks is packed into its fallback, and measures as the fallback does.
 * Without a fallback, the face packs its own missing glyph for it.
 **/
BOOST_AUTO_TEST_CASE(a_missing_code_point_is_packed_from_the_fallback) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::TextureFontCache cache(1024, 1024, 1, logger);

    const boost::shared_ptr<v3d::font::TextureFont> noto = face(kNoto, &cache, logger);
    const boost::shared_ptr<v3d::font::TextureFont> inter = face(kInter, &cache, logger);
    inter->fallback(noto);
    BOOST_REQUIRE(inter->loadGlyphs(std::string("A") + kDzCaron));

    BOOST_REQUIRE(noto->packed(kDzCaronCode));
    BOOST_CHECK(inter->packed(kDzCaronCode) == noto->packed(kDzCaronCode));
    BOOST_CHECK_CLOSE(inter->width(kDzCaron), noto->width(kDzCaron), 0.01F);
    BOOST_CHECK(!noto->packed(U'A'));

    const boost::shared_ptr<v3d::font::TextureFont> alone = face(kInter, &cache, logger);
    BOOST_REQUIRE(alone->loadGlyphs(kDzCaron));
    BOOST_REQUIRE(alone->packed(kDzCaronCode));
    BOOST_CHECK(alone->packed(kDzCaronCode) != noto->packed(kDzCaronCode));
}

/**
 * Two faces sharing a fallback pack a code point into it once, and both find the same glyph.
 **/
BOOST_AUTO_TEST_CASE(a_shared_fallback_packs_a_code_point_once) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::TextureFontCache cache(1024, 1024, 1, logger);

    const boost::shared_ptr<v3d::font::TextureFont> noto = face(kNoto, &cache, logger);
    const boost::shared_ptr<v3d::font::TextureFont> regular = face(kInter, &cache, logger);
    const boost::shared_ptr<v3d::font::TextureFont> semiBold = face(kInterSemiBold, &cache, logger);
    regular->fallback(noto);
    semiBold->fallback(noto);
    BOOST_REQUIRE(regular->loadGlyphs(kDzCaron));
    const boost::shared_ptr<v3d::image::Image> image = cache.atlas()->image();
    const std::size_t bytes = static_cast<std::size_t>(image->width()) * image->height();
    const std::vector<unsigned char> before(image->data(), image->data() + bytes);
    BOOST_REQUIRE(semiBold->loadGlyphs(kDzCaron));
    const std::vector<unsigned char> after(image->data(), image->data() + bytes);

    BOOST_CHECK(regular->packed(kDzCaronCode) == semiBold->packed(kDzCaronCode));
    // the second load rasterized nothing, so the atlas holds what it held before
    BOOST_CHECK(before == after);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
