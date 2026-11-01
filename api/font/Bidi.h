/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::font::internal {
class BidiHandle;
};  // namespace v3d::font::internal

namespace v3d::font {

/**
 * The direction a paragraph is read in, before the embedding levels inside it are resolved.
 **/
enum class BaseDirection : std::uint8_t {
    Auto,         /**< the direction of the paragraph's first strong character, or left to right with none **/
    LeftToRight,
    RightToLeft
};

/**
 * A stretch of text at one embedding level. An even level reads left to right and an odd one
 * right to left.
 **/
struct LevelRun final {
    std::size_t start = 0;   /**< the byte offset where it starts **/
    std::size_t end = 0;     /**< and one past where it ends **/
    std::uint8_t level = 0;

    bool rightToLeft() const noexcept {
        return (level & 1U) != 0;
    }
};

/**
 * Which character a caret goes with, where the two either side of it are drawn apart.
 **/
enum class Affinity : std::uint8_t {
    Upstream,   /**< the character before it, as after typing or moving forwards onto it **/
    Downstream  /**< the character after it **/
};

/**
 * Where a caret is drawn: against one edge of one character's glyphs.
 **/
struct CaretEdge final {
    std::size_t character = 0;  /**< the byte offset where the character starts **/
    bool right = false;         /**< whether the caret is at its right edge, rather than its left **/
};

/**
 * The bidirectional structure of a UTF-8 text, as UAX #9 resolves it.
 *
 * The text is divided into paragraphs at each paragraph separator, such as a newline, and each
 * paragraph takes its base direction from the one given. Inside a paragraph every character
 * has an embedding level, so a number in an Arabic sentence reads left to right inside text
 * read right to left. Explicit embeddings, overrides and isolates are applied.
 *
 * Every offset is a byte offset in the text. An offset inside a character means that character.
 * A Bidi reads the text only while it is made, and does not hold it.
 **/
class Bidi final {
 public:
    Bidi(std::string_view utf8, BaseDirection base);

    /**
     * @return the text's maximal runs of one level, in the text's order, covering it with no gap.
     *         An empty text gives none
     **/
    const std::vector<LevelRun>& runs() const noexcept;

    /**
     * @return the level of the character at an offset, or the last paragraph's level at or past
     *         the end
     **/
    std::uint8_t level(std::size_t offset) const;

    /**
     * @return whether the paragraph holding an offset reads right to left
     **/
    bool rightToLeft(std::size_t offset) const;

    /**
     * One line's runs, reordered from the text's order into the order they are drawn, left to
     * right.
     *
     * Spaces at the line's end, and before a tab, take the paragraph's level, so trailing space
     * stays at the paragraph's end edge.
     *
     * @return the runs from start to end in visual order, or none when the range is empty, out
     *         of the text, or crosses from one paragraph into another
     **/
    std::vector<LevelRun> line(std::size_t start, std::size_t end) const;

    /**
     * @return the edge a caret at an offset is drawn against. Upstream, it is the end edge of the
     *         character before, and downstream the start edge of the character after; where
     *         there is no such character the other is taken. An empty text gives the paragraph's
     *         start edge
     **/
    CaretEdge caret(std::size_t offset, Affinity affinity) const;

 private:
    boost::shared_ptr<internal::BidiHandle> handle_;
    std::vector<LevelRun> runs_;
    std::size_t size_ = 0;
    std::uint8_t base_ = 0;  // the level every character takes when ICU could not resolve them
};

};  // namespace v3d::font
