/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "FaceStyle.h"

#include <boost/shared_ptr.hpp>

namespace v3d::font {

class Face;

/**
 * The faces of one typeface, such as Inter Regular and Inter SemiBold, chosen by weight and
 * style as CSS chooses them.
 *
 * Faces of the style asked for are tried first, and all faces when there are none. Among them,
 * the weight asked for is used when a face has it. A variable face has every weight its weight
 * axis covers, and is measured from the end of the axis nearest the weight. Otherwise:
 *
 * - For a weight from 400 to 500, the nearest heavier face up to 500, then the nearest lighter
 *   face, then the nearest face heavier than 500.
 * - For a weight below 400, the nearest lighter face, then the nearest heavier one.
 * - For a weight above 500, the nearest heavier face, then the nearest lighter one.
 **/
class Family final {
 public:
    explicit Family(const std::string& name);

    /**
     * Add a face. A null face, from a file that did not open, is left out.
     **/
    void add(const boost::shared_ptr<Face>& face);

    const std::string& name() const noexcept;
    const std::vector<boost::shared_ptr<Face>>& faces() const noexcept;
    bool empty() const noexcept;

    /**
     * @return the face for a weight and a style, or null for a family with no faces
     **/
    boost::shared_ptr<Face> face(const FaceStyle& style) const;

 private:
    std::string name_;
    std::vector<boost::shared_ptr<Face>> faces_;
};

};  // namespace v3d::font
