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

const wchar_t* const kPrintable =
L" !\"#$%&'()*+,-./0123456789:;<=>?"
L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
L"`abcdefghijklmnopqrstuvwxyz{|}~";
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

/**
 * An atlas too small for what it is given fails rather than reporting success.
 *
 * The 64x64 here is chosen to be hopeless rather than marginal: printable ascii at 48px
 * wants far more than that, so the run packs some glyphs and then cannot. What is being
 * asserted is that the partial success is reported as failure - the count of glyphs that
 * did not fit used to be kept and never read, so a caller saw true and drew text with
 * characters missing.
 **/
BOOST_AUTO_TEST_CASE(texturefont_atlas_overflow_test) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::TextureFontCache cache(64, 64, 1, logger);

    boost::shared_ptr<v3d::font::TextureFont> font =
        boost::make_shared<v3d::font::TextureFont>(std::string(kTypeface), 48.0f, logger);
    font->atlas(cache.atlas());

    BOOST_CHECK_EQUAL(font->loadGlyphs(kPrintable), false);

    // and an atlas with room for the same charset still succeeds, so the failure is the
    // packing rather than the face or the charset
    v3d::font::TextureFontCache roomy(512, 512, 1, logger);
    boost::shared_ptr<v3d::font::TextureFont> small =
        boost::make_shared<v3d::font::TextureFont>(std::string(kTypeface), 12.0f, logger);
    small->atlas(roomy.atlas());
    BOOST_CHECK_EQUAL(small->loadGlyphs(kPrintable), true);
}

/**
 * The atlas budget a distance field asks for, at the base size and spread ui::TextRenderer
 * defaults to.
 *
 * A distance field glyph carries its spread on every side, so it is substantially larger
 * than the coverage glyph of the same face and size, and the 512 square the tree has always
 * packed into was the thing most likely to stop being enough. It is still enough at 48 with
 * a spread of 8, and it is close: raising either knob one step overflows it, which is what
 * the second half of this asserts and why the dimensions are an argument now.
 **/
BOOST_AUTO_TEST_CASE(texturefont_distance_field_packing_test) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();

    v3d::font::TextureFontCache roomy(512, 512, 1, logger);
    boost::shared_ptr<v3d::font::TextureFont> field =
        boost::make_shared<v3d::font::TextureFont>(std::string(kTypeface), 48.0f, logger, 8);
    field->atlas(roomy.atlas());
    BOOST_CHECK_EQUAL(field->spread(), 8u);
    BOOST_REQUIRE_EQUAL(field->loadGlyphs(kPrintable), true);

    // the same charset one step wider does not fit the same square, and says so
    v3d::font::TextureFontCache cramped(512, 512, 1, logger);
    boost::shared_ptr<v3d::font::TextureFont> tooBig =
        boost::make_shared<v3d::font::TextureFont>(std::string(kTypeface), 48.0f, logger, 12);
    tooBig->atlas(cramped.atlas());
    BOOST_CHECK_EQUAL(tooBig->loadGlyphs(kPrintable), false);

    // and asking for the dimensions it needs is what makes it fit
    v3d::font::TextureFontCache larger(1024, 1024, 1, logger);
    boost::shared_ptr<v3d::font::TextureFont> wider =
        boost::make_shared<v3d::font::TextureFont>(std::string(kTypeface), 48.0f, logger, 12);
    wider->atlas(larger.atlas());
    BOOST_CHECK_EQUAL(wider->loadGlyphs(kPrintable), true);

    // a distance field glyph is the coverage one grown by the spread on each side, which
    // is the whole of why the budget moved
    boost::shared_ptr<v3d::font::TextureFontCache> plain =
        boost::make_shared<v3d::font::TextureFontCache>(512, 512, 1, logger);
    boost::shared_ptr<v3d::font::TextureFont> coverage =
        boost::make_shared<v3d::font::TextureFont>(std::string(kTypeface), 48.0f, logger);
    coverage->atlas(plain->atlas());
    BOOST_CHECK_EQUAL(coverage->spread(), 0u);
    BOOST_REQUIRE_EQUAL(coverage->loadGlyphs(kPrintable), true);

    boost::shared_ptr<v3d::font::TextureFont::Glyph> wide = field->glyph(L'm');
    boost::shared_ptr<v3d::font::TextureFont::Glyph> narrow = coverage->glyph(L'm');
    BOOST_REQUIRE(wide != nullptr);
    BOOST_REQUIRE(narrow != nullptr);
    BOOST_CHECK(wide->width_ > narrow->width_);
    BOOST_CHECK(wide->height_ > narrow->height_);

    // the advance is the face's own metric and is not a bitmap dimension, so the spread
    // must not have moved it - it is what a line of text is laid out along
    BOOST_CHECK_CLOSE(wide->advance_.x, narrow->advance_.x, 0.01f);

    // and the coordinates still land inside the atlas they were packed into
    BOOST_CHECK(wide->st_[0][0] >= 0.0f);
    BOOST_CHECK(wide->st_[1][0] <= 1.0f);
    BOOST_CHECK(wide->st_[0][1] >= 0.0f);
    BOOST_CHECK(wide->st_[1][1] <= 1.0f);
}
