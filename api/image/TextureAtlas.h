/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::image {
class Image;
/**
 **/
class TextureAtlas {
 public:
    TextureAtlas(unsigned int width, unsigned int height, unsigned int depth, const boost::shared_ptr<v3d::log::Logger> & logger);
    ~TextureAtlas();

    unsigned int width() const;
    unsigned int height() const;
    unsigned int depth() const;
    unsigned int id() const;

    boost::shared_ptr<Image> image();
    void write(const std::string & filename);

    /**
     * Reserve room for an image of this size, with a gutter around it.
     *
     * The atlas always reserves a one-texel gutter of zeroed texels on all four sides of
     * the region. Linear filtering at the region's edge therefore never samples a
     * neighbouring region.
     *
     * The rectangle returned is the usable one: the size asked for, positioned inside the
     * gutter. Pass it to the blit below and use it for texture coordinates as it is. A
     * caller asks for the size it will blit, and neither adds the gutter to the request
     * nor subtracts it from the result.
     *
     * @return the usable rectangle, or x and y of -1 when the atlas has no room left
     **/
    glm::ivec4 region(unsigned int width, unsigned int height);
    void region(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char * data, unsigned int stride);

 protected:
    glm::ivec4 allocate(unsigned int width, unsigned int height);
    int fit(unsigned int index, unsigned int width, unsigned int height);
    void merge();

 private:
    std::vector<glm::ivec3> nodes_;
    unsigned int width_;
    unsigned int height_;
    unsigned int depth_;
    size_t used_;
    unsigned int id_;
    boost::shared_ptr<Image> image_;
    boost::shared_ptr<v3d::log::Logger> logger_;
};
};  // namespace v3d::image
