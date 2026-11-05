/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/texture/Font.h>
#include <api/font/texture/FontCache.h>
#include <api/font/texture/TextBuffer.h>
#include <api/log/Logger.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

namespace {

const char* const kTypeface = "data/fonts/NotoSans-Regular.ttf";

const float kBase = 48.0F;
const unsigned int kSpread = 8;

/**
 * A markup drawing plain text in the given face at the given size.
 *
 * Every decoration is off, because what these cases are about is the glyph quad and the
 * pen, and a background or an underline would put quads of its own into the buffer.
 **/
v3d::font::texture::TextBuffer::Markup plain(const boost::shared_ptr<v3d::font::texture::Font>& font, float size) {
    v3d::font::texture::TextBuffer::Markup markup;
    markup.size_ = size;
    markup.gamma_ = 1.0F;
    markup.underline_ = false;
    markup.overline_ = false;
    markup.strikethrough_ = false;
    markup.foregroundColor_ = glm::vec4(1.0F, 1.0F, 1.0F, 1.0F);
    markup.backgroundColor_ = glm::vec4(0.0F, 0.0F, 0.0F, 0.0F);
    markup.font_ = font;
    return markup;
}

/**
 * How far a line of text laid out at a size reaches from where the pen started.
 **/
float advanceOf(const boost::shared_ptr<v3d::font::texture::Font>& font, const std::string& text, float size) {
    v3d::font::texture::TextBuffer buffer;
    glm::vec2 pen(0.0F, 0.0F);
    const v3d::font::texture::TextBuffer::Markup markup = plain(font, size);
    buffer.addText(&pen, markup, text);
    return pen.x;
}

/**
 * One atlas, drawn at more than one size.
 *
 * Checked without a device: the glyph metrics are in pixels of the size the face was
 * rasterized at, so a markup with another size lays out at a ratio of them. Twice the base
 * size advances the pen twice as far and produces a quad twice as large, from the same
 * glyphs in the same atlas.
 **/
BOOST_AUTO_TEST_CASE(texturetextbuffer_scales_to_the_markup_size_test) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::texture::FontCache cache(512, 512, 1, logger);

    const boost::shared_ptr<v3d::font::texture::Font> font =
        boost::make_shared<v3d::font::texture::Font>(std::string(kTypeface), kBase, logger, kSpread);
    font->atlas(cache.atlas());
    BOOST_REQUIRE_EQUAL(font->loadGlyphs("Hamburgefonstiv"), true);

    const std::string line = "Hamburgefonstiv";

    const float base = advanceOf(font, line, kBase);
    BOOST_REQUIRE(base > 0.0F);

    // the pen advances in proportion, which a layout that measures a string relies on
    BOOST_CHECK_CLOSE(advanceOf(font, line, kBase * 2.0F), base * 2.0F, 0.01F);
    BOOST_CHECK_CLOSE(advanceOf(font, line, kBase * 0.5F), base * 0.5F, 0.01F);

    // a markup with no size set, which is zero, uses the base size
    BOOST_CHECK_CLOSE(advanceOf(font, line, 0.0F), base, 0.01F);
}

/**
 * The quad a glyph is drawn into scales with the size, and its atlas coordinates do not.
 *
 * One atlas serves every size because the glyph is sampled from the same place however
 * large it is drawn.
 **/
BOOST_AUTO_TEST_CASE(texturetextbuffer_quad_scales_and_the_atlas_does_not_test) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::texture::FontCache cache(512, 512, 1, logger);

    const boost::shared_ptr<v3d::font::texture::Font> font =
        boost::make_shared<v3d::font::texture::Font>(std::string(kTypeface), kBase, logger, kSpread);
    font->atlas(cache.atlas());
    BOOST_REQUIRE_EQUAL(font->loadGlyphs("M"), true);

    v3d::font::texture::TextBuffer atBase;
    glm::vec2 basePen(0.0F, 100.0F);
    const v3d::font::texture::TextBuffer::Markup baseMarkup = plain(font, kBase);
    atBase.addText(&basePen, baseMarkup, "M");

    v3d::font::texture::TextBuffer atDouble;
    glm::vec2 doublePen(0.0F, 100.0F);
    const v3d::font::texture::TextBuffer::Markup doubleMarkup = plain(font, kBase * 2.0F);
    atDouble.addText(&doublePen, doubleMarkup, "M");

    // one glyph is one quad, so both buffers hold four vertices
    BOOST_REQUIRE_EQUAL(atBase.vertices().size(), 4U);
    BOOST_REQUIRE_EQUAL(atDouble.vertices().size(), 4U);

    const float baseWidth = std::abs(atBase.vertices()[2].x - atBase.vertices()[0].x);
    const float doubleWidth = std::abs(atDouble.vertices()[2].x - atDouble.vertices()[0].x);
    BOOST_REQUIRE(baseWidth > 0.0F);
    BOOST_CHECK_CLOSE(doubleWidth, baseWidth * 2.0F, 1.0F);

    const float baseHeight = std::abs(atBase.vertices()[2].y - atBase.vertices()[0].y);
    const float doubleHeight = std::abs(atDouble.vertices()[2].y - atDouble.vertices()[0].y);
    BOOST_REQUIRE(baseHeight > 0.0F);
    BOOST_CHECK_CLOSE(doubleHeight, baseHeight * 2.0F, 1.0F);

    // the same texels, whatever size the quad over them turned out to be
    for (std::size_t index = 0; index < 4; index++) {
        BOOST_CHECK_CLOSE(atDouble.uvs()[index].x, atBase.uvs()[index].x, 0.001F);
        BOOST_CHECK_CLOSE(atDouble.uvs()[index].y, atBase.uvs()[index].y, 0.001F);
    }
}

/**
 * A newline in a markup with no size moves the pen down one line at the font's own size. The
 * font's height is the whole line, from one baseline to the next.
 *
 * A new markup's size is zero, which means the font's own size, so it lays out the same
 * lines as a markup set to the base size. Each line starts back at the pen's first x.
 **/
BOOST_AUTO_TEST_CASE(texturetextbuffer_newline_in_a_default_markup_moves_one_line_test) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::texture::FontCache cache(512, 512, 1, logger);

    const boost::shared_ptr<v3d::font::texture::Font> font =
        boost::make_shared<v3d::font::texture::Font>(std::string(kTypeface), kBase, logger, kSpread);
    font->atlas(cache.atlas());
    BOOST_REQUIRE_EQUAL(font->loadGlyphs("ab"), true);

    const float line = font->height();
    BOOST_REQUIRE(line > 0.0F);

    v3d::font::texture::TextBuffer unsized;
    glm::vec2 unsizedPen(10.0F, 100.0F);
    v3d::font::texture::TextBuffer::Markup markup;
    markup.font_ = font;
    unsized.addText(&unsizedPen, markup, "a\nb");

    BOOST_CHECK_CLOSE(unsizedPen.y, 100.0F + line, 0.01F);
    BOOST_CHECK_CLOSE(unsizedPen.x, 10.0F + font->glyph(U'b')->advance_.x, 0.01F);

    v3d::font::texture::TextBuffer sized;
    glm::vec2 sizedPen(10.0F, 100.0F);
    const v3d::font::texture::TextBuffer::Markup baseMarkup = plain(font, kBase);
    sized.addText(&sizedPen, baseMarkup, "a\nb");

    BOOST_CHECK_CLOSE(unsizedPen.y, sizedPen.y, 0.01F);
}

/**
 * Every glyph of a line sits on one baseline, the pen's y rounded to a whole pixel, and each
 * quad is its glyph's bitmap at the markup's size exactly, at a fractional size and pen.
 **/
BOOST_AUTO_TEST_CASE(texturetextbuffer_glyphs_share_one_baseline_test) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::texture::FontCache cache(512, 512, 1, logger);

    const boost::shared_ptr<v3d::font::texture::Font> font =
        boost::make_shared<v3d::font::texture::Font>(std::string(kTypeface), kBase, logger, kSpread);
    font->atlas(cache.atlas());
    const std::string line = "Hamburgefonstiv";
    BOOST_REQUIRE_EQUAL(font->loadGlyphs(line), true);

    const float size = 12.3F;
    const float scale = size / kBase;
    v3d::font::texture::TextBuffer buffer;
    glm::vec2 pen(3.0F, 100.4F);
    buffer.addText(&pen, plain(font, size), line);
    BOOST_REQUIRE_EQUAL(buffer.vertices().size(), line.size() * 4U);

    for (std::size_t index = 0; index < line.size(); index++) {
        const boost::shared_ptr<v3d::font::texture::Font::Glyph> glyph = font->packed(static_cast<char32_t>(line[index]));
        BOOST_REQUIRE(glyph);
        // a quad's first vertex is its bottom left, and its second its top left
        const float bottom = buffer.vertices()[index * 4].y;
        const float top = buffer.vertices()[(index * 4) + 1].y;
        const float baseline = bottom - ((static_cast<float>(glyph->height_) - static_cast<float>(glyph->offset_.y)) * scale);
        BOOST_CHECK_SMALL(baseline - 100.0F, 1e-3F);
        BOOST_CHECK_CLOSE(bottom - top, static_cast<float>(glyph->height_) * scale, 0.01F);
    }
}

/**
 * A background is one quad behind a whole line, drawn before its glyphs, and an underline is
 * one quad along it, drawn after them below the baseline. A newline starts both again.
 **/
BOOST_AUTO_TEST_CASE(texturetextbuffer_lines_are_drawn_per_line_test) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::texture::FontCache cache(512, 512, 1, logger);

    const boost::shared_ptr<v3d::font::texture::Font> font =
        boost::make_shared<v3d::font::texture::Font>(std::string(kTypeface), kBase, logger, kSpread);
    font->atlas(cache.atlas());
    BOOST_REQUIRE_EQUAL(font->loadGlyphs("abc"), true);
    BOOST_REQUIRE(font->glyph(v3d::font::texture::Font::SOLID));

    v3d::font::texture::TextBuffer::Markup markup = plain(font, kBase);
    markup.backgroundColor_ = glm::vec4(0.0F, 0.0F, 1.0F, 1.0F);
    markup.underline_ = true;
    markup.underlineColor_ = glm::vec4(0.0F, 1.0F, 0.0F, 1.0F);
    v3d::font::texture::TextBuffer buffer;
    glm::vec2 pen(10.0F, 100.0F);
    buffer.addText(&pen, markup, "abc\nab");

    // a background, three glyphs and an underline, then a background, two glyphs and an underline
    BOOST_REQUIRE_EQUAL(buffer.vertices().size(), (5U + 4U) * 4U);
    const float width = font->width("abc");
    BOOST_CHECK_CLOSE(buffer.vertices()[2].x - buffer.vertices()[0].x, width, 0.01F);
    BOOST_CHECK(buffer.colors()[0] == markup.backgroundColor_);
    const std::size_t underline = std::size_t{ 4 } * 4;
    BOOST_CHECK_CLOSE(buffer.vertices()[underline + 2].x - buffer.vertices()[underline].x, width, 0.01F);
    BOOST_CHECK(std::min(buffer.vertices()[underline].y, buffer.vertices()[underline + 1].y) > 100.0F);
    BOOST_CHECK(buffer.colors()[underline] == markup.underlineColor_);
}

/**
 * The layout draws only glyphs already packed. A character that is not draws nothing, moves
 * the pen by nothing, and is not packed by being laid out.
 **/
BOOST_AUTO_TEST_CASE(texturetextbuffer_lays_out_only_packed_glyphs_test) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::texture::FontCache cache(512, 512, 1, logger);

    const boost::shared_ptr<v3d::font::texture::Font> font =
        boost::make_shared<v3d::font::texture::Font>(std::string(kTypeface), kBase, logger, kSpread);
    font->atlas(cache.atlas());
    BOOST_REQUIRE_EQUAL(font->loadGlyphs("a"), true);

    v3d::font::texture::TextBuffer buffer;
    glm::vec2 pen(0.0F, 0.0F);
    buffer.addText(&pen, plain(font, kBase), "aza");

    BOOST_CHECK_CLOSE(pen.x, font->packed(U'a')->advance_.x * 2.0F, 0.01F);
    BOOST_CHECK_EQUAL(buffer.vertices().size(), 8U);
    BOOST_CHECK(font->packed(U'z') == nullptr);
}

/**
 * A font measures a UTF-8 string as far as the layout moves the pen, at any size, and a
 * character it has not packed adds nothing.
 **/
BOOST_AUTO_TEST_CASE(texturefont_width_matches_the_layout_test) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::texture::FontCache cache(512, 512, 1, logger);

    const boost::shared_ptr<v3d::font::texture::Font> font =
        boost::make_shared<v3d::font::texture::Font>(std::string(kTypeface), kBase, logger, kSpread);
    font->atlas(cache.atlas());
    // e with an acute accent, U+00E9, as UTF-8, and the euro sign, U+20AC, which is not packed
    const std::string line = "Hamburgefonstiv\xC3\xA9";
    BOOST_REQUIRE_EQUAL(font->loadGlyphs(line), true);

    BOOST_CHECK_CLOSE(font->width(line), advanceOf(font, line, kBase), 0.01F);
    BOOST_CHECK_CLOSE(font->width(line, kBase * 2.0F), advanceOf(font, line, kBase * 2.0F), 0.01F);
    BOOST_CHECK_CLOSE(font->width(line + "\xE2\x82\xAC"), font->width(line), 0.01F);
}

};  // namespace
