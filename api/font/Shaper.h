/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string_view>
#include <vector>

#include "Bidi.h"
#include "FaceStyle.h"
#include "Feature.h"
#include "ShapedRun.h"

#include <boost/shared_ptr.hpp>

namespace v3d::font {

class FallbackChain;

/**
 * Shapes text into glyphs as its fonts define: kerning, marks, ligatures and contextual forms.
 *
 * A string is split into runs. A character stays in the run before it when that run's face
 * has it, or is the face the chain finds for it. A space or a combining mark therefore does not
 * start a run of its own. Otherwise a character starts a run in the face the chain finds for
 * it. A run also ends where the script or the embedding level changes. A character that belongs
 * to no script, such as a space, a digit or a mark, takes the script of the run it is in, or of
 * the first character after it with one.
 *
 * Each run's embedding level is resolved as UAX #9 resolves it, and its direction is its level's,
 * so a number in an Arabic sentence is shaped left to right. Runs are in the string's order, and
 * a line puts them in visual order by their levels.
 **/
class Shaper final {
 public:
    /**
     * The largest size text is shaped at, in pixels.
     **/
    static constexpr float LARGEST = 4096.0F;

    explicit Shaper(const boost::shared_ptr<FallbackChain>& chain);

    /**
     * Shape a UTF-8 string at a size in pixels.
     *
     * @param features turned on or off across the whole string. A tag that is not four letters
     *        is left out
     * @param base the direction each paragraph of the string is read in
     * @return its runs, or none for an empty string, a size that is not from 0 to LARGEST, or
     *         a chain with no faces
     **/
    std::vector<ShapedRun> shape(std::string_view utf8, const FaceStyle& style, float size,
        const std::vector<Feature>& features = {}, BaseDirection base = BaseDirection::Auto) const;

    /**
     * Shape the bytes from one offset to another of a UTF-8 string, with the levels of the whole
     * string. The rest of the string is the context, so the characters either side of the range
     * shape its edges as they would inside one run.
     *
     * @param bidi resolved over the whole string
     * @return the range's runs, or none for an empty range, a range out of the string, a size
     *         that is not from 0 to LARGEST, or a chain with no faces
     **/
    std::vector<ShapedRun> shape(std::string_view utf8, std::size_t from, std::size_t to, const Bidi& bidi, const FaceStyle& style, float size,
        const std::vector<Feature>& features = {}) const;

 private:
    boost::shared_ptr<FallbackChain> chain_;
};

};  // namespace v3d::font
