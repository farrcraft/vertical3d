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
const float TextRenderer::baseSize = 48.0F;
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
    const glm::uvec2& atlas) :
    size_(size) {
    // a one channel atlas: the glyph's distance becomes its alpha, so text goes through the
    // quad shader. Subpixel (LCD) filtering would need dual source blending or a second
    // pass, and is not a distance field
    cache_ = boost::make_shared<v3d::font::TextureFontCache>(atlas.x, atlas.y, 1, logger);
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
    if (size <= 0.0F || size_ <= 0.0F) {
        return 1.0F;
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
    // a byte below this is a whole character on its own
    constexpr int singleByteLimit = 0x80;
    // the smallest code point each longer sequence may encode; anything below is overlong
    constexpr char32_t twoByteSmallest = 0x80;
    constexpr char32_t threeByteSmallest = 0x800;
    constexpr char32_t fourByteSmallest = 0x10000;
    // the same, indexed by sequence length
    constexpr std::array<char32_t, 5> minimum = {0, 0, twoByteSmallest, threeByteSmallest, fourByteSmallest};
    // a lead byte of each sequence length: the mask of its tag bits, the tag, and the mask of
    // the code point bits it carries
    constexpr int twoByteMask = 0xE0;
    constexpr int twoByteTag = 0xC0;
    constexpr int twoBytePayload = 0x1F;
    constexpr int threeByteMask = 0xF0;
    constexpr int threeByteTag = 0xE0;
    constexpr int threeBytePayload = 0x0F;
    constexpr int fourByteMask = 0xF8;
    constexpr int fourByteTag = 0xF0;
    constexpr int fourBytePayload = 0x07;
    // a continuation byte: the mask of its tag bits, the tag, and the six code point bits it
    // carries
    constexpr int continuationMask = 0xC0;
    constexpr int continuationTag = 0x80;
    constexpr int continuationBits = 6;
    constexpr int continuationPayload = 0x3F;
    // the largest code point, and the surrogate range, which no code point may fall in
    constexpr char32_t largestPoint = 0x10FFFF;
    constexpr char32_t firstSurrogate = 0xD800;
    constexpr char32_t lastSurrogate = 0xDFFF;

    const std::size_t start = *index;
    const unsigned char lead = static_cast<unsigned char>(utf8[start]);
    std::size_t length = 0;
    char32_t point = 0;
    if (lead < singleByteLimit) {
        length = 1;
        point = lead;
    } else if ((lead & twoByteMask) == twoByteTag) {
        length = 2;
        point = static_cast<char32_t>(lead & twoBytePayload);
    } else if ((lead & threeByteMask) == threeByteTag) {
        length = 3;
        point = static_cast<char32_t>(lead & threeBytePayload);
    } else if ((lead & fourByteMask) == fourByteTag) {
        length = 4;
        point = static_cast<char32_t>(lead & fourBytePayload);
    }

    bool valid = length > 0 && start + length <= utf8.size();
    for (std::size_t offset = 1; valid && offset < length; offset++) {
        const unsigned char following = static_cast<unsigned char>(utf8[start + offset]);
        if ((following & continuationMask) != continuationTag) {
            valid = false;
        } else {
            point = (point << continuationBits) | static_cast<char32_t>(following & continuationPayload);
        }
    }
    if (valid && (point < minimum.at(length) || point > largestPoint || (point >= firstSurrogate && point <= lastSurrogate))) {
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
        return 0.0F;
    }
    float width = 0.0F;
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
void TextRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, std::string_view text, const glm::vec2& pen, const glm::vec4& colour,
    float size) {
    if (text.empty() || canvas == nullptr || !loaded()) {
        return;
    }
    buffer_->clear();
    markup_.foregroundColor_ = colour;
    // the markup's size against the font's is what the layout scales its metrics by
    markup_.size_ = size > 0.0F ? size : size_;

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
Write TextRenderer::write(v3d::render::realtime::canvas::Canvas* canvas, float size) {
    return [this, canvas, size](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
        draw(canvas, text, pen, colour, size);
    };
}

};  // namespace v3d::ui::paint
