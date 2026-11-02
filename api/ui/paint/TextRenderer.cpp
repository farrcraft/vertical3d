/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextRenderer.h"

#include <api/asset/Manager.h>
#include <api/font/Face.h>
#include <api/font/FaceStyle.h>
#include <api/font/Family.h>
#include <api/font/FallbackChain.h>
#include <api/font/GlyphStore.h>
#include <api/font/Library.h>
#include <api/font/Paragraph.h>
#include <api/font/Rasterize.h>
#include <api/font/ShapedGlyph.h>
#include <api/font/ShapingCache.h>
#include <api/font/TextStyle.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>

#include "GlyphPages.h"
#include "Text.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <glm/vec2.hpp>

namespace v3d::ui::paint {

namespace {

/**
 * The weight semibold text is drawn at.
 **/
constexpr unsigned int SEMIBOLD_WEIGHT = 600;

};  // namespace

const char* const TextRenderer::defaultFont = "fonts/Inter-Regular.ttf";
const char* const TextRenderer::semiBoldFont = "fonts/Inter-SemiBold.ttf";
const char* const TextRenderer::fallbackFont = "fonts/NotoSans-Regular.ttf";

const float TextRenderer::baseSize = 48.0F;

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
    GlyphPages pages,
    float size,
    const Faces& faces) :
    shaping_(boost::make_shared<font::ShapingCache>("", logger)),
    pages_(std::move(pages)),
    size_(size) {
    // each face is opened here, not loaded as an asset. The manager only supplies the file's path
    const boost::shared_ptr<font::Library> library = boost::make_shared<font::Library>();
    const auto open = [&](const std::string& file) {
        return file.empty() ? boost::shared_ptr<font::Face>() : font::Face::open(library, assetManager->path(file), logger);
    };
    // the regular and semibold faces are one family, and the fallback the next in the chain
    const boost::shared_ptr<font::Family> primary = boost::make_shared<font::Family>("primary");
    primary->add(open(faces.regular));
    primary->add(open(faces.semiBold));
    const boost::shared_ptr<font::Family> fallback = boost::make_shared<font::Family>("fallback");
    fallback->add(open(faces.fallback));
    if (!faces.fallback.empty() && fallback->empty()) {
        logger->get()->warn("{} could not be loaded, so a character the faces lack draws as their missing glyph", faces.fallback);
    }
    if (primary->empty()) {
        logger->get()->error("{} could not be opened, so nothing drawn through it will have text", faces.regular);
        return;
    }
    chain_ = boost::make_shared<font::FallbackChain>();
    chain_->add(primary);
    chain_->add(fallback);
}

/**
 **/
bool TextRenderer::loaded() const noexcept {
    return static_cast<bool>(chain_);
}

/**
 **/
float TextRenderer::size() const noexcept {
    return size_;
}

/**
 **/
void TextRenderer::beginFrame() {
    store_.beginFrame();
}

/**
 **/
font::TextStyle TextRenderer::style(float size, Weight weight) const {
    font::TextStyle style;
    style.fonts = chain_;
    style.face.weight = weight == Weight::SemiBold ? SEMIBOLD_WEIGHT : font::FaceStyle::REGULAR;
    style.size = size > 0.0F ? size : size_;
    return style;
}

/**
 **/
float TextRenderer::width(std::string_view text, float size, Weight weight) const {
    if (!loaded() || text.empty()) {
        return 0.0F;
    }
    // the runs reach past the space at a line's end, which a caret placed after it needs
    const font::Paragraph paragraph(shaping_, std::string(text), style(size, weight));
    float widest = 0.0F;
    for (const font::Line& line : paragraph.lines()) {
        float reach = 0.0F;
        for (const font::LineRun& run : line.runs) {
            reach += run.advance;
        }
        widest = std::max(widest, reach);
    }
    return widest;
}

/**
 **/
void TextRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, std::string_view text, const glm::vec2& pen, const glm::vec4& colour,
    float size, Weight weight) {
    if (text.empty() || canvas == nullptr || !loaded()) {
        return;
    }
    const font::Paragraph paragraph(shaping_, std::string(text), style(size, weight));
    const std::vector<font::Line>& lines = paragraph.lines();
    placed_.clear();
    for (const font::Line& line : lines) {
        const float baseline = pen.y + line.baseline - lines.front().baseline;
        for (const font::LineRun& run : line.runs) {
            float x = pen.x + run.x;
            for (const font::ShapedGlyph& glyph : run.glyphs) {
                // the pen is kept to a step of a pixel, and the glyph rasterized at that step
                const float at = x + glyph.offset.x;
                x += glyph.advance;
                if (!std::isfinite(at)) {
                    continue;
                }
                const float whole = std::floor(at);
                const float steps = std::min<float>(std::floor((at - whole) * font::SUBPIXEL_STEPS), font::SUBPIXEL_STEPS - 1);
                const auto step = static_cast<std::uint8_t>(steps);  // checked: at is finite, so the step is from 0 to 3
                const font::GlyphKey key{ .face = run.face.get(), .index = glyph.index, .size = run.size, .subpixel = step };
                const std::optional<font::StoredGlyph> stored = store_.glyph(key, font::rasterize);
                if (stored.has_value() && stored->size.x > 0 && stored->size.y > 0) {
                    const glm::vec2 corner(whole + static_cast<float>(stored->bearing.x), std::round(baseline + glyph.offset.y) + static_cast<float>(stored->bearing.y));
                    placed_.push_back(Placed{ .glyph = *stored, .position = corner });
                }
            }
        }
    }

    // a page a glyph was just put on has to be on the device before a quad names it
    pages_.sync(&store_);
    const auto side = static_cast<float>(store_.spec().pageSize);
    for (const Placed& placed : placed_) {
        const glm::vec2 extent(placed.glyph.size);
        const glm::vec2 from = glm::vec2(placed.glyph.position) / side;
        canvas->rect(placed.position, placed.position + extent, from, from + (extent / side), colour, pages_.texture(placed.glyph.page));
    }
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
