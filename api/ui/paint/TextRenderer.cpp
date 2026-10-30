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

const char* const TextRenderer::defaultFont = "fonts/Inter-Regular.ttf";
const char* const TextRenderer::semiBoldFont = "fonts/Inter-SemiBold.ttf";
const char* const TextRenderer::fallbackFont = "fonts/NotoSans-Regular.ttf";

// found by packing printable ascii, not by arithmetic. 48 with a spread of 8 fits one face in
// a 512 atlas with little to spare, and two faces need twice the room. A charset or a base
// that needs more passes larger atlas dimensions
const float TextRenderer::baseSize = 48.0F;
const unsigned int TextRenderer::defaultSpread = 8;
const unsigned int TextRenderer::defaultAtlas = 1024;

/**
 **/
TextRenderer::Faces::Faces() :
regular(defaultFont),
semiBold(semiBoldFont),
fallback(fallbackFont) {
}

/**
 **/
TextRenderer::TextRenderer(const boost::shared_ptr<v3d::asset::Manager>& assetManager,
    const boost::shared_ptr<v3d::log::Logger>& logger,
    const Upload& upload,
    float size,
    const Faces& faces,
    std::string_view charset,
    unsigned int spread,
    const glm::uvec2& atlas) :
    size_(size) {
    // a one channel atlas: the glyph's distance becomes its alpha, so text goes through the
    // quad shader. Subpixel (LCD) filtering would need dual source blending or a second
    // pass, and is not a distance field
    cache_ = boost::make_shared<v3d::font::TextureFontCache>(atlas.x, atlas.y, 1, logger);
    markup_.size_ = size_;

    // each face is opened here, not loaded as an asset, because this renderer chooses the
    // size it is rasterized at. The manager only supplies the file's path
    const auto open = [&](const std::string& file) {
        const boost::shared_ptr<v3d::font::TextureFont> opened =
            boost::make_shared<v3d::font::TextureFont>(assetManager->path(file), size_, logger, spread);
        opened->atlas(cache_->atlas());
        return opened;
    };

    boost::shared_ptr<v3d::font::TextureFont> fallback;
    if (!faces.fallback.empty()) {
        fallback = open(faces.fallback);
        // packing nothing opens the file, so a fallback that will not load is found here
        // rather than as glyphs that are silently missing
        if (!fallback->loadGlyphs(std::string_view())) {
            logger->get()->warn("{} could not be loaded, so a character the faces lack draws as their missing glyph", faces.fallback);
            fallback.reset();
        }
    }

    regular_ = open(faces.regular);
    regular_->fallback(fallback);
    if (!regular_->loadGlyphs(charset)) {
        // a face that would not open packs nothing, and one that opened into an atlas too
        // small packs only some glyphs. Either way nothing is drawn, so no text is shown
        // with characters missing
        logger->get()->error("{} could not be packed at size {}, so nothing drawn through it will have text", faces.regular, size_);
        regular_.reset();
        return;
    }
    // the opaque white square a line or a background is drawn with. The text buffer looks it
    // up with every glyph, so it is packed now, before the upload
    regular_->glyph(v3d::font::TextureFont::SOLID);
    cache_->add(regular_);

    if (!faces.semiBold.empty()) {
        semiBold_ = open(faces.semiBold);
        semiBold_->fallback(fallback);
        if (semiBold_->loadGlyphs(charset)) {
            semiBold_->glyph(v3d::font::TextureFont::SOLID);
            cache_->add(semiBold_);
        } else {
            logger->get()->warn("{} could not be packed at size {}, so semibold text is drawn in {}", faces.semiBold, size_, faces.regular);
            semiBold_.reset();
        }
    }
    markup_.font_ = regular_;

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
float TextRenderer::width(std::string_view text, float size, Weight weight) const {
    return loaded() ? face(weight)->width(text, size) : 0.0F;
}

/**
 **/
const boost::shared_ptr<v3d::font::TextureFont>& TextRenderer::face(Weight weight) const noexcept {
    return weight == Weight::SemiBold && semiBold_ ? semiBold_ : regular_;
}

/**
 **/
void TextRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, std::string_view text, const glm::vec2& pen, const glm::vec4& colour,
    float size, Weight weight) {
    if (text.empty() || canvas == nullptr || !loaded()) {
        return;
    }
    buffer_->clear();
    markup_.foregroundColor_ = colour;
    markup_.font_ = face(weight);
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
Measure TextRenderer::measure(float size, Weight weight) const {
    return [this, size, weight](std::string_view text) -> float {
        return width(text, size, weight);
    };
}

/**
 **/
Write TextRenderer::write(v3d::render::realtime::canvas::Canvas* canvas, float size, Weight weight) {
    return [this, canvas, size, weight](std::string_view text, const glm::vec2& pen, const glm::vec4& colour) {
        draw(canvas, text, pen, colour, size, weight);
    };
}

};  // namespace v3d::ui::paint
