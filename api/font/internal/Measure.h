/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <vector>

namespace v3d::font {
struct ShapedText;
};  // namespace v3d::font

namespace v3d::font::internal {

/**
 * The measures of one shaped text that its lines are broken and placed by.
 *
 * A glyph's advance belongs to the character it came from, so a ligature's whole advance is its
 * first character's. The text has to outlive the measure.
 **/
class Measure {
 public:
    explicit Measure(const ShapedText& shaped);

    /**
     * @return the advance of the bytes from one offset to another
     **/
    float width(std::size_t start, std::size_t end) const;

    /**
     * @return where a stretch's text ends, before the line separator ending it
     **/
    std::size_t visibleEnd(std::size_t start, std::size_t end) const;

    /**
     * @return where a stretch's text ends, before the white space ending it, which hangs past the
     *         end of a line
     **/
    std::size_t contentEnd(std::size_t start, std::size_t end) const;

    /**
     * @return the width of a stretch's text, without the white space ending it
     **/
    float contentWidth(std::size_t start, std::size_t end) const;

    /**
     * @return the furthest cluster boundary after start, up to end, that keeps the stretch from
     *         start within a width, or the first boundary after start when none does
     **/
    std::size_t fit(std::size_t start, std::size_t end, float limit) const;

    /**
     * @return whether a character ends a line wherever it is: a line feed, a carriage return, a
     *         vertical tab, a form feed, a next line, or a line or paragraph separator
     **/
    static bool separator(char32_t charcode);

    /**
     * @return whether a character is white space, a line separator among it. A no-break space,
     *         a figure space and a narrow no-break space are not, as they hold words together
     **/
    static bool white(char32_t charcode);

 private:
    /**
     * The line separators outside ASCII: next line, line separator and paragraph separator.
     **/
    static constexpr char32_t NEXT_LINE = 0x85;
    static constexpr char32_t LINE_SEPARATOR = 0x2028;
    static constexpr char32_t PARAGRAPH_SEPARATOR = 0x2029;

    /**
     * The spaces that hold the words either side of them on one line.
     **/
    static constexpr char32_t NO_BREAK_SPACE = 0xA0;
    static constexpr char32_t FIGURE_SPACE = 0x2007;
    static constexpr char32_t NARROW_NO_BREAK_SPACE = 0x202F;

    /**
     * @return where a stretch ends without the clusters at its end whose first character passes
     *         a test
     **/
    std::size_t trim(std::size_t start, std::size_t end, bool (*test)(char32_t)) const;

    const ShapedText& shaped_;
    std::vector<float> prefix_;  // the advance of each byte and every byte before it, from 0
};

};  // namespace v3d::font::internal
