/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/face/Style.h>
#include <api/font/face/Variation.h>

#include <string_view>
#include <vector>

#include "Bidi.h"
#include "Feature.h"
#include "ShapedRun.h"

#include <boost/shared_ptr.hpp>

namespace v3d::font::face {

class FallbackChain;

};  // namespace v3d::font::face

namespace v3d::font::text {

/**
 * Shapes text into glyphs as its fonts define: kerning, marks, ligatures and contextual forms.
 *
 * A string is split into runs, a cluster at a time. A cluster is a character and the marks,
 * joiners, variation selectors and emoji modifiers after it. A zero width joiner joins the next
 * character to it, and two regional indicators are one cluster, a flag. A cluster asks for emoji
 * when it holds the emoji variation selector or a keycap, or starts with a character shown as
 * an emoji by default, and has no text variation selector.
 *
 * A cluster stays in the run before it when that run's face is the one found for it, or holds
 * it and is not a colour face. A space after an emoji is therefore set in the text's face, and a
 * space or a mark otherwise does not start a run of its own. Otherwise a cluster starts a run.
 * A cluster that asks for emoji is set in the chain's first colour face that has its first
 * character. Any other is set in the chain's first face that has all its visible characters.
 * When no face has them all, it is set in the face the chain finds for its first character.
 *
 * A run also ends where the script or the embedding level changes. A character that belongs to
 * no script, such as a space, a digit or a mark, takes the script of the run it is in, or of the
 * first character after it with one.
 *
 * Each run is shaped at the coordinates Face::coordinates() finds in its face for the style,
 * the size and the variations, and holds them for its glyphs to be drawn at.
 *
 * A language, as a BCP 47 tag, picks the forms a face draws for it through OpenType's language
 * systems: Japanese and Chinese text in one CJK face take their own forms of a shared character.
 * With none, each face's default forms are drawn.
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

    explicit Shaper(const boost::shared_ptr<face::FallbackChain>& chain);

    /**
     * Shape a UTF-8 string at a size in pixels.
     *
     * @param utf8 the string to shape
     * @param style the weight and style each face is chosen by
     * @param size the size to shape at, in pixels
     * @param features turned on or off across the whole string. A tag that is not four letters
     *        is left out
     * @param variations set on each variable face's axes across the whole string
     * @param language the BCP 47 tag of the string's language, or empty for none
     * @param base the direction each paragraph of the string is read in
     * @return its runs, or none for an empty string, a size that is not from 0 to LARGEST, or
     *         a chain with no faces
     **/
    std::vector<ShapedRun> shape(std::string_view utf8, const face::Style& style, float size,
        const std::vector<Feature>& features = {}, const std::vector<face::Variation>& variations = {}, std::string_view language = {},
        BaseDirection base = BaseDirection::Auto) const;

    /**
     * Shape the bytes from one offset to another of a UTF-8 string, with the levels of the whole
     * string. The rest of the string is the context, so the characters either side of the range
     * shape its edges as they would inside one run.
     *
     * @param utf8 the whole string
     * @param from the byte offset the range starts at
     * @param to the byte offset just past the range's end
     * @param bidi resolved over the whole string
     * @param style the weight and style each face is chosen by
     * @param size the size to shape at, in pixels
     * @param features turned on or off across the range. A tag that is not four letters is
     *        left out
     * @param variations set on each variable face's axes across the range
     * @param language the BCP 47 tag of the string's language, or empty for none
     * @return the range's runs, or none for an empty range, a range out of the string, a size
     *         that is not from 0 to LARGEST, or a chain with no faces
     **/
    std::vector<ShapedRun> shape(std::string_view utf8, std::size_t from, std::size_t to, const Bidi& bidi, const face::Style& style, float size,
        const std::vector<Feature>& features = {}, const std::vector<face::Variation>& variations = {}, std::string_view language = {}) const;

 private:
    boost::shared_ptr<face::FallbackChain> chain_;
};

};  // namespace v3d::font::text
