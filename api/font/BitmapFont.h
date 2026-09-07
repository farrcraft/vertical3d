/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <stdint.h>

#include <map>
#include <sstream>
#include <string>

#include <boost/shared_ptr.hpp>

#include "../image/Texture.h"
#include "../log/Logger.h"

namespace v3d::font {
class BitmapFont {
 public:
    BitmapFont(const std::string & path, const std::string & name, const boost::shared_ptr<v3d::log::Logger> & logger);

    // A char line in a .fnt names its fields by key, so a malformed or abbreviated one
    // leaves whichever it omits unwritten. The descriptor is copied into the charset
    // whatever the line held, so every field has to start from a defined value.
    struct CharDescriptor {
        uint16_t x_ = 0;
        uint16_t y_ = 0;
        uint16_t width_ = 0;
        uint16_t height_ = 0;
        int16_t xOffset_ = 0;
        uint16_t yOffset_ = 0;
        uint16_t xAdvance_ = 0;
        uint16_t page_ = 0;
        uint16_t channel_ = 0;
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

    /**
     * One line of the .fnt file, read as the type its first token named. The info and
     * chars lines carry nothing this reader keeps, so neither has one of these.
     **/
    void readCommonLine(std::stringstream* line);
    void readPageLine(std::stringstream* line);
    void readCharLine(std::stringstream* line);

    bool loadTexture(const std::string & filename);

 private:
    Charset charset_;
    boost::shared_ptr<v3d::image::Texture> texture_;
    boost::shared_ptr<v3d::log::Logger> logger_;
};
};  // namespace v3d::font
