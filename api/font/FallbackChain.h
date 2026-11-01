/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "FaceGlyph.h"
#include "FaceStyle.h"

#include <boost/shared_ptr.hpp>

namespace v3d::font {

class Face;
class Family;

/**
 * Families tried in order for each code point, so text that mixes scripts draws as one font.
 *
 * For a weight and a style, each family gives its face for them, as Family::face() chooses it.
 * A code point is drawn from the first of those faces that has it. A code point no face has is
 * drawn as the first face's missing glyph.
 **/
class FallbackChain final {
 public:
    /**
     * Add a family after those already added. An empty family is left out.
     **/
    void add(const boost::shared_ptr<Family>& family);

    const std::vector<boost::shared_ptr<Family>>& families() const noexcept;

    /**
     * @return each family's face for a weight and a style, in the chain's order
     **/
    std::vector<boost::shared_ptr<Face>> faces(const FaceStyle& style) const;

    /**
     * @return the glyph a code point is drawn with, at a weight and a style. Its face is null
     *         for a chain with no families
     **/
    FaceGlyph find(char32_t charcode, const FaceStyle& style) const;

 private:
    std::vector<boost::shared_ptr<Family>> families_;
};

};  // namespace v3d::font
