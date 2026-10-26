/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextRenderer.h"

#include <api/asset/Manager.h>
#include <api/font/TextureFont.h>
#include <api/font/TextureFontCache.h>
#include <api/font/TextureTextBuffer.h>
#include <api/image/TextureAtlas.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>

#include "Text.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::ui::paint {

const char* const TextRenderer::ascii =
" !\"#$%&'()*+,-./0123456789:;<=>?"
"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
"`abcdefghijklmnopqrstuvwxyz{|}~";

const char* const TextRenderer::defaultFont = "fonts/NotoSans-Regular.ttf";

// found by packing printable ascii, not by arithmetic. 48 with a spread of 8 is the largest
// pair tried that fits a 512 atlas: 48 with 12 does not, and 64 with 8 does not. A charset
// or a base that needs more passes larger atlas dimensions
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
    std::string_view charset,
    unsigned int spread,
    const glm::uvec2& atlas) :
    size_(size) {
    // a one channel atlas: the glyph's distance becomes its alpha, so text goes through the
    // quad shader. Subpixel (LCD) filtering would need dual source blending or a second
    // pass, and is not a distance field
    cache_ = boost::make_shared<v3d::font::TextureFontCache>(atlas.x, atlas.y, 1, logger);
    markup_.size_ = size_;

    // the face is opened here, not loaded as an asset, because this renderer chooses the
    // size it is rasterized at. The manager only supplies the file's path
    const boost::shared_ptr<v3d::font::TextureFont> face =
        boost::make_shared<v3d::font::TextureFont>(assetManager->path(font), markup_.size_, logger, spread);
    face->atlas(cache_->atlas());
    if (!face->loadGlyphs(charset)) {
        // a face that would not open packs nothing, and one that opened into an atlas too
        // small packs only some glyphs. Either way nothing is drawn, so no text is shown
        // with characters missing
        logger->get()->error("{} could not be packed at size {}, so nothing drawn through it will have text", font, size_);
        return;
    }
    // the opaque white square a line or a background is drawn with. The text buffer looks it
    // up with every glyph, so it is packed now, before the upload
    face->glyph(v3d::font::TextureFont::SOLID);
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
float TextRenderer::width(std::string_view text, float size) const {
    return loaded() ? markup_.font_->width(text, size) : 0.0F;
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

    // the layout draws only packed glyphs, so it never adds to the atlas after the upload
    glm::vec2 cursor = pen;
    buffer_->addText(&cursor, markup_, text);

    // glyphs are laid out in three dimensions. A 2D canvas has no use for the third
    const std::vector<glm::vec3>& positions = buffer_->vertices();
    const std::vector<glm::vec2>& uvs = buffer_->uvs();
    const std::vector<glm::vec4>& colours = buffer_->colors();
    vertices_.clear();
    for (std::size_t index = 0; index < positions.size(); index++) {
        vertices_.push_back({.position = glm::vec2(positions[index]), .uv = uvs[index], .colour = colours[index]});
    }
    indices_.assign(buffer_->indices().begin(), buffer_->indices().end());
    canvas->text(vertices_, indices_, atlas_);
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
