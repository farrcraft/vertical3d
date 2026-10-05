/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Image.h>
#include <api/log/Logger.h>

#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline {

/**
 * An image as SL's `texture()` reads it: colours in [0, 1], sampled bilinearly between
 * texel centres and wrapped periodically, which are RI's defaults.
 *
 * s runs left to right and t top to bottom, so (0, 0) is the image's upper left corner.
 **/
class Texture final {
 public:
    /** A grey image is read into all three channels, and an alpha channel is dropped. **/
    explicit Texture(const v3d::image::Image & image);

    glm::vec3 sample(float s, float t) const;

    unsigned int width() const;
    unsigned int height() const;

 private:
    /** The texel at a column and row, either of which may be outside the image. **/
    const glm::vec3 & texel(int column, int row) const;

    unsigned int width_ = 0;
    unsigned int height_ = 0;
    std::vector<glm::vec3> texels_;
};

/**
 * The textures a frame's shaders name, each read once.
 *
 * There is no `txmake` here: the image a scene names is the texture, so `MakeTexture` has
 * nothing to make. A name that does not read is remembered as missing, so a scene naming it
 * on a thousand grids tries once.
 **/
class Textures final {
 public:
    explicit Textures(const boost::shared_ptr<v3d::log::Logger> & logger);

    /**
     * Where a relative name is looked for, as `Option "searchpath" "texture"` gives it. A
     * name found on none of them is tried as it is, against the working directory.
     **/
    void searchpath(const std::string & path);

    /** The texture of that name, or null when it cannot be read. **/
    const Texture* find(const std::string & name);

 private:
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::vector<std::string> directories_;
    std::map<std::string, boost::shared_ptr<Texture> > textures_;
};

};  // namespace v3d::render::offline
