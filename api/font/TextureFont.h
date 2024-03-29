/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>
#include <string>

#include "../image/TextureAtlas.h"

#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>

#include "../log/Logger.h"

namespace v3d::font {

    class TextureFont {
     public:
        typedef struct {
            wchar_t charcode_;
            float kerning_;
        } Kerning;

        typedef enum {
            OUTLINE_TYPE_NONE = 0,
            OUTLINE_TYPE_LINE = 1,
            OUTLINE_TYPE_INNER = 2,
            OUTLINE_TYPE_OUTER = 3
        } OutlineType;

        typedef struct {
            unsigned int id_;
            wchar_t charcode_;
            unsigned int width_;
            unsigned int height_;
            glm::ivec2 offset_;
            glm::vec2 advance_;
            glm::vec2 st_[2];
            std::vector<Kerning> kerning_;
            OutlineType outline_;
            float outlineThickness_;
        } Glyph;

        TextureFont(const std::string& filename, float size, const boost::shared_ptr<v3d::log::Logger> & logger);

        boost::shared_ptr<Glyph> glyph(wchar_t charcode);
        float kerning(boost::shared_ptr<Glyph>, wchar_t charcode);

        /**
         **/
        void atlas(boost::shared_ptr<v3d::image::TextureAtlas> atlas);

        bool loadGlyphs(const wchar_t * charcodes);

        float size() const;
        std::string filename() const;

        float ascender() const;
        float descender() const;
        float height() const;
        float linegap() const;
        float underlinePosition() const;
        float underlineThickness() const;

     protected:
        void generateKerning();
        boost::shared_ptr<Glyph> createGlyph();

     private:
        std::vector<boost::shared_ptr<Glyph> > glyphs_;
        boost::shared_ptr<v3d::image::TextureAtlas> atlas_;
        std::string filename_;
        float size_;
        int hinting_;
        OutlineType outline_;
        float outlineThickness_;
        int lcdFiltering_;
        unsigned char lcdWeights_[5];
        float height_;
        float linegap_;
        float ascender_;
        float descender_;
        float underlinePosition_;
        float underlineThickness_;

        class Freetype;
        boost::shared_ptr<Freetype> freetype_;
        boost::shared_ptr<v3d::log::Logger> logger_;
    };
};  // namespace v3d::font
