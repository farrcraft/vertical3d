/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Font.h"

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::font::texture {
class FontCache {
 public:
    /**
     * @param depth bytes per atlas texel: 1 for coverage or a distance field, 3 for subpixel
     *        coverage
     **/
    FontCache(unsigned int width, unsigned int height, unsigned int depth, const boost::shared_ptr<v3d::log::Logger> & logger);

    boost::shared_ptr<Font> find(const std::string & filename, float size);
    void add(const boost::shared_ptr<Font>& font);
    bool remove(const boost::shared_ptr<Font>& font);

    boost::shared_ptr<v3d::image::TextureAtlas> atlas();

 private:
    boost::shared_ptr<v3d::image::TextureAtlas> atlas_;
    std::vector<boost::shared_ptr<Font> > fonts_;
    boost::shared_ptr<v3d::log::Logger> logger_;
};
};  // namespace v3d::font::texture
