/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../TextureFontCache.h"

namespace {
const char* kTypeface = "data/fonts/NotoSans-Regular.ttf";
};  // namespace

/**
 * The cache this replaces was v3D::FontCache, which loaded fonts by name. TextureFontCache
 * does not load anything - it owns the atlas the fonts share and keeps the fonts that have
 * been built against it, keyed by file and size.
 **/
BOOST_AUTO_TEST_CASE(texturefontcache_test) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::TextureFontCache cache(256, 256, 1, logger);

    boost::shared_ptr<v3d::image::TextureAtlas> atlas = cache.atlas();
    BOOST_REQUIRE(atlas != nullptr);
    BOOST_CHECK_EQUAL(atlas->width(), 256u);
    BOOST_CHECK_EQUAL(atlas->height(), 256u);
    BOOST_CHECK_EQUAL(atlas->depth(), 1u);

    // nothing is in the cache to begin with
    BOOST_CHECK(cache.find(kTypeface, 12.0f) == nullptr);

    boost::shared_ptr<v3d::font::TextureFont> font =
        boost::make_shared<v3d::font::TextureFont>(std::string(kTypeface), 12.0f, logger);
    font->atlas(atlas);
    cache.add(font);

    // a font is found by file and size together
    BOOST_CHECK(cache.find(kTypeface, 12.0f) == font);
    BOOST_CHECK(cache.find(kTypeface, 14.0f) == nullptr);
    BOOST_CHECK(cache.find("data/fonts/DoesNotExist.ttf", 12.0f) == nullptr);

    // a second size of the same face is a separate entry
    boost::shared_ptr<v3d::font::TextureFont> larger =
        boost::make_shared<v3d::font::TextureFont>(std::string(kTypeface), 14.0f, logger);
    larger->atlas(atlas);
    cache.add(larger);
    BOOST_CHECK(cache.find(kTypeface, 14.0f) == larger);
    BOOST_CHECK(cache.find(kTypeface, 12.0f) == font);

    // removing matches on the same key, and only removes once
    BOOST_CHECK_EQUAL(cache.remove(font), true);
    BOOST_CHECK(cache.find(kTypeface, 12.0f) == nullptr);
    BOOST_CHECK_EQUAL(cache.remove(font), false);
    BOOST_CHECK(cache.find(kTypeface, 14.0f) == larger);
}

BOOST_AUTO_TEST_CASE(texturefont_glyph_test) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::TextureFontCache cache(256, 256, 1, logger);

    boost::shared_ptr<v3d::font::TextureFont> font =
        boost::make_shared<v3d::font::TextureFont>(std::string(kTypeface), 12.0f, logger);
    font->atlas(cache.atlas());
    cache.add(font);

    // the face's own metrics come off the file
    BOOST_CHECK_EQUAL(font->size(), 12.0f);
    BOOST_CHECK_EQUAL(font->filename(), std::string(kTypeface));
    BOOST_CHECK(font->ascender() > 0.0f);
    BOOST_CHECK(font->descender() < 0.0f);
    BOOST_CHECK(font->height() > 0.0f);

    // glyphs are rendered into the shared atlas on demand
    BOOST_REQUIRE_EQUAL(font->loadGlyphs(L"abc"), true);
    boost::shared_ptr<v3d::font::TextureFont::Glyph> glyph = font->glyph(L'a');
    BOOST_REQUIRE(glyph != nullptr);
    BOOST_CHECK_EQUAL(glyph->charcode_, L'a');
    BOOST_CHECK(glyph->width_ > 0u);
    BOOST_CHECK(glyph->height_ > 0u);
    BOOST_CHECK(glyph->advance_[0] > 0.0f);

    // and their texture coordinates stay inside it
    BOOST_CHECK(glyph->st_[0][0] >= 0.0f);
    BOOST_CHECK(glyph->st_[1][0] <= 1.0f);
    BOOST_CHECK(glyph->st_[0][1] >= 0.0f);
    BOOST_CHECK(glyph->st_[1][1] <= 1.0f);
}
