/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/font/FallbackChain.h>
#include <api/font/GlyphStore.h>
#include <api/font/Paragraph.h>
#include <api/font/ShapingCache.h>
#include <api/font/TextStyle.h>
#include <api/log/Logger.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/ui/paint/internal/EffectQuad.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "GlyphPages.h"
#include "Text.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::paint {

/**
 * One typeface in two weights, and the text drawn with it.
 *
 * The regular and the semibold face are one family, and the fallback face is the next in a
 * font::FallbackChain, so a string mixing the faces lays out as one font. Text is shaped and
 * laid out as a font::Paragraph, through a cache that keeps what it shaped.
 *
 * Every glyph is rasterized the first time it is drawn, at the size it is drawn at, into a
 * font::GlyphStore whose pages reach the device through GlyphPages. Its pen is kept to a
 * quarter of a pixel, and each line's baseline is on a whole pixel, so text on the pixel grid is
 * drawn at its exact size and sharp. A glyph is a batched quad sampling its page, so a line of
 * text joins whatever canvas the app is already filling, and costs the frame no pass of its own.
 *
 * A style's effects are drawn beneath its text from each glyph's distance field, kept in the
 * same store, and every glyph's effects before any glyph.
 *
 * ComponentRenderer and Immediate measure, wrap and draw through one, which the screen owns. It
 * needs no device, so a ui is drawn and tested without one.
 **/
class TextRenderer {
 public:
    /**
     * The regular face in the shared data directory, used when the caller names no other.
     **/
    static const char* const defaultFont;

    /**
     * The semibold face in the shared data directory, used when the caller names no other.
     **/
    static const char* const semiBoldFont;

    /**
     * The face in the shared data directory that the code points the other two lack are
     * drawn from, used when the caller names no other.
     **/
    static const char* const fallbackFont;

    /**
     * The files a renderer loads its faces from, each resolved against the asset manager's
     * path. An empty semibold draws semibold text in the regular face, and an empty fallback
     * draws a code point the faces lack as their missing glyph.
     **/
    struct Faces final {
        /**
         * The faces in the shared data directory.
         **/
        Faces();

        std::string regular;
        std::string semiBold;
        std::string fallback;
    };

    /**
     * The size text is drawn at when a draw names none, in pixels.
     **/
    static const float baseSize;

    /**
     * A line of text as it was drawn: its text without the space at its end, where its baseline
     * starts, its colour, its weight and its size.
     **/
    struct Traced final {
        std::string text;
        glm::vec2 pen{ 0.0F };
        glm::vec4 colour{ 1.0F };
        Weight weight = Weight::Regular;
        float size = 0.0F;
    };

    /**
     * Told of each line drawn, so a test or a tool can read back what a ui drew without a
     * device.
     **/
    using Trace = std::function<void(const Traced&)>;

    /**
     * Load the faces.
     *
     * A regular face that cannot be loaded leaves this measuring zero and drawing nothing, and
     * the log says so. The app then loses its labels but still draws its frame. A semibold face
     * that cannot be loaded draws semibold text in the regular face, and a fallback that cannot
     * be loaded leaves the missing glyphs in place, and the log says so.
     *
     * @param pages what puts the glyph pages on the device
     * @param size the size text is drawn at when a draw names none
     **/
    TextRenderer(const boost::shared_ptr<v3d::asset::Manager>& assetManager,
        const boost::shared_ptr<v3d::log::Logger>& logger,
        GlyphPages pages,
        float size = baseSize,
        const Faces& faces = Faces());

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    /**
     * Whether the regular face was loaded. Nothing is measured or drawn when it was not.
     **/
    bool loaded() const noexcept;

    /**
     * @return the size text is drawn at when a draw names none
     **/
    float size() const noexcept;

    /**
     * Start a frame. A glyph drawn in the frame before may now make room for another.
     **/
    void beginFrame();

    /**
     * How wide a string is when it is drawn at a size, in pixels, including any space at its end.
     *
     * @param size the size it will be drawn at, defaulting to size()
     * @param weight the face it will be drawn in
     **/
    float width(std::string_view text, float size = 0.0F, Weight weight = Weight::Regular) const;

    /**
     * How wide a string is in a style, in pixels, including any space at its end.
     *
     * @param styled the style it is set in, from style(), with any features it asks for
     **/
    float width(std::string_view text, const font::TextStyle& styled) const;

    /**
     * Break a string into the rows it takes at a width, as a paragraph lays it out: at Unicode's
     * line break opportunities, and inside a word wider than the width. Each row is without the
     * space at its end.
     *
     * @param width the room a row has. At 0 or less the string takes one row for each of its
     *        lines
     * @param styled the style it is set in, from style()
     * @return the rows, and none for a string with no text
     **/
    std::vector<std::string> rows(std::string_view text, float width, const font::TextStyle& styled) const;

    /**
     * Lay a string out at the pen and append its glyphs to a canvas.
     *
     * The string is UTF-8. Each glyph is drawn at its pen rounded down to a quarter of a pixel,
     * and each line's baseline at its pen rounded to a whole pixel. A newline starts a line below.
     *
     * @param pen where the baseline of the first line starts
     * @param size the size to draw at, defaulting to size()
     * @param weight the face to draw in
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, std::string_view text, const glm::vec2& pen, const glm::vec4& colour,
        float size = 0.0F, Weight weight = Weight::Regular);

    /**
     * Lay a string out in a style at the pen and append its glyphs to a canvas, as the draw
     * above does.
     *
     * @param styled the style it is set in, from style(), with its colour and any features
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, std::string_view text, const glm::vec2& pen, const font::TextStyle& styled);

    /**
     * Append a paragraph's glyphs to a canvas, each in the colour of its style, with its
     * style's effects beneath it. Glyphs are placed as the string draw places them.
     *
     * @param pen where the baseline of the first line starts
     **/
    void draw(v3d::render::realtime::canvas::Canvas* canvas, const font::Paragraph& paragraph, const glm::vec2& pen);

    /**
     * @return the style text of a size and a weight is set in, defaulting to size(), for a
     *         paragraph drawn through this
     **/
    font::TextStyle style(float size = 0.0F, Weight weight = Weight::Regular) const;

    /**
     * @return the cache this shapes text through, which a paragraph drawn through this can share
     **/
    const boost::shared_ptr<font::ShapingCache>& shaping() const noexcept;

    /**
     * Tell a trace of every line drawn from now on, or of none when it is empty.
     **/
    void trace(Trace trace);

 private:
    /**
     * A glyph placed for a draw, before its page is on the device.
     **/
    struct Placed {
        font::StoredGlyph glyph;
        glm::vec2 position{ 0.0F };
        glm::vec4 colour{ 1.0F };
        float contrast = 0.0F;
    };

    /**
     * Place a glyph and its style's effects for a draw, the glyph rasterized at its pen's step of
     * a pixel and each effect at its exact pen.
     *
     * @param pen the glyph's pen, its baseline already on a whole pixel
     **/
    void place(const font::LineRun& run, unsigned int index, const font::TextStyle& styled, const glm::vec2& pen);

    /**
     * Tell the trace of a line drawn with its baseline starting at a pen.
     **/
    void report(const font::Paragraph& paragraph, const font::Line& line, const glm::vec2& pen) const;

    boost::shared_ptr<font::FallbackChain> chain_;
    boost::shared_ptr<font::ShapingCache> shaping_;
    font::GlyphStore store_;
    GlyphPages pages_;
    float size_;
    Trace trace_;
    // what draw() places, kept between draws so a frame of labels does not allocate
    std::vector<Placed> placed_;
    std::vector<internal::PlacedEffect> effects_;
    std::vector<render::realtime::canvas::Canvas::Vertex> vertices_;
    std::vector<std::uint32_t> indices_;
};

};  // namespace v3d::ui::paint
