/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
 **/

#include "Font2D.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <cmath>
#include <algorithm>

#include <boost/make_shared.hpp>

#include "../type/3dtypes.h"

namespace v3d::font {
    Font2D::Font2D() : typeface_("Courier New"), size_(24), line_height_(0), tex_line_height_(0.0f) {
    }

    Font2D::Font2D(const std::string& face, unsigned int size) : typeface_(face), size_(size), line_height_(0), tex_line_height_(0.0f) {
    }

    Font2D::~Font2D() {
    }

    unsigned int Font2D::style(void) const {
        return style_;
    }

    void Font2D::style(unsigned int attribs) {
        style_ = attribs;
    }

    int Font2D::size(void) const {
        return size_;
    }

    void Font2D::size(int s) {
        size_ = s;
    }

    std::string Font2D::typeface(void) const {
        return typeface_;
    }

    void Font2D::typeface(const std::string& face) {
        typeface_ = face;
    }

    boost::shared_ptr<v3d::image::Texture> Font2D::texture(void) const {
        return texture_;
    }

    // NOTE: fonts aren't locale or language aware - there is a limited predefined character range supported.
    // freetype font loading based on gpwiki sample code
    bool Font2D::build(const boost::shared_ptr<v3d::core::Logger>& logger) {
        // the set of all characters to be loaded from the font file
        const std::string charset("abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "1234567890~!@#$%^&*()-=+;:"
            " _'\",./?[]|\\<>{}`\xFF");

        LOG_DEBUG(logger) << "Font2D::build - Building font: " << typeface_;

        // initialize freetype library
        FT_Library library;
        FT_Error error;
        if ((error = FT_Init_FreeType(&library)) != 0) {
            LOG_DEBUG(logger) << "Font2D::build - Error initializing freetype library!";
            return false;
        }
        // load font file
        FT_Face face;
        std::string filename(typeface_);
        if ((error = FT_New_Face(library, filename.c_str(), 0, &face)) != 0) {
            LOG_DEBUG(logger) << "Font2D::build - Error creating new freetype face!";
            return false;
        }
        // make sure this is a scalable truetype font
        if (!(face->face_flags & FT_FACE_FLAG_SCALABLE) || !(face->face_flags & FT_FACE_FLAG_HORIZONTAL)) {
            LOG_DEBUG(logger) << "Font2D::build - Not a scalable TTF font!";
            return false;
        }

        // set the font size
        if ((error = FT_Set_Pixel_Sizes(face, size_, 0)) != 0) {
            LOG_DEBUG(logger) << "Font2D::build - Error setting font pixel size!";
            return false;
        }

        // horizontal character padding
        const unsigned int pad = 3;
        // texture dimensions - width is fixed
        unsigned int image_width = 256;
        // image height is determined by the number of rows of character images are required
        unsigned int image_height = 0;
        // we need to figure out the maximum descent & ascent of all the glyphs in the charset
        int max_descent = 0;
        int max_ascent = 0;
        // the number of rows of glyphs that will be in the texture
        unsigned int rows = 1;
        // the space used on the current row by glyphs so far
        unsigned int row_space = image_width - pad;

        // iterate over each character we want to use in the font
        for (size_t i = 0; i < charset.size(); i++) {
            unsigned int charcode = charset[i];
            if (charset[i] == '\xFF') {
                charcode = 0;
            }

            // load the glyph and render the bitmap
            if ((error = FT_Load_Char(face, charcode, FT_LOAD_RENDER)) != 0) {
                LOG_DEBUG(logger) << "Font2D::build - Error loading character [" << charcode << "] from typeface!";
                return false;
            }
            // get the advance in pixels and added padding
            // glyph metrics are stored in 26.6 pixel format (1/64 pixels)
            unsigned int advance = (face->glyph->metrics.horiAdvance >> 6) + pad;

            // not enough room on this row for this glyph
            if (advance > row_space) {
                // reset counter and increment row count
                row_space = image_width - pad;
                rows++;
            }
            // take up space for this glyph in the row
            row_space -= advance;

            max_ascent = std::max(face->glyph->bitmap_top, max_ascent);
            max_descent = std::max((static_cast<int>(face->glyph->bitmap.rows) - face->glyph->bitmap_top), max_descent);
        }

        // calculate final texture size
        unsigned int requested_height = (max_ascent + max_descent + pad) * rows + pad;
        // the size must be a power of two
        image_height = npot(requested_height);

        boost::shared_ptr<v3d::image::Image> image = boost::make_shared<v3d::image::Image>(image_width, image_height, 8);

        unsigned int x = pad;
        unsigned int y = pad + max_ascent;

        for (size_t i = 0; i < charset.size(); i++) {
            unsigned char charcode = charset[i];
            if (charset[i] == '\xFF') {
                charcode = 0;
            }

            // load the glyph and render the bitmap
            if ((error = FT_Load_Char(face, charcode, FT_LOAD_RENDER)) != 0) {
                LOG_DEBUG(logger) << "Font2D::build - Error loading character [" << charcode << "] !";
                return false;
            }
            unsigned int advance = (face->glyph->metrics.horiAdvance >> 6) + pad;
            if (advance > image_width - x) {
                x = pad;
                y += (max_ascent + max_descent + pad);
            }
            // create glyph
            Glyph glyph;
            glyph.x1_ = static_cast<float>(x) / image_width;
            glyph.x2_ = static_cast<float>(x + (advance - pad)) / image_width;
            glyph.y1_ = static_cast<float>((y - max_ascent)) / image_height;
            glyph.advance_ = (advance - pad);

            glyphs_[charcode] = glyph;

            // copy bitmap to texture image
            for (unsigned int row = 0; row < face->glyph->bitmap.rows; row++) {
                for (unsigned int pixel = 0; pixel < face->glyph->bitmap.width; pixel++) {
                    (*image)[(x + face->glyph->bitmap_left + pixel) + (y - face->glyph->bitmap_top + row) * image_width] =
                        face->glyph->bitmap.buffer[pixel + row * face->glyph->bitmap.pitch];
                }
            }
            x += advance;
        }

        FT_Done_FreeType(library);

        texture_ = boost::make_shared<v3d::image::Texture>(image);

        line_height_ = max_ascent + max_descent;
        tex_line_height_ = static_cast<float>(line_height_) / image_height;

        return true;
    }

    unsigned int Font2D::width(const std::string& str) const {
        unsigned int w = 0;
        for (size_t i = 0; i < str.size(); i++) {
            unsigned char ch = str[i];
            w += width(ch);
        }
        return w;
    }

    unsigned int Font2D::height(void) const {
        return line_height_;
    }

    float Font2D::textureHeight(void) const {
        return tex_line_height_;
    }

    unsigned int Font2D::width(unsigned char charcode) const {
        std::map<unsigned char, Glyph>::const_iterator iter = glyphs_.find(charcode);
        if (iter != glyphs_.end()) {
            return iter->second.advance_;
        }
        return 0;
    }

    const Font2D::Glyph* Font2D::glyph(unsigned char charcode) const {
        std::map<unsigned char, Glyph>::const_iterator iter = glyphs_.find(charcode);
        if (iter == glyphs_.end()) {
            iter = glyphs_.begin();
        }
        const Glyph* glyph = &(iter->second);
        return glyph;
    }

};  // namespace v3d::font
