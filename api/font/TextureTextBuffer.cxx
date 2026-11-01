/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureTextBuffer.h"

#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "TextBuffer.h"
#include "TextureFont.h"
#include "Utf8.h"

#include <glm/gtc/matrix_transform.hpp>

namespace v3d::font {

TextureTextBuffer::TextureTextBuffer() = default;

void TextureTextBuffer::addLine(glm::vec2* pen, const Markup& markup, std::u32string_view line) {
    // every metric below is in pixels of the size the face was rasterized at, so another
    // size scales them by the ratio. A markup that sets no size, or the font's own, leaves
    // this at one
    const float scale = markup.size_ > 0.0F && markup.font_->size() > 0.0F ? markup.size_ / markup.font_->size() : 1.0F;
    const float baseline = std::round(pen->y);
    const float start = pen->x;
    float end = start;
    for (const char32_t charcode : line) {
        if (const boost::shared_ptr<TextureFont::Glyph> glyph = markup.font_->packed(charcode)) {
            end += glyph->advance_.x * scale;
        }
    }

    // y grows downwards, so a metric above the baseline is taken from it
    const float ascender = markup.font_->ascender() * scale;
    const float thickness = markup.font_->underlineThickness() * scale;
    if (end > start && markup.backgroundColor_.a > 0.0F) {
        addSolid(glm::vec2(start, baseline - ascender), glm::vec2(end, baseline - ascender + (markup.font_->height() * scale)), markup,
            markup.backgroundColor_);
    }
    for (const char32_t charcode : line) {
        addGlyph(*pen, baseline, scale, markup, charcode);
        if (const boost::shared_ptr<TextureFont::Glyph> glyph = markup.font_->packed(charcode)) {
            pen->x += glyph->advance_.x * scale;
        }
    }
    if (end <= start) {
        return;
    }
    // the underline position is below the baseline when it is negative, as the face gives it
    if (markup.underline_) {
        const float top = baseline - (markup.font_->underlinePosition() * scale);
        addSolid(glm::vec2(start, top), glm::vec2(end, top + thickness), markup, markup.underlineColor_);
    }
    if (markup.overline_) {
        addSolid(glm::vec2(start, baseline - ascender), glm::vec2(end, baseline - ascender + thickness), markup, markup.overlineColor_);
    }
    if (markup.strikethrough_) {
        const float top = baseline - (ascender * STRIKETHROUGH);
        addSolid(glm::vec2(start, top), glm::vec2(end, top + thickness), markup, markup.strikethroughColor_);
    }
}

void TextureTextBuffer::addGlyph(const glm::vec2& pen, float baseline, float scale, const Markup& markup, char32_t charcode) {
    const boost::shared_ptr<TextureFont::Glyph> glyph = markup.font_->packed(charcode);
    if (!glyph) {
        return;
    }
    // the quad's bottom is as far below the baseline as the bitmap reaches, its height less its
    // top bearing, and its top is one bitmap height above that
    const float glyphWidth = static_cast<float>(glyph->width_) * scale;
    const float glyphHeight = static_cast<float>(glyph->height_) * scale;
    const glm::vec2 xy0(pen.x + (static_cast<float>(glyph->offset_.x) * scale), baseline + glyphHeight - (static_cast<float>(glyph->offset_.y) * scale));
    const glm::vec2 xy1(xy0.x + glyphWidth, xy0.y - glyphHeight);
    addQuad(xy0, xy1, glyph->st_[0], glyph->st_[1], markup.foregroundColor_, markup.gamma_);
}

void TextureTextBuffer::addSolid(const glm::vec2& xy0, const glm::vec2& xy1, const Markup& markup, const glm::vec4& color) {
    // a full atlas may have no room for the white square, and then nothing is drawn
    const boost::shared_ptr<TextureFont::Glyph> solid = markup.font_->packed(TextureFont::SOLID);
    if (solid) {
        addQuad(xy0, xy1, solid->st_[0], solid->st_[1], color, markup.gamma_);
    }
}

void TextureTextBuffer::addQuad(const glm::vec2& xy0, const glm::vec2& xy1, const glm::vec2& uv0, const glm::vec2& uv1, const glm::vec4& color, float gamma) {
    const unsigned int vcount = static_cast<unsigned int>(vertices().size());

    // uv[0,1].t are flipped so y(0) can be top of screen (otherwise texture is upside down)
    addVertex(glm::vec3(xy0.x, xy0.y, 0.0F), glm::vec2(uv0.s, uv1.t), color, xy0.x - static_cast<float>(static_cast<int>(xy0.x)), gamma);
    addVertex(glm::vec3(xy0.x, xy1.y, 0.0F), glm::vec2(uv0.s, uv0.t), color, xy0.x - static_cast<float>(static_cast<int>(xy0.x)), gamma);
    addVertex(glm::vec3(xy1.x, xy1.y, 0.0F), glm::vec2(uv1.s, uv0.t), color, xy1.x - static_cast<float>(static_cast<int>(xy1.x)), gamma);
    addVertex(glm::vec3(xy1.x, xy0.y, 0.0F), glm::vec2(uv1.s, uv1.t), color, xy1.x - static_cast<float>(static_cast<int>(xy1.x)), gamma);

    // Use CCW winding so that the ortho matrix can put y(0) at the top of the screen
    // CW winding tri indices would be (0, 1, 2), (0, 2, 3)
    addIndex(vcount);
    addIndex(vcount + 2);
    addIndex(vcount + 1);
    addIndex(vcount);
    addIndex(vcount + 3);
    addIndex(vcount + 2);
}

void TextureTextBuffer::addVertex(const glm::vec3& position, const glm::vec2& texture, const glm::vec4& color, float shift, float gamma) {
    TextBuffer::addVertex(position);
    addTextureCoordinate(texture);
    addColor(color);
    shift_.push_back(shift);
    gamma_.push_back(gamma);
}

void TextureTextBuffer::addText(glm::vec2* pen, const Markup& markup, std::string_view utf8) {
    if (vertices().empty()) {
        origin_ = *pen;
    }
    const std::u32string text = decode(utf8);
    std::size_t from = 0;
    while (true) {
        const std::size_t newline = text.find(U'\n', from);
        addLine(pen, markup, std::u32string_view(text).substr(from, newline == std::u32string::npos ? std::u32string::npos : newline - from));
        if (newline == std::u32string::npos) {
            return;
        }
        // the same ratio as a glyph's metrics, so a markup with no size moves down one line
        // at the font's own size. The line height already holds the descender and the gap
        const float scale = markup.size_ > 0.0F && markup.font_->size() > 0.0F ? markup.size_ / markup.font_->size() : 1.0F;
        pen->x = origin_.x;
        pen->y += markup.font_->height() * scale;
        from = newline + 1;
    }
}

void TextureTextBuffer::clear() {
    // reset all of the underlying vertex buffer data sources
    TextBuffer::clear();
    shift_.clear();
    gamma_.clear();
}


std::vector<float>& TextureTextBuffer::shift() {
    return shift_;
}

std::vector<float>& TextureTextBuffer::gamma() {
    return gamma_;
}

};  // namespace v3d::font
