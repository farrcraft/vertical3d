/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextRenderer.h"

#include <api/font/TextureFont.h>
#include <api/image/TextureAtlas.h>

#include <array>
#include <limits>
#include <string>
#include <string_view>

#include <boost/make_shared.hpp>

namespace v3d::ui::paint {

const wchar_t* const TextRenderer::ascii =
L" !\"#$%&'()*+,-./0123456789:;<=>?"
L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
L"`abcdefghijklmnopqrstuvwxyz{|}~";

const char* const TextRenderer::defaultFont = "fonts/NotoSans-Regular.ttf";

// chosen by packing printable ascii and seeing what fit rather than by arithmetic. 48 with
// a spread of 8 is the largest of the pairs tried that still fits a 512 atlas: 48 and 12
// does not, and 64 and 8 does not. A charset or a base that needs more passes larger
// atlas dimensions
const float TextRenderer::baseSize = 48.0f;
const unsigned int TextRenderer::defaultSpread = 8;
const unsigned int TextRenderer::defaultAtlas = 512;

/**
 **/
TextRenderer::TextRenderer(const boost::shared_ptr<v3d::asset::Manager>& assetManager,
    const boost::shared_ptr<v3d::log::Logger>& logger,
    const Upload& upload,
    float size,
    const std::string& font,
    const wchar_t* charcodes,
    unsigned int spread,
    unsigned int atlasWidth,
    unsigned int atlasHeight) :
    size_(size) {
    // a one channel atlas: the glyph's distance becomes its alpha, so text goes through the
    // quad shader. Subpixel (LCD) filtering would need dual source blending or a second
    // pass, and is not a distance field
    cache_ = boost::make_shared<v3d::font::TextureFontCache>(atlasWidth, atlasHeight, 1, logger);
    markup_.size_ = size_;

    // the face is opened here rather than loaded as an asset: what it is rasterized at is this
    // renderer's to say, and the manager only says where the file is
    const boost::shared_ptr<v3d::font::TextureFont> face =
        boost::make_shared<v3d::font::TextureFont>(assetManager->path(font), markup_.size_, logger, spread);
    face->atlas(cache_->atlas());
    if (!face->loadGlyphs(charcodes)) {
        // a face that would not open packs nothing, and one that opened into an atlas too
        // small packs some. Drawing what did fit would be text with characters missing,
        // measured short, laid out around the short measure
        logger->get()->error("{} could not be packed at size {}, so nothing drawn through it will have text", font, size_);
        return;
    }
    // the opaque white square a line or a background is drawn with. The text buffer asks
    // for it with every glyph, so it is packed now rather than after the upload
    face->glyph(static_cast<wchar_t>(-1));
    cache_->add(face);
    markup_.font_ = face;

    // every glyph is packed by now, so the atlas can go to the device once and stay there
    atlas_ = upload(cache_->atlas()->image());

    buffer_ = boost::make_shared<v3d::font::TextureTextBuffer>();
}

/**
 **/
bool TextRenderer::loaded() const noexcept {
    return markup_.font_ && buffer_;
}

/**
 **/
float TextRenderer::size() const noexcept {
    return size_;
}

/**
 **/
float TextRenderer::ratio(float size) const noexcept {
    if (size <= 0.0f || size_ <= 0.0f) {
        return 1.0f;
    }
    return size / size_;
}

/**
 **/
std::u32string TextRenderer::decode(std::string_view utf8) {
    std::u32string decoded;
    decoded.reserve(utf8.size());
    std::size_t index = 0;
    while (index < utf8.size()) {
        decoded.push_back(next(utf8, &index));
    }
    return decoded;
}

/**
 **/
char32_t TextRenderer::next(std::string_view utf8, std::size_t* index) {
    const char32_t replacement = 0xFFFD;
    // the smallest code point each sequence length may encode; anything below is overlong
    const std::array<char32_t, 5> minimum = {0, 0, 0x80, 0x800, 0x10000};

    const std::size_t start = *index;
    const unsigned char lead = static_cast<unsigned char>(utf8[start]);
    std::size_t length = 0;
    char32_t point = 0;
    if (lead < 0x80) {
        length = 1;
        point = lead;
    } else if ((lead & 0xE0) == 0xC0) {
        length = 2;
        point = static_cast<char32_t>(lead & 0x1F);
    } else if ((lead & 0xF0) == 0xE0) {
        length = 3;
        point = static_cast<char32_t>(lead & 0x0F);
    } else if ((lead & 0xF8) == 0xF0) {
        length = 4;
        point = static_cast<char32_t>(lead & 0x07);
    }

    bool valid = length > 0 && start + length <= utf8.size();
    for (std::size_t offset = 1; valid && offset < length; offset++) {
        const unsigned char following = static_cast<unsigned char>(utf8[start + offset]);
        if ((following & 0xC0) != 0x80) {
            valid = false;
        } else {
            point = (point << 6) | static_cast<char32_t>(following & 0x3F);
        }
    }
    if (valid && (point < minimum.at(length) || point > 0x10FFFF || (point >= 0xD800 && point <= 0xDFFF))) {
        valid = false;
    }

    if (!valid) {
        *index = start + 1;
        return replacement;
    }
    *index = start + length;
    return point;
}

/**
 **/
boost::shared_ptr<v3d::font::TextureFont::Glyph> TextRenderer::packed(char32_t point) const {
    // a code point wider than wchar_t cannot have been packed, since charcodes are wchar_t
    if (point > static_cast<char32_t>(std::numeric_limits<wchar_t>::max())) {
        return boost::shared_ptr<v3d::font::TextureFont::Glyph>();
    }
    const wchar_t character = static_cast<wchar_t>(point);
    // -1 names the white square lines are drawn with, which is not a character of text
    if (character == static_cast<wchar_t>(-1)) {
        return boost::shared_ptr<v3d::font::TextureFont::Glyph>();
    }
    return markup_.font_->packed(character);
}

/**
 **/
float TextRenderer::width(std::string_view text, float size) const {
    if (!loaded()) {
        return 0.0f;
    }
    float width = 0.0f;
    std::size_t index = 0;
    while (index < text.size()) {
        const boost::shared_ptr<v3d::font::TextureFont::Glyph> glyph = packed(next(text, &index));
        if (glyph) {
            width += glyph->advance_.x;
        }
    }
    return width * ratio(size);
}

/**
 **/
void TextRenderer::draw(v3d::render::realtime::Canvas* canvas, std::string_view text, const glm::vec2& pen, const glm::vec4& colour,
    float size) {
    if (text.empty() || !canvas || !loaded()) {
        return;
    }
    buffer_->clear();
    markup_.foregroundColor_ = colour;
    // the markup's size against the font's is what the layout scales its metrics by
    markup_.size_ = size > 0.0f ? size : size_;

    // only code points with a packed glyph reach the layout, so it never packs one after
    // the upload. A newline has no glyph and is kept, since it moves the pen
    std::wstring drawable;
    drawable.reserve(text.size());
    std::size_t index = 0;
    while (index < text.size()) {
        const char32_t point = next(text, &index);
        if (point == U'\n' || packed(point)) {
            drawable.push_back(static_cast<wchar_t>(point));
        }
    }

    glm::vec2 cursor = pen;
    buffer_->addText(&cursor, markup_, drawable);

    canvas->text(*buffer_, atlas_);
}

/**
 **/
Measure TextRenderer::measure(float size) const {
    return [this, size](std::string_view text) -> float {
        return width(text, size);
    };
}

/**
 **/
Write TextRenderer::write(v3d::render::realtime::Canvas* canvas, float size) {
    return [this, canvas, size](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
        draw(canvas, text, pen, colour, size);
    };
}

};  // namespace v3d::ui::paint
