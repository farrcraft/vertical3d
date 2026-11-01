/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "FallbackChain.h"

#include <vector>

#include "Face.h"
#include "FaceGlyph.h"
#include "FaceStyle.h"
#include "Family.h"

#include <boost/shared_ptr.hpp>

namespace v3d::font {

void FallbackChain::add(const boost::shared_ptr<Family>& family) {
    if (family && !family->empty()) {
        families_.push_back(family);
    }
}

const std::vector<boost::shared_ptr<Family>>& FallbackChain::families() const noexcept {
    return families_;
}

std::vector<boost::shared_ptr<Face>> FallbackChain::faces(const FaceStyle& style) const {
    std::vector<boost::shared_ptr<Face>> found;
    found.reserve(families_.size());
    for (const boost::shared_ptr<Family>& family : families_) {
        found.push_back(family->face(style));
    }
    return found;
}

FaceGlyph FallbackChain::find(char32_t charcode, const FaceStyle& style) const {
    const std::vector<boost::shared_ptr<Face>> tried = faces(style);
    for (const boost::shared_ptr<Face>& face : tried) {
        const unsigned int index = face->glyphIndex(charcode);
        if (index != 0) {
            return { .face = face, .index = index };
        }
    }
    return { .face = tried.empty() ? boost::shared_ptr<Face>() : tried.front(), .index = 0 };
}

};  // namespace v3d::font
