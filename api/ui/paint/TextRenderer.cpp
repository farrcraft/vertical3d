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
#include <api/font/ColourGlyph.h>
#include <api/font/Paint.h>
#include <api/font/Rasterize.h>
#include <api/font/ShapedGlyph.h>
#include <api/font/ShapingCache.h>
#include <api/font/TextStyle.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/paint/internal/EffectQuad.h>

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
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint {

namespace {

/**
 * The weight semibold text is drawn at.
 **/
constexpr unsigned int SEMIBOLD_WEIGHT = 600;

/**
 * @return the pages colour glyphs are kept in: the default pages, of four bytes a pixel
 **/
font::GlyphStore::Spec colourPages() {
    font::GlyphStore::Spec spec;
    spec.channels = 4;
    return spec;
}

};  // namespace

const char* const TextRenderer::defaultFont = "fonts/Inter-Regular.ttf";
const char* const TextRenderer::semiBoldFont = "fonts/Inter-SemiBold.ttf";
const char* const TextRenderer::fallbackFont = "fonts/NotoSans-Regular.ttf";
const char* const TextRenderer::emojiFont = "fonts/NotoColorEmoji.ttf";

const float TextRenderer::baseSize = 48.0F;

/**
 **/
TextRenderer::Faces::Faces() :
regular(defaultFont),
semiBold(semiBoldFont),
fallback(fallbackFont),
emoji(emojiFont) {
}

/**
 **/
TextRenderer::TextRenderer(const boost::shared_ptr<v3d::asset::Manager>& assetManager,
    const boost::shared_ptr<v3d::log::Logger>& logger,
    GlyphPages pages,
    float size,
    const Faces& faces) :
    shaping_(boost::make_shared<font::ShapingCache>("", logger)),
    colourStore_(colourPages()),
    pages_(pages),
    colourPages_(std::move(pages)),
    size_(size) {
    // each face is opened here, not loaded as an asset. The manager only supplies the file's path
    const boost::shared_ptr<font::Library> library = boost::make_shared<font::Library>();
    const auto open = [&](const std::string& file) {
        return file.empty() ? boost::shared_ptr<font::Face>() : font::Face::open(library, assetManager->path(file), logger);
    };
    // the regular and semibold faces are one family, and the fallback and the emoji the next in
    // the chain
    const boost::shared_ptr<font::Family> primary = boost::make_shared<font::Family>("primary");
    primary->add(open(faces.regular));
    primary->add(open(faces.semiBold));
    const boost::shared_ptr<font::Family> fallback = boost::make_shared<font::Family>("fallback");
    fallback->add(open(faces.fallback));
    if (!faces.fallback.empty() && fallback->empty()) {
        logger->get()->warn("{} could not be loaded, so a character the faces lack draws as their missing glyph", faces.fallback);
    }
    const boost::shared_ptr<font::Family> emoji = boost::make_shared<font::Family>("emoji");
    emoji->add(open(faces.emoji));
    if (!faces.emoji.empty() && emoji->empty()) {
        logger->get()->warn("{} could not be loaded, so an emoji the faces lack draws as their missing glyph", faces.emoji);
    }
    if (primary->empty()) {
        logger->get()->error("{} could not be opened, so nothing drawn through it will have text", faces.regular);
        return;
    }
    chain_ = boost::make_shared<font::FallbackChain>();
    chain_->add(primary);
    chain_->add(fallback);
    chain_->add(emoji);
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
    colourStore_.beginFrame();
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
const boost::shared_ptr<font::ShapingCache>& TextRenderer::shaping() const noexcept {
    return shaping_;
}

/**
 **/
void TextRenderer::trace(Trace trace) {
    trace_ = std::move(trace);
}

/**
 **/
float TextRenderer::width(std::string_view text, float size, Weight weight) const {
    return width(text, style(size, weight));
}

/**
 **/
float TextRenderer::width(std::string_view text, const font::TextStyle& styled) const {
    if (!loaded() || text.empty()) {
        return 0.0F;
    }
    // the runs reach past the space at a line's end, which a caret placed after it needs
    const font::Paragraph paragraph(shaping_, std::string(text), styled);
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
std::vector<std::string> TextRenderer::rows(std::string_view text, float width, const font::TextStyle& styled) const {
    std::vector<std::string> found;
    if (!loaded() || text.empty()) {
        return found;
    }
    font::ParagraphStyle layout;
    layout.width = width;
    const font::Paragraph paragraph(shaping_, std::string(text), styled, layout);
    for (const font::Line& line : paragraph.lines()) {
        std::string_view row = std::string_view(paragraph.text()).substr(line.start, line.end - line.start);
        while (!row.empty() && (row.back() == ' ' || row.back() == '\n' || row.back() == '\r')) {
            row.remove_suffix(1);
        }
        if (!row.empty()) {
            found.emplace_back(row);
        }
    }
    return found;
}

/**
 **/
void TextRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, std::string_view text, const glm::vec2& pen, const glm::vec4& colour,
    float size, Weight weight) {
    if (text.empty() || canvas == nullptr || !loaded()) {
        return;
    }
    font::TextStyle drawn = style(size, weight);
    drawn.colour = colour;
    draw(canvas, text, pen, drawn);
}

/**
 **/
void TextRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, std::string_view text, const glm::vec2& pen, const font::TextStyle& styled) {
    if (text.empty() || canvas == nullptr || !loaded()) {
        return;
    }
    draw(canvas, font::Paragraph(shaping_, std::string(text), styled), pen);
}

/**
 **/
void TextRenderer::draw(v3d::render::realtime::canvas::Canvas* canvas, const font::Paragraph& paragraph, const glm::vec2& pen) {
    if (canvas == nullptr) {
        return;
    }
    const std::vector<font::Line>& lines = paragraph.lines();
    placed_.clear();
    effects_.clear();
    for (const font::Line& line : lines) {
        const float baseline = pen.y + line.baseline - lines.front().baseline;
        report(paragraph, line, glm::vec2(pen.x + line.x, baseline));
        for (const font::LineRun& run : line.runs) {
            float x = pen.x + run.x;
            for (const font::ShapedGlyph& glyph : run.glyphs) {
                // the pen is kept to a step of a pixel, and the glyph rasterized at that step
                const float at = x + glyph.offset.x;
                x += glyph.advance;
                if (!std::isfinite(at) || !run.face) {
                    continue;
                }
                const font::TextStyle& styled = paragraph.styleAt(run.ellipsis ? run.start : glyph.cluster);
                place(run, glyph.index, styled, glm::vec2(at, std::round(baseline + glyph.offset.y)));
            }
        }
    }

    // a page a glyph was just put on has to be on the device before a quad names it
    pages_.sync(&store_);
    colourPages_.sync(&colourStore_);

    // every glyph's effects beneath every glyph, each layer beneath the next
    internal::layerEffects(&effects_);
    internal::drawEffects(canvas, effects_, pages_, glm::mat3(1.0F), &vertices_, &indices_);

    drawPlaced(canvas);
}

/**
 **/
void TextRenderer::drawPlaced(v3d::render::realtime::canvas::Canvas* canvas) {
    // each glyph a quad sampling its page, one call for each run on one page
    for (std::size_t index = 0; index < placed_.size(); index++) {
        const Placed& placed = placed_[index];
        const auto side = static_cast<float>((placed.coloured ? colourStore_ : store_).spec().pageSize);
        const glm::vec2 low = placed.position;
        const glm::vec2 high = low + glm::vec2(placed.glyph.size);
        const glm::vec2 from = glm::vec2(placed.glyph.position) / side;
        const glm::vec2 to = from + (glm::vec2(placed.glyph.size) / side);
        const auto first = static_cast<std::uint32_t>(vertices_.size());
        vertices_.push_back({ .position = low, .uv = from, .colour = placed.colour, .contrast = placed.contrast });
        vertices_.push_back({ .position = glm::vec2(low.x, high.y), .uv = glm::vec2(from.x, to.y), .colour = placed.colour, .contrast = placed.contrast });
        vertices_.push_back({ .position = high, .uv = to, .colour = placed.colour, .contrast = placed.contrast });
        vertices_.push_back({ .position = glm::vec2(high.x, low.y), .uv = glm::vec2(to.x, from.y), .colour = placed.colour, .contrast = placed.contrast });
        for (const std::uint32_t corner : { 0U, 1U, 2U, 2U, 3U, 0U }) {
            indices_.push_back(first + corner);
        }
        if (index + 1 == placed_.size() || placed_[index + 1].glyph.page != placed.glyph.page || placed_[index + 1].coloured != placed.coloured) {
            canvas->glyphs(vertices_, indices_, (placed.coloured ? colourPages_ : pages_).texture(placed.glyph.page));
            vertices_.clear();
            indices_.clear();
        }
    }
}

/**
 **/
void TextRenderer::place(const font::LineRun& run, unsigned int index, const font::TextStyle& styled, const glm::vec2& pen) {
    // an effect is drawn from the glyph's exact pen, since its field serves every step
    internal::placeEffects(&effects_, &store_, *run.face, index, run.size, styled.effects, pen);
    const float whole = std::floor(pen.x);
    const float steps = std::min<float>(std::floor((pen.x - whole) * font::SUBPIXEL_STEPS), font::SUBPIXEL_STEPS - 1);
    const auto step = static_cast<std::uint8_t>(steps);  // checked: the pen is finite, so the step is from 0 to 3
    const Drawn how = drawn(*run.face, index, run.size);
    if (how == Drawn::Coverage) {
        const font::GlyphKey key{ .face = run.face.get(), .index = index, .size = run.size, .subpixel = step };
        const std::optional<font::StoredGlyph> stored = store_.glyph(key, font::rasterize);
        if (stored.has_value() && stored->size.x > 0 && stored->size.y > 0) {
            const glm::vec2 corner(whole + static_cast<float>(stored->bearing.x), pen.y + static_cast<float>(stored->bearing.y));
            placed_.push_back(Placed{ .glyph = *stored, .position = corner, .colour = styled.colour, .contrast = styled.contrast });
        }
        return;
    }
    // a glyph with no layer in the text's colour is the same in every colour, so it is kept once
    const glm::vec4 opaque(glm::vec3(styled.colour), 1.0F);
    const font::GlyphKey key{ .face = run.face.get(), .index = index, .size = run.size, .subpixel = step, .mode = font::GlyphMode::Colour,
        .foreground = how == Drawn::Foreground ? font::packColour(opaque) : 0 };
    const std::optional<font::StoredGlyph> stored = colourStore_.glyph(key, [](const font::GlyphKey& asked) { return font::paint(asked); });
    if (stored.has_value() && stored->size.x > 0 && stored->size.y > 0) {
        const glm::vec2 corner(whole + static_cast<float>(stored->bearing.x), pen.y + static_cast<float>(stored->bearing.y));
        // a picture quad, not text: its texel is its colour, and the text's alpha fades it
        placed_.push_back(Placed{ .glyph = *stored, .position = corner, .colour = glm::vec4(1.0F, 1.0F, 1.0F, styled.colour.a), .contrast = -1.0F,
            .coloured = true });
    }
}

/**
 **/
TextRenderer::Drawn TextRenderer::drawn(const font::Face& face, unsigned int index, float size) {
    const std::pair<const font::Face*, unsigned int> key(&face, index);
    const auto found = drawn_.find(key);
    if (found != drawn_.end()) {
        return found->second;
    }
    Drawn how = Drawn::Coverage;
    if (face.coloured(index)) {
        how = font::colour(face, index, 0, size).foreground() ? Drawn::Foreground : Drawn::Colour;
    }
    drawn_.emplace(key, how);
    return how;
}

/**
 **/
void TextRenderer::report(const font::Paragraph& paragraph, const font::Line& line, const glm::vec2& pen) const {
    if (!trace_) {
        return;
    }
    std::string_view shown = std::string_view(paragraph.text()).substr(line.start, line.end - line.start);
    while (!shown.empty() && (shown.back() == ' ' || shown.back() == '\n' || shown.back() == '\r')) {
        shown.remove_suffix(1);
    }
    const font::TextStyle& styled = paragraph.styleAt(line.start);
    trace_(Traced{ .text = std::string(shown), .pen = pen, .colour = styled.colour, .weight = weight(static_cast<float>(styled.face.weight)),
        .size = styled.size });
}

};  // namespace v3d::ui::paint
