/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "Bidi.h"
#include "ShapedGlyph.h"
#include "TextStyle.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::font {

class Face;
class ShapingCache;
struct ShapedText;

/**
 * A stretch of one line drawn in one face, at one size and one level, in the order it is drawn.
 **/
struct LineRun final {
    boost::shared_ptr<Face> face;
    float size = 0.0F;
    bool rightToLeft = false;
    bool ellipsis = false;           /**< whether it is the ellipsis that ends a line cut short **/
    std::size_t start = 0;           /**< the byte offset where its text starts. An ellipsis is at the cut **/
    std::size_t end = 0;
    float x = 0.0F;                  /**< its left edge, from the paragraph's left **/
    float advance = 0.0F;
    std::vector<ShapedGlyph> glyphs; /**< left to right, each drawn at the sum of the advances before it **/
};

/**
 * One line of a paragraph. y grows downwards from the paragraph's top.
 **/
struct Line final {
    std::size_t start = 0;     /**< the byte offset where it starts **/
    std::size_t end = 0;       /**< and one past where it ends, after any space and line separator **/
    float top = 0.0F;
    float height = 0.0F;
    float baseline = 0.0F;
    float x = 0.0F;            /**< the left edge of its text, without the space at its end **/
    float width = 0.0F;        /**< the width of its text, without the space at its end **/
    std::vector<LineRun> runs; /**< left to right **/
};

/**
 * Where a caret is drawn: a vertical line from top, height pixels down.
 **/
struct Caret final {
    float x = 0.0F;
    float top = 0.0F;
    float height = 0.0F;
};

/**
 * The place in the text a point is over: a caret position and the character it goes with.
 **/
struct Hit final {
    std::size_t index = 0;
    Affinity affinity = Affinity::Downstream;
};

/**
 * One grapheme cluster on a line, with its left and right edges from the paragraph's left.
 **/
struct ClusterEdges final {
    std::size_t start = 0;
    std::size_t end = 0;
    float left = 0.0F;
    float right = 0.0F;
    bool rightToLeft = false;
};

/**
 * A rectangle a selection covers, from the paragraph's top left.
 **/
struct SelectionRect final {
    glm::vec2 position{ 0.0F };
    glm::vec2 size{ 0.0F };
};

/**
 * A text with styles over its ranges, divided into lines and placed for drawing, which the ui
 * and the games hold.
 *
 * It is shaped once through a cache shared by every paragraph, and laid out once for its width.
 * A change of width, alignment or line limit lays it out again without shaping it. A change of
 * colour or decoration does neither, and a change of text or of how a range shapes looks the
 * text up in the cache again. Its queries lay it out first when it has to be.
 *
 * Lines break where Unicode allows, and a word wider than a line breaks between its grapheme
 * clusters. Each line's runs are in the order they are drawn. Text is not shaped again where a
 * line breaks it, so a ligature or a joining form across the break keeps its shape.
 *
 * Every index is a byte offset in the text, and every caret position is at a grapheme cluster's
 * edge. A paragraph is used from one thread, with its cache.
 **/
class Paragraph final {
 public:
    Paragraph(const boost::shared_ptr<ShapingCache>& cache, const std::string& text, const TextStyle& style, const ParagraphStyle& paragraph = {});

    /**
     * Replace the text, which takes the style of the first range.
     **/
    void setText(const std::string& text);

    /**
     * Set the style of the bytes from one offset to another, clamped to the text.
     **/
    void setStyle(std::size_t start, std::size_t end, const TextStyle& style);

    void setParagraphStyle(const ParagraphStyle& paragraph);

    void setWidth(float width);

    const std::string& text() const noexcept;

    /**
     * @return the styles over the text, in order, covering it with no gap
     **/
    const std::vector<StyledRange>& styles() const noexcept;

    /**
     * @return the style of the character at an offset, or the last one's past the end
     **/
    const TextStyle& styleAt(std::size_t offset) const;

    const ParagraphStyle& paragraphStyle() const noexcept;

    /**
     * @return the width of its widest line and the height of all its lines
     **/
    glm::vec2 size() const;

    /**
     * @return its lines, top to bottom. Empty text, and text that ends in a line separator, has
     *         an empty line at its end
     **/
    const std::vector<Line>& lines() const;

    /**
     * @return the offsets a caret may stop at, in order, as Segmenter::graphemes() gives them
     **/
    const std::vector<std::size_t>& clusters() const;

    /**
     * Where a caret at an index is drawn. At the edge of a run read in the other direction, the
     * caret is drawn against the character its affinity names. At a wrap, upstream is the end
     * of the line above and downstream the start of the line below.
     *
     * An index cut away by the line limit is drawn at the end of the last line shown.
     **/
    Caret caret(std::size_t index, Affinity affinity) const;

    /**
     * @return the caret position nearest a point, on the line it is over or the nearest line.
     *         Over a cluster's left half it is that cluster's left edge, and over its right half
     *         its right edge
     **/
    Hit hitTest(const glm::vec2& point) const;

    /**
     * @return the rectangles covering the clusters from one index to another, one for each
     *         stretch of a line they cover side by side, top to bottom
     **/
    std::vector<SelectionRect> selection(std::size_t start, std::size_t end) const;

 private:
    void shape() const;
    void layout() const;
    std::size_t lineOf(std::size_t index, Affinity affinity) const;

    boost::shared_ptr<ShapingCache> cache_;
    std::string text_;
    std::vector<StyledRange> styles_;
    ParagraphStyle paragraph_;

    // filled when first asked for, and emptied by a change that undoes them
    mutable boost::shared_ptr<const ShapedText> shaped_;
    mutable bool laidOut_ = false;
    mutable std::vector<Line> lines_;
    mutable std::vector<std::vector<ClusterEdges>> stops_;  // each line's clusters, in the text's order
    mutable std::vector<std::size_t> visibleEnds_;  // where each line's text ends, before its separator
    mutable glm::vec2 size_{ 0.0F };
};

};  // namespace v3d::font
