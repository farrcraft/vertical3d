/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureFont.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_LCD_FILTER_H

#include <algorithm>
#include <cmath>
#include <string>

#include <boost/make_shared.hpp>

#include "../image/TextureAtlas.h"
#include "../log/Logger.h"

namespace v3d::font {
class TextureFont::Freetype {
 public:
     explicit Freetype(const boost::shared_ptr<v3d::log::Logger> & logger);

    bool loadFace(const std::string& filename, float size);

    void release();

    FT_Library library_;
    FT_Face face_;
    boost::shared_ptr<v3d::log::Logger> logger_;
};

TextureFont::Freetype::Freetype(const boost::shared_ptr<v3d::log::Logger>& logger) : logger_(logger) {
}

void TextureFont::Freetype::release() {
    FT_Done_Face(face_);
    FT_Done_FreeType(library_);
}

bool TextureFont::Freetype::loadFace(const std::string& filename, float size) {
    // initialize freetype library
    FT_Error error;
    error = FT_Init_FreeType(&library_);
    if (error != 0) {
        logger_->get()->error("Error initializing freetype library!");
        return false;
    }

    // load font file
    error = FT_New_Face(library_, filename.c_str(), 0, &face_);
    if (error != 0) {
        logger_->get()->error("Error creating new freetype face!");
        FT_Done_FreeType(library_);
        return false;
    }

    // select charmap
    error = FT_Select_Charmap(face_, FT_ENCODING_UNICODE);
    if (error != 0) {
        logger_->get()->error("Error selecting freetype charmap!");
        release();
        return false;
    }

    // set char size
    // size *= 100.0f;
    const FT_UInt hres = 64;
    error = FT_Set_Char_Size(face_, static_cast<int>((size * 64)), 0, 72 * hres, 72);
    if (error != 0) {
        logger_->get()->error("Error setting freetype char size!");
        release();
        return false;
    }

    // set transform matrix
    FT_Matrix matrix = {
        static_cast<int>((1.0 / hres) * 0x10000L),
        static_cast<int>(0.0 * 0x10000L),
        static_cast<int>(0.0 * 0x10000L),
        static_cast<int>(1.0 * 0x10000L)
    };
    FT_Set_Transform(face_, &matrix, NULL);

    return true;
}

TextureFont::TextureFont(const std::string& filename, float size, const boost::shared_ptr<v3d::log::Logger> & logger) :
    atlas_(nullptr),
    filename_(filename),
    size_(size),
    height_(0),
    ascender_(0),
    descender_(0),
    outline_(OUTLINE_TYPE_NONE),
    outlineThickness_(0.0f),
    hinting_(1),
    lcdFiltering_(1),
    logger_(logger) {
    lcdWeights_[0] = 0x10;
    lcdWeights_[1] = 0x40;
    lcdWeights_[2] = 0x70;
    lcdWeights_[3] = 0x40;
    lcdWeights_[4] = 0x10;

    freetype_ = boost::make_shared<Freetype>(logger);

    // the face is loaded at its own size. Asking for a hundred times the size - the
    // upstream trick for reading the metrics with more precision - overflows what
    // FT_Set_Char_Size accepts at the 64x horizontal resolution this uses, and every
    // metric comes back zero.
    if (!freetype_->loadFace(filename_, size_)) {
        return;
    }

    // 64 * 64 because of 26.6 encoding AND the transform matrix used
    // in texture_font_load_face (hres = 64)
    underlinePosition_ = freetype_->face_->underline_position / (64.0f * 64.0f) * size_;
    underlinePosition_ = underlinePosition_ < 0.0f ? std::ceil(underlinePosition_ - 0.5f) : std::floor(underlinePosition_ + 0.5f);
    underlinePosition_ = std::min(underlinePosition_, -2.0f);

    underlineThickness_ = freetype_->face_->underline_thickness / (64.0f * 64.0f) * size_;
    underlineThickness_ = underlineThickness_ < 0.0f ? std::ceil(underlineThickness_ - 0.5f) : std::floor(underlineThickness_ + 0.5f);
    underlineThickness_ = std::max(underlineThickness_, 1.0f);

    // metrics are 26.6 fixed point, which carries the fractional pixel the /100 was
    // reaching for
    FT_Size_Metrics metrics = freetype_->face_->size->metrics;
    ascender_ = metrics.ascender / 64.0f;
    descender_ = metrics.descender / 64.0f;
    height_ = metrics.height / 64.0f;
    linegap_ = height_ - ascender_ + descender_;

    freetype_->release();
}

/**
 **/
void TextureFont::atlas(boost::shared_ptr<v3d::image::TextureAtlas> atlas) {
    atlas_ = atlas;
}

boost::shared_ptr<TextureFont::Glyph> TextureFont::createGlyph() {
    boost::shared_ptr<Glyph> glyph(new Glyph);

    glyph->id_ = 0;
    glyph->width_ = 0;
    glyph->height_ = 0;
    glyph->outline_ = OUTLINE_TYPE_NONE;
    glyph->outlineThickness_ = 0.0;
    glyph->offset_.x = 0;
    glyph->offset_.y = 0;
    glyph->advance_.x = 0.0f;
    glyph->advance_.y = 0.0f;
    glyph->st_[0].s = 0.0f;
    glyph->st_[0].t = 0.0f;
    glyph->st_[1].s = 0.0f;
    glyph->st_[1].t = 0.0f;

    return glyph;
}

boost::shared_ptr<TextureFont::Glyph> TextureFont::glyph(wchar_t charcode) {
    wchar_t lineCode = static_cast<wchar_t>(-1);

    for (unsigned int i = 0; i < glyphs_.size(); ++i) {
        if ((glyphs_[i]->charcode_ == charcode) &&
            ((charcode == lineCode) ||
                ((glyphs_[i]->outline_ == outline_) &&
                    (glyphs_[i]->outlineThickness_ == outlineThickness_)))) {
            return glyphs_[i];
        }
    }
    boost::shared_ptr<Glyph> glyph;
    // -1 is used for line drawing (overline, underline, strikethrough) and background
    if (charcode == lineCode) {
        glm::ivec4 region = atlas_->region(5, 5);

        if (region.x < 0) {
            logger_->get()->error("Texture atlas is full!");
            return glyph;
        }
        glyph = createGlyph();
        // opaque white, so a line takes the colour it is drawn with
        static unsigned char data[4 * 4 * 3] = {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
        };

        atlas_->region(region.x, region.y, 4, 4, data, 0);

        float width = static_cast<float>(atlas_->width());
        float height = static_cast<float>(atlas_->height());

        glyph->charcode_ = lineCode;

        glyph->st_[0].s = (region.x + 2) / width;
        glyph->st_[0].t = (region.y + 2) / height;
        glyph->st_[1].s = (region.x + 3) / width;
        glyph->st_[1].t = (region.y + 3) / height;

        glyphs_.push_back(glyph);

        return glyph;
    }

    wchar_t buffer[2] = { 0, 0 };
    buffer[0] = charcode;

    if (loadGlyphs(buffer)) {
        return glyphs_[glyphs_.size() - 1];
    }

    return glyph;
}

namespace {

/**
 * What one rendered glyph's bitmap is, whichever of the two paths produced it.
 **/
struct GlyphBitmap final {
    FT_Bitmap bitmap = {};
    int width = 0;
    int rows = 0;
    int top = 0;
    int left = 0;
};

/**
 * Owns the stroker for as long as the outline is being built.
 *
 * Every step between FT_Stroker_New and the bitmap can fail, and each of those failures is
 * a return - so the release belongs to the scope rather than to any one of them.
 **/
class StrokerHandle final {
 public:
    explicit StrokerHandle(FT_Stroker stroker) : stroker_(stroker) {
    }
    ~StrokerHandle() {
        FT_Stroker_Done(stroker_);
    }
    StrokerHandle(const StrokerHandle&) = delete;
    StrokerHandle& operator=(const StrokerHandle&) = delete;

    FT_Stroker get() const {
        return stroker_;
    }

 private:
    FT_Stroker stroker_;
};

/**
 * Owns the glyph the outline path builds, for the iteration that built it.
 *
 * Holding nothing is the normal case: a glyph with no outline is read out of the face's
 * own slot, which the face owns. Only FT_Get_Glyph hands back a copy to release, and it is
 * released however the iteration ends - a full atlas skips the rest of one, and a stroke
 * that fails abandons the whole run.
 **/
class GlyphHandle final {
 public:
    GlyphHandle() = default;
    ~GlyphHandle() {
        if (glyph_ != nullptr) {
            FT_Done_Glyph(glyph_);
        }
    }
    GlyphHandle(const GlyphHandle&) = delete;
    GlyphHandle& operator=(const GlyphHandle&) = delete;

    /**
     * Where FT_Get_Glyph writes. It is left alone by a failure before that call, and holds
     * whatever the last step that succeeded replaced it with after one.
     **/
    FT_Glyph* address() {
        return &glyph_;
    }

 private:
    FT_Glyph glyph_ = nullptr;
};

/**
 * The FT_LOAD flags one glyph is asked for under, and the lcd filter that goes with them.
 *
 * Setting the filter is part of choosing the flags rather than a step of its own: it only
 * means anything alongside FT_LOAD_TARGET_LCD.
 **/
FT_Int32 glyphLoadFlags(FT_Library library, TextureFont::OutlineType outline, bool hinting,
    unsigned int depth, bool lcdFiltering, const unsigned char* lcdWeights) {
    FT_Int32 flags = 0;
    if (outline != TextureFont::OUTLINE_TYPE_NONE) {
        flags |= FT_LOAD_NO_BITMAP;
    } else {
        flags |= FT_LOAD_RENDER;
    }

    if (!hinting) {
        flags |= FT_LOAD_NO_HINTING | FT_LOAD_NO_AUTOHINT;
    } else {
        flags |= FT_LOAD_FORCE_AUTOHINT;
    }

    if (depth == 3) {
        FT_Library_SetLcdFilter(library, FT_LCD_FILTER_LIGHT);
        flags |= FT_LOAD_TARGET_LCD;
        if (lcdFiltering) {
            FT_Library_SetLcdFilterWeights(library, const_cast<unsigned char*>(lcdWeights));
        }
    }
    return flags;
}

/**
 * Stroke the loaded glyph and render the outline to a bitmap.
 *
 * *glyph is whatever FT_Get_Glyph handed back, which the caller's GlyphHandle owns from
 * that point on: FT_Glyph_Stroke and FT_Glyph_To_Bitmap are both asked to destroy what they
 * replace, and only do so when they succeed. Releasing the face after a failure is the
 * caller's, which is where the face was loaded.
 **/
bool strokeGlyph(FT_Library library, FT_Face face, TextureFont::OutlineType outline, float thickness,
    unsigned int depth, FT_Glyph* glyph, GlyphBitmap* out,
    const boost::shared_ptr<v3d::log::Logger>& logger) {
    FT_Stroker raw;
    FT_Error error = FT_Stroker_New(library, &raw);
    if (error != 0) {
        logger->get()->error("Error creating stroker!");
        return false;
    }
    const StrokerHandle stroker(raw);
    FT_Stroker_Set(
        stroker.get(),
        static_cast<int>(thickness * 64),
        FT_STROKER_LINECAP_ROUND,
        FT_STROKER_LINEJOIN_ROUND,
        0);
    error = FT_Get_Glyph(face->glyph, glyph);
    if (error != 0) {
        logger->get()->error("Error getting glyph!");
        return false;
    }

    if (outline == TextureFont::OUTLINE_TYPE_LINE) {
        error = FT_Glyph_Stroke(glyph, stroker.get(), 1);
    } else if (outline == TextureFont::OUTLINE_TYPE_INNER) {
        error = FT_Glyph_StrokeBorder(glyph, stroker.get(), 0, 1);
    } else if (outline == TextureFont::OUTLINE_TYPE_OUTER) {
        error = FT_Glyph_StrokeBorder(glyph, stroker.get(), 1, 1);
    }
    if (error) {
        logger->get()->error("Error setting glyph stroke border!");
        return false;
    }

    error = FT_Glyph_To_Bitmap(glyph, depth == 1 ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_LCD, 0, 1);
    if (error != 0) {
        logger->get()->error("Error converting glyph to bitmap!");
        return false;
    }

    FT_BitmapGlyph bitmapGlyph = (FT_BitmapGlyph)*glyph;
    out->bitmap = bitmapGlyph->bitmap;
    out->width = bitmapGlyph->bitmap.width;
    out->rows = bitmapGlyph->bitmap.rows;
    out->top = bitmapGlyph->top;
    out->left = bitmapGlyph->left;
    return true;
}

};  // namespace

bool TextureFont::loadGlyphs(const wchar_t* charcodes) {
    if (!atlas_) {
        return false;
    }

    if (!freetype_->loadFace(filename_, size_)) {
        return false;
    }
    unsigned int missed = 0;
    for (unsigned int i = 0; i < wcslen(charcodes); ++i) {
        FT_UInt glyphIndex = FT_Get_Char_Index(freetype_->face_, charcodes[i]);
        const FT_Int32 flags = glyphLoadFlags(freetype_->library_, outline_, hinting_ != 0,
            atlas_->depth(), lcdFiltering_ != 0, lcdWeights_);

        FT_Error error = FT_Load_Glyph(freetype_->face_, glyphIndex, flags);
        if (error != 0) {
            logger_->get()->error("Error loading glyph!");
            FT_Done_FreeType(freetype_->library_);
            return false;
        }

        // the handle is the iteration's, so a stroke that fails and a full atlas both
        // release what FT_Get_Glyph built without either having to say so
        GlyphHandle stroked;
        GlyphBitmap rendered;
        if (outline_ == OUTLINE_TYPE_NONE) {
            FT_GlyphSlot loaded = freetype_->face_->glyph;
            rendered.bitmap = loaded->bitmap;
            rendered.width = loaded->bitmap.width;
            rendered.rows = loaded->bitmap.rows;
            rendered.top = loaded->bitmap_top;
            rendered.left = loaded->bitmap_left;
        } else if (!strokeGlyph(freetype_->library_, freetype_->face_, outline_, outlineThickness_,
                atlas_->depth(), stroked.address(), &rendered, logger_)) {
            freetype_->release();
            return false;
        }

        // We want each glyph to be separated by at least one black pixel
        unsigned int w = rendered.width / atlas_->depth() + 1;
        unsigned int h = rendered.rows + 1;
        glm::ivec4 region = atlas_->region(w, h);
        if (region.x < 0) {
            missed++;
            logger_->get()->error("Texture atlas is full!");
            continue;
        }
        w = w - 1;
        h = h - 1;
        unsigned int x = region.x;
        unsigned int y = region.y;
        atlas_->region(x, y, w, h, rendered.bitmap.buffer, rendered.bitmap.pitch);

        boost::shared_ptr<Glyph> glyph = createGlyph();
        glyph->charcode_ = charcodes[i];
        glyph->width_ = w;
        glyph->height_ = h;
        glyph->outline_ = outline_;
        glyph->outlineThickness_ = outlineThickness_;
        glyph->offset_.x = rendered.left;
        glyph->offset_.y = rendered.top;
        size_t width = atlas_->width();
        size_t height = atlas_->height();
        glyph->st_[0].s = x / static_cast<float>(width);
        glyph->st_[0].t = y / static_cast<float>(height);
        glyph->st_[1].s = (x + glyph->width_) / static_cast<float>(width);
        glyph->st_[1].t = (y + glyph->height_) / static_cast<float>(height);

        // Discard hinting to get advance
        FT_Load_Glyph(freetype_->face_, glyphIndex, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
        FT_GlyphSlot advanced = freetype_->face_->glyph;
        glyph->advance_.x = advanced->advance.x / 64.0f;
        glyph->advance_.y = advanced->advance.y / 64.0f;

        glyphs_.push_back(glyph);
    }

    generateKerning();
    freetype_->release();

    return true;
}

void TextureFont::generateKerning() {
    // For each glyph couple combination, check if kerning is necessary
    // Starts at index 1 since 0 is for the special backgroudn glyph
    for (unsigned int i = 1; i < glyphs_.size(); ++i) {
        boost::shared_ptr<Glyph> glyph = glyphs_[i];
        FT_UInt glyphIndex = FT_Get_Char_Index(freetype_->face_, glyph->charcode_);
        glyph->kerning_.clear();

        for (unsigned int j = 1; j < glyphs_.size(); ++j) {
            boost::shared_ptr<Glyph> prevGlyph = glyphs_[j];
            FT_UInt prevIndex = FT_Get_Char_Index(freetype_->face_, prevGlyph->charcode_);
            FT_Vector kerning;
            FT_Get_Kerning(freetype_->face_, prevIndex, glyphIndex, FT_KERNING_UNFITTED, &kerning);
            if (kerning.x) {
                // 64 * 64 because of 26.6 encoding AND the transform matrix used
                // in loadFace (hres = 64)
                Kerning k = { prevGlyph->charcode_, kerning.x / (64.0f * 64.0f) };
                glyph->kerning_.push_back(k);
            }
        }
    }
}

float TextureFont::kerning(boost::shared_ptr<Glyph> glyph, wchar_t charcode) {
    for (unsigned int i = 0; i < glyph->kerning_.size(); ++i) {
        if (glyph->kerning_[i].charcode_ == charcode) {
            return glyph->kerning_[i].kerning_;
        }
    }
    return 0.0f;
}

float TextureFont::size() const {
    return size_;
}

std::string TextureFont::filename() const {
    return filename_;
}

float TextureFont::ascender() const {
    return ascender_;
}

float TextureFont::descender() const {
    return descender_;
}

float TextureFont::height() const {
    return height_;
}

float TextureFont::linegap() const {
    return linegap_;
}

float TextureFont::underlinePosition() const {
    return underlinePosition_;
}

float TextureFont::underlineThickness() const {
    return underlineThickness_;
}

};  // namespace v3d::font
