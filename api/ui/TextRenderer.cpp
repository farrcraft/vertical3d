/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextRenderer.h"

#include <string>

#include "../asset/TextureFont.h"
#include "../asset/Type.h"
#include "../image/TextureAtlas.h"
#include "../render/realtime/vulkan/QuadRenderer.h"

#include <boost/make_shared.hpp>

namespace v3d::ui {

const wchar_t* const TextRenderer::ascii =
L" !\"#$%&'()*+,-./0123456789:;<=>?"
L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
L"`abcdefghijklmnopqrstuvwxyz{|}~";

const char* const TextRenderer::defaultFont = "fonts/NotoSans-Regular.ttf";

/**
 **/
TextRenderer::TextRenderer(const boost::shared_ptr<v3d::asset::Manager>& assetManager,
    const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::render::realtime::vulkan::QuadRenderer>& quads,
    float size,
    const std::string& font,
    const wchar_t* charcodes) :
    size_(size) {
    // a one channel atlas: the glyph's coverage becomes its alpha, which is what lets text
    // go through the quad shader. Subpixel (LCD) filtering would need dual source blending
    // or a second pass
    cache_ = boost::make_shared<v3d::font::TextureFontCache>(512, 512, v3d::font::TextureTextBuffer::LCD_FILTERING_OFF, logger);
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
    boost::shared_ptr<v3d::asset::TextureFont> asset = boost::dynamic_pointer_cast<v3d::asset::TextureFont>(
        assetManager->load(font, v3d::asset::Type::TextureFont));
    if (!asset || !asset->font()) {
        logger->get()->error("the font {} could not be loaded, so nothing drawn through it will have text", font);
        return;
    }

    asset->font()->atlas(cache_->atlas());
    asset->font()->loadGlyphs(charcodes);
    cache_->add(asset->font());
    markup_.font_ = asset->font();

    // every glyph is packed by now, so the atlas can go to the device once and stay there
    atlas_ = quads->texture(cache_->atlas()->image());

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
float TextRenderer::width(const std::string& text) const {
    if (!markup_.font_) {
        return 0.0f;
    }
    float width = 0.0f;
    for (char character : text) {
        boost::shared_ptr<v3d::font::TextureFont::Glyph> glyph = markup_.font_->glyph(static_cast<wchar_t>(character));
        if (glyph) {
            width += glyph->advance_.x;
        }
    }
    return width;
}

/**
 **/
void TextRenderer::draw(v3d::render::realtime::Canvas* canvas, const std::string& text, const glm::vec2& pen, const glm::vec4& colour) {
    if (text.empty() || !canvas || !loaded()) {
        return;
    }
    buffer_->clear();
    markup_.foregroundColor_ = colour;

    glm::vec2 cursor = pen;
    const std::wstring wide(text.begin(), text.end());
    buffer_->addText(&cursor, markup_, wide);

    canvas->text(*buffer_, atlas_);
}

/**
 **/
ComponentRenderer::Measure TextRenderer::measure() const {
    return [this](const std::string& text) -> float {
        return width(text);
    };
}

/**
 **/
ComponentRenderer::Write TextRenderer::write(v3d::render::realtime::Canvas* canvas) {
    return [this, canvas](const std::string& text, const glm::vec2& pen, const glm::vec4& colour) {
        draw(canvas, text, pen, colour);
    };
}

};  // namespace v3d::ui
