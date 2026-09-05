/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../Font2D.h"

namespace {
// the shared font, copied next to the executable by this directory's CMakeLists
const char* kTypeface = "data/fonts/NotoSans-Regular.ttf";
};  // namespace

BOOST_AUTO_TEST_CASE(font_properties_test) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::Font2D font(logger);

    font.size(12);
    BOOST_CHECK_EQUAL(font.size(), 12);

    font.style(static_cast<unsigned int>(v3d::font::Font2D::FontStyle::STYLE_BOLD));
    BOOST_CHECK_EQUAL(font.style(), static_cast<unsigned int>(v3d::font::Font2D::FontStyle::STYLE_BOLD));

    std::string fontface("Comic Sans");
    font.typeface(fontface);
    BOOST_CHECK_EQUAL(font.typeface(), fontface);

    // the typeface is a path freetype has to be able to open, so a face name alone fails
    BOOST_CHECK_EQUAL(font.build(), false);

    // and a font that has not been built has no glyphs to hand out
    BOOST_CHECK(font.glyph(static_cast<unsigned char>('a')) == nullptr);
    BOOST_CHECK_EQUAL(font.width(std::string("hello")), 0u);
}

BOOST_AUTO_TEST_CASE(font_build_test) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::Font2D font(kTypeface, 12, logger);

    BOOST_REQUIRE_EQUAL(font.build(), true);
    BOOST_CHECK_EQUAL(font.typeface(), std::string(kTypeface));

    // building fills the atlas texture and the line height
    boost::shared_ptr<v3d::image::Texture> texture = font.texture();
    BOOST_REQUIRE(texture != nullptr);
    BOOST_CHECK_EQUAL(texture->width(), 256u);
    BOOST_CHECK(texture->height() > 0u);
    BOOST_CHECK(font.height() > 0u);

    // the line height as a fraction of the texture is what the renderer steps by
    BOOST_CHECK_CLOSE(font.textureHeight(), static_cast<float>(font.height()) / texture->height(), 0.01f);

    // every glyph in the charset has an advance, and a wider letter advances further
    BOOST_CHECK(font.width(static_cast<unsigned char>('w')) > 0u);
    BOOST_CHECK(font.width(static_cast<unsigned char>('w')) > font.width(static_cast<unsigned char>('i')));

    // a string's width is the sum of its glyph advances
    std::string text("hello");
    unsigned int sum = 0;
    for (char letter : text) {
        sum += font.width(static_cast<unsigned char>(letter));
    }
    BOOST_CHECK_EQUAL(font.width(text), sum);

    // a character outside the charset has no advance. glyph() does not agree with width()
    // about that - rather than returning nothing it falls back to the first glyph in the
    // map, so an unknown character draws as whatever sorts first
    BOOST_CHECK(font.glyph(static_cast<unsigned char>('a')) != nullptr);
    BOOST_CHECK_EQUAL(font.width(static_cast<unsigned char>(0x01)), 0u);
    BOOST_CHECK(font.glyph(0x01) != nullptr);

    // glyph texture coordinates stay inside the atlas
    const v3d::font::Font2D::Glyph* glyph = font.glyph(static_cast<unsigned char>('a'));
    BOOST_REQUIRE(glyph != nullptr);
    BOOST_CHECK(glyph->x1_ >= 0.0f);
    BOOST_CHECK(glyph->x2_ <= 1.0f);
    BOOST_CHECK(glyph->x2_ > glyph->x1_);
    BOOST_CHECK(glyph->y1_ >= 0.0f);
    BOOST_CHECK(glyph->y1_ <= 1.0f);
}

BOOST_AUTO_TEST_CASE(font_missing_file_test) {
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::font::Font2D font("data/fonts/DoesNotExist.ttf", 12, logger);
    BOOST_CHECK_EQUAL(font.build(), false);
}
