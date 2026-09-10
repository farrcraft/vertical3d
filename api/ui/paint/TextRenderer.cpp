/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextRenderer.h"

#include <api/asset/Type.h>
#include <api/asset/kind/TextureFont.h>
#include <api/image/TextureAtlas.h>

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
// does not, and 64 and 8 does not. So the atlas the tree has always used stays the default
// and the dimensions are an argument for the charset or the base that needs more
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
    // a one channel atlas: the glyph's distance becomes its alpha, which is what lets text
    // go through the quad shader. Subpixel (LCD) filtering would need dual source blending
    // or a second pass, and is not a distance field
    cache_ = boost::make_shared<v3d::font::TextureFontCache>(atlasWidth, atlasHeight, v3d::font::TextureTextBuffer::LCD_FILTERING_OFF, logger);
    cache_->charcodes(charcodes);

    markup_.family_ = "sans";
    markup_.bold_ = false;
    markup_.italic_ = false;
    markup_.rise_ = 0.0f;
    markup_.spacing_ = 0.0f;
    markup_.gamma_ = 1.0f;
    markup_.outline_ = false;
    markup_.underline_ = false;
    markup_.overline_ = false;
    markup_.strikethrough_ = false;
    markup_.foregroundColor_ = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    // transparent, so no background quad is emitted behind each glyph
    markup_.backgroundColor_ = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    markup_.size_ = size_;

    boost::shared_ptr<v3d::asset::Loader> loader = assetManager->resolveLoader(v3d::asset::Type::TextureFont);
    v3d::asset::ParameterValue value = markup_.size_;
    loader->parameter("fontSize", value);
    v3d::asset::ParameterValue field = static_cast<float>(spread);
    loader->parameter("spread", field);
    boost::shared_ptr<v3d::asset::kind::TextureFont> asset = boost::dynamic_pointer_cast<v3d::asset::kind::TextureFont>(
        assetManager->load(font, v3d::asset::Type::TextureFont));
    if (!asset || !asset->font()) {
        logger->get()->error("the font {} could not be loaded, so nothing drawn through it will have text", font);
        return;
    }

    asset->font()->atlas(cache_->atlas());
    if (!asset->font()->loadGlyphs(charcodes)) {
        // the font itself is fine and the atlas is not - drawing what did fit would be
        // text with characters missing, measured short, laid out around the short measure
        logger->get()->error("the atlas could not hold {} at size {}, so nothing drawn through it will have text", font, size_);
        return;
    }
    cache_->add(asset->font());
    markup_.font_ = asset->font();

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
float TextRenderer::width(std::string_view text, float size) const {
    if (!loaded()) {
        return 0.0f;
    }
    float width = 0.0f;
    for (char character : text) {
        boost::shared_ptr<v3d::font::TextureFont::Glyph> glyph = markup_.font_->glyph(static_cast<wchar_t>(character));
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

    glm::vec2 cursor = pen;
    const std::wstring wide(text.begin(), text.end());
    buffer_->addText(&cursor, markup_, wide);

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
