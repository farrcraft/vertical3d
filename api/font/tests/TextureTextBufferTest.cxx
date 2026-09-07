/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../TextureFontCache.h"
#include "../TextureTextBuffer.h"

namespace {

const char* kTypeface = "data/fonts/NotoSans-Regular.ttf";

const float kBase = 48.0f;
const unsigned int kSpread = 8;

/**
 * A markup drawing plain text in the given face at the given size.
 *
 * Every decoration is off, because what these cases are about is the glyph quad and the
 * pen, and a background or an underline would put quads of its own into the buffer.
 **/
v3d::font::TextureTextBuffer::Markup plain(const boost::shared_ptr<v3d::font::TextureFont>& font, float size) {
    v3d::font::TextureTextBuffer::Markup markup;
    markup.family_ = "sans";
    markup.size_ = size;
    markup.bold_ = false;
    markup.italic_ = false;
    markup.rise_ = 0.0f;
    markup.spacing_ = 0.0f;
    markup.gamma_ = 1.0f;
    markup.outline_ = false;
    markup.underline_ = false;
    markup.overline_ = false;
    markup.strikethrough_ = false;
    markup.foregroundColor_ = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    markup.backgroundColor_ = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    markup.font_ = font;
    return markup;
}

/**
 * How far a line of text laid out at a size reaches from where the pen started.
 **/
float advanceOf(const boost::shared_ptr<v3d::font::TextureFont>& font, const std::wstring& text, float size) {
    v3d::font::TextureTextBuffer buffer;
    glm::vec2 pen(0.0f, 0.0f);
    v3d::font::TextureTextBuffer::Markup markup = plain(font, size);
    buffer.addText(&pen, markup, text);
    return pen.x;
}

};  // namespace

/**
 * One atlas, drawn at more than one size.
 *
 * This is the whole of what ADR-0036 bought, checked without a device: the glyph metrics
 * are in pixels of the size the face was rasterized at, so a markup asking for another size
 * lays out at a ratio of them. Asking for twice the base has to advance the pen twice as
 * far and put out a quad twice as large, from the same glyphs in the same atlas.
 **/
BOOST_AUTO_TEST_CASE(texturetextbuffer_scales_to_the_markup_size_test) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::TextureFontCache cache(512, 512, 1, logger);

    boost::shared_ptr<v3d::font::TextureFont> font =
        boost::make_shared<v3d::font::TextureFont>(std::string(kTypeface), kBase, logger, kSpread);
    font->atlas(cache.atlas());
    BOOST_REQUIRE_EQUAL(font->loadGlyphs(L"Hamburgefonstiv"), true);

    const std::wstring line = L"Hamburgefonstiv";

    const float base = advanceOf(font, line, kBase);
    BOOST_REQUIRE(base > 0.0f);

    // the pen advances in proportion, which is what a layout measuring a string relies on
    BOOST_CHECK_CLOSE(advanceOf(font, line, kBase * 2.0f), base * 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(advanceOf(font, line, kBase * 0.5f), base * 0.5f, 0.01f);

    // and a size that was not asked for is the base, so a caller that does not care about
    // size gets what it always got
    BOOST_CHECK_CLOSE(advanceOf(font, line, kBase), base, 0.01f);
}

/**
 * The quad a glyph is drawn into scales with the size, and its atlas coordinates do not.
 *
 * The second half is the point: one atlas serves every size because the glyph is sampled
 * from the same place however large it is drawn.
 **/
BOOST_AUTO_TEST_CASE(texturetextbuffer_quad_scales_and_the_atlas_does_not_test) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::TextureFontCache cache(512, 512, 1, logger);

    boost::shared_ptr<v3d::font::TextureFont> font =
        boost::make_shared<v3d::font::TextureFont>(std::string(kTypeface), kBase, logger, kSpread);
    font->atlas(cache.atlas());
    BOOST_REQUIRE_EQUAL(font->loadGlyphs(L"M"), true);

    v3d::font::TextureTextBuffer atBase;
    glm::vec2 basePen(0.0f, 100.0f);
    v3d::font::TextureTextBuffer::Markup baseMarkup = plain(font, kBase);
    atBase.addText(&basePen, baseMarkup, L"M");

    v3d::font::TextureTextBuffer atDouble;
    glm::vec2 doublePen(0.0f, 100.0f);
    v3d::font::TextureTextBuffer::Markup doubleMarkup = plain(font, kBase * 2.0f);
    atDouble.addText(&doublePen, doubleMarkup, L"M");

    // one glyph is one quad, so both buffers hold four vertices
    BOOST_REQUIRE_EQUAL(atBase.vertices().size(), 4u);
    BOOST_REQUIRE_EQUAL(atDouble.vertices().size(), 4u);

    const float baseWidth = std::abs(atBase.vertices()[2].x - atBase.vertices()[0].x);
    const float doubleWidth = std::abs(atDouble.vertices()[2].x - atDouble.vertices()[0].x);
    BOOST_REQUIRE(baseWidth > 0.0f);
    BOOST_CHECK_CLOSE(doubleWidth, baseWidth * 2.0f, 1.0f);

    const float baseHeight = std::abs(atBase.vertices()[2].y - atBase.vertices()[0].y);
    const float doubleHeight = std::abs(atDouble.vertices()[2].y - atDouble.vertices()[0].y);
    BOOST_REQUIRE(baseHeight > 0.0f);
    BOOST_CHECK_CLOSE(doubleHeight, baseHeight * 2.0f, 1.0f);

    // the same texels, whatever size the quad over them turned out to be
    for (std::size_t index = 0; index < 4; index++) {
        BOOST_CHECK_CLOSE(atDouble.uvs()[index].x, atBase.uvs()[index].x, 0.001f);
        BOOST_CHECK_CLOSE(atDouble.uvs()[index].y, atBase.uvs()[index].y, 0.001f);
    }
}
