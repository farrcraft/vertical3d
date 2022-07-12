/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <stdint.h>

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include "../image/Texture.h"
#include "../core/Logger.h"

namespace v3d::font {
    class BitmapFont {
     public:
        BitmapFont(const std::string & path, const std::string & name, const boost::shared_ptr<v3d::core::Logger> & logger);

        struct CharDescriptor {
            uint16_t x_;
            uint16_t y_;
            uint16_t width_;
            uint16_t height_;
            int16_t xOffset_;
            uint16_t yOffset_;
            uint16_t xAdvance_;
            uint16_t page_;
            uint16_t channel_;
        };

        struct Charset {
            uint16_t lineHeight_;
            uint16_t base_;
            uint16_t width_;
            uint16_t height_;
            uint16_t pages_;
            std::string fileName_;
            std::map<uint16_t, CharDescriptor> chars_;
        };

        uint16_t charsetWidth() const;
        uint16_t charsetHeight() const;
        uint16_t lineHeight() const;

        CharDescriptor character(char c);
        boost::shared_ptr<v3d::image::Texture> texture();

     protected:
        void loadCharset(const std::string & filename);
        bool loadTexture(const std::string & filename);

     private:
        Charset charset_;
        boost::shared_ptr<v3d::image::Texture> texture_;
        boost::shared_ptr<v3d::core::Logger> logger_;
    };
};  // namespace v3d::font
