/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>

#include <api/font/text/Bidi.h>
#include <api/font/text/Segmenter.h>
#include <api/font/text/style/Range.h>

#include <cstddef>
#include <list>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Text.h"

#include <boost/shared_ptr.hpp>

namespace v3d::font::text::shaping {

/**
 * Shapes texts and keeps the most recently used, so text shaped again with the same glyphs is
 * found rather than shaped.
 *
 * A text is found by its string, its base direction, and the ranges of its styles as they
 * shape. Adjacent ranges that differ only in colour or decoration are one range here, so
 * recolouring text finds what was shaped before. When more texts are kept than the capacity,
 * the one used longest ago is let go. A paragraph holding a shaped text keeps it whether or
 * not the cache does.
 *
 * Every paragraph of an app shares one cache. It is used from one thread.
 **/
class Cache final {
 public:
    /**
     * The number of texts kept unless another is given.
     **/
    static constexpr std::size_t CAPACITY = 256;

    /**
     * @param locale the BCP 47 language tag whose rules divide words and lines, and the language
     *        text is shaped in when its style names none
     * @param logger where an error is written when the locale's rules cannot be loaded
     * @param capacity how many texts are kept. Zero is taken as one
     **/
    Cache(const std::string& locale, const boost::shared_ptr<v3d::log::Logger>& logger, std::size_t capacity = CAPACITY);

    /**
     * @param text the UTF-8 string to shape
     * @param ranges the styles over the text, in order, each starting where the last ended. A
     *        text whose ranges do not cover it in that way is shaped in the first range's style
     *        throughout
     * @param base the direction each paragraph of the text is read in
     * @return the text shaped, found or made
     **/
    boost::shared_ptr<const Text> shape(std::string_view text, const std::vector<style::Range>& ranges, BaseDirection base);

    /**
     * @return how many texts have been shaped rather than found
     **/
    std::size_t shaped() const noexcept;

    /**
     * @return how many texts are kept
     **/
    std::size_t size() const noexcept;

 private:
    struct Entry {
        boost::shared_ptr<const Text> text;
        std::list<std::string>::iterator used;
    };

    Segmenter segmenter_;
    std::string locale_;
    std::size_t capacity_;
    std::size_t shaped_ = 0;
    std::list<std::string> order_;  // keys, most recently used first
    std::unordered_map<std::string, Entry> entries_;
};

};  // namespace v3d::font::text::shaping
