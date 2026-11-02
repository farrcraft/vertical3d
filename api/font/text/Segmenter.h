/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::font::text::internal {
class BreakHandle;
};  // namespace v3d::font::text::internal

namespace v3d::font::text {

/**
 * A stretch of text between two word boundaries.
 **/
struct WordSegment final {
    std::size_t start = 0;  /**< the byte offset where it starts **/
    std::size_t end = 0;    /**< and one past where it ends **/
    bool word = false;      /**< whether it holds letters, digits or ideographs, rather than spaces or punctuation **/
};

/**
 * A place a line may end.
 **/
struct LineBreak final {
    std::size_t offset = 0;  /**< the byte offset where the next line would start **/
    bool mandatory = false;  /**< whether the line has to end here, after a newline or a paragraph separator **/
};

/**
 * Divides UTF-8 text where Unicode's text segmentation divides it: into grapheme clusters and
 * words as UAX #29 defines them, and at the line break opportunities UAX #14 defines.
 *
 * A grapheme cluster is what a reader sees as one character, such as a letter with its accents,
 * a Hangul syllable, or an emoji sequence. A caret stops only between clusters.
 *
 * Words and lines follow a locale's rules, and break Thai, Lao, Khmer and Myanmar text, which
 * marks no word boundaries, where a dictionary finds its words. Every offset is a byte offset
 * in the string, at the start of a character. A segmenter is used from one thread.
 **/
class Segmenter final {
 public:
    /**
     * @param locale a BCP 47 language tag, such as "ja" or "en-US". An empty tag uses Unicode's
     *        rules untailored
     **/
    Segmenter(const std::string& locale, const boost::shared_ptr<v3d::log::Logger>& logger);
    Segmenter(const Segmenter&) = delete;
    Segmenter& operator=(const Segmenter&) = delete;

    /**
     * @return whether the rules were loaded. When they were not, which is logged, every code
     *         point is its own cluster, the whole string is one word segment, and a line ends
     *         only at its end
     **/
    bool valid() const;

    /**
     * @return the offsets a caret may stop at: 0, the start of every cluster, and the string's
     *         size, in order. An empty string gives only 0
     **/
    std::vector<std::size_t> graphemes(std::string_view utf8) const;

    /**
     * @return the string divided at its word boundaries, in order, with no gap between one
     *         segment and the next. An empty string gives none
     **/
    std::vector<WordSegment> words(std::string_view utf8) const;

    /**
     * @return the places a line may end, in order. The last is the string's size. An empty
     *         string gives none
     **/
    std::vector<LineBreak> lineBreaks(std::string_view utf8) const;

 private:
    boost::shared_ptr<internal::BreakHandle> handle_;
};

};  // namespace v3d::font::text
