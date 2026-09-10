/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "BitmapFont.h"

#include <api/image/Factory.h>
#include <api/image/Texture.h>

#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#include <boost/make_shared.hpp>

namespace v3d::font {
std::pair<std::string, std::string> tokenize(const std::string& token) {
    size_t pos = token.find('=');
    std::string key = token.substr(0, pos);
    std::string value = token.substr(pos + 1);
    std::pair<std::string, std::string> kv(key, value);
    return kv;
}

BitmapFont::BitmapFont(const std::string& path, const std::string& name, const boost::shared_ptr<v3d::log::Logger>& logger) :
    logger_(logger) {
    std::string filename = path + name + std::string(".fnt");
    loadCharset(filename);
    filename = path + charset_.fileName_;
    if (!loadTexture(filename)) {
        std::string err = std::string("Error opening font image: ") + filename;
        throw std::runtime_error(err);
    }
}

void BitmapFont::readCommonLine(std::stringstream* line) {
    while (!line->eof()) {
        std::string token;
        *line >> token;
        std::pair<std::string, std::string> kv = tokenize(token);
        std::stringstream converter;
        converter << kv.second;
        if (kv.first == "lineHeight") {
            converter >> charset_.lineHeight_;
        } else if (kv.first == "base") {
            converter >> charset_.base_;
        } else if (kv.first == "scaleW") {
            converter >> charset_.width_;
        } else if (kv.first == "scaleH") {
            converter >> charset_.height_;
        } else if (kv.first == "pages") {
            converter >> charset_.pages_;
        } else if (kv.first == "packed" || kv.first == "alphaChnl" || kv.first == "redChnl" ||
                   kv.first == "greenChnl" || kv.first == "blueChnl") {
            // the channel layout of the page image, which the texture reader derives
            // from the file itself
        }
    }
}

void BitmapFont::readPageLine(std::stringstream* line) {
    while (!line->eof()) {
        std::string token;
        *line >> token;
        std::pair<std::string, std::string> kv = tokenize(token);
        if (kv.first == "file") {
            charset_.fileName_ = kv.second.substr(1, kv.second.length() - 2);  // get raw filename from quoted value
        }
    }
}

void BitmapFont::readCharLine(std::stringstream* line) {
    uint16_t id = 0;
    CharDescriptor desc;
    while (!line->eof()) {
        std::string token;
        *line >> token;
        std::pair<std::string, std::string> kv = tokenize(token);
        std::stringstream converter;
        converter << kv.second;
        if (kv.first == "id") {
            converter >> id;
        } else if (kv.first == "x") {
            converter >> desc.x_;
        } else if (kv.first == "y") {
            converter >> desc.y_;
        } else if (kv.first == "width") {
            converter >> desc.width_;
        } else if (kv.first == "height") {
            converter >> desc.height_;
        } else if (kv.first == "xoffset") {
            converter >> desc.xOffset_;
        } else if (kv.first == "yoffset") {
            converter >> desc.yOffset_;
        } else if (kv.first == "xadvance") {
            converter >> desc.xAdvance_;
        } else if (kv.first == "page") {
            converter >> desc.page_;
        }
    }
    charset_.chars_[id] = desc;
}

void BitmapFont::loadCharset(const std::string& filename) {
    std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
    if (!file) {
        std::string err = std::string("Error opening font file: ") + filename;
        throw std::runtime_error(err);
    }

    std::string line;
    std::string token;

    while (!file.eof()) {
        std::stringstream str;
        std::getline(file, line);
        str << line;
        str >> token;
        // first token in line indicates the type of line - one of: info, common, page,
        // chars, char. info and chars carry nothing this reader keeps: the character
        // count is the size of the map the char lines fill.
        if (token == "common") {
            readCommonLine(&str);
        } else if (token == "page") {  // page # & associated bitmap filename
            readPageLine(&str);
        } else if (token == "char") {  // character data
            readCharLine(&str);
        }
    }
}

BitmapFont::CharDescriptor BitmapFont::character(char c) {
    return charset_.chars_[c];
}

boost::shared_ptr<v3d::image::Texture> BitmapFont::texture() {
    return texture_;
}

uint16_t BitmapFont::charsetWidth() const {
    return charset_.width_;
}

uint16_t BitmapFont::charsetHeight() const {
    return charset_.height_;
}

uint16_t BitmapFont::lineHeight() const {
    return charset_.lineHeight_;
}

bool BitmapFont::loadTexture(const std::string& filename) {
    v3d::image::Factory factory(logger_);
    boost::shared_ptr<v3d::image::Image> image;

    try {
        image = factory.read(filename);
    }
    catch (const std::string&) {
        return false;
    }

    texture_ = boost::make_shared<v3d::image::Texture>(image);

    return true;
}

};  // namespace v3d::font
