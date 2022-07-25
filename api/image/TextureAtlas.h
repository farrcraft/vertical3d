/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>

#include "../log/Logger.h"

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

        glm::ivec4 region(unsigned int width, unsigned int height);
        void region(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char * data, unsigned int stride);

     protected:
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
