/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureFont.h"

#include <api/font/internal/Freetype.h>
#include <api/font/internal/GlyphBitmap.h>
#include <api/font/internal/GlyphHandle.h>
#include <api/font/internal/StrokerHandle.h>
#include <api/image/TextureAtlas.h>
#include <api/log/Logger.h>

#include "Utf8.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_LCD_FILTER_H
#include FT_MODULE_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

#include <boost/make_shared.hpp>

namespace v3d::font {

namespace {

/**
 * The FreeType render mode for a run of glyphs.
 *
 * A three channel atlas holds subpixel (LCD) coverage. Any other atlas holds a distance field
 * when sdf is set, and coverage otherwise.
 **/
FT_Render_Mode glyphRenderMode(unsigned int depth, bool sdf) {
    if (depth == 3) {
        return FT_RENDER_MODE_LCD;
    }
    return sdf ? FT_RENDER_MODE_SDF : FT_RENDER_MODE_NORMAL;
}

/**
 * The FT_LOAD flags for one glyph.
 *
 * For a three channel atlas this also sets the library's LCD filter, because the filter only
 * has an effect with FT_LOAD_TARGET_LCD.
 **/
FT_Int32 glyphLoadFlags(FT_Library library, TextureFont::OutlineType outline, bool hinting,
    unsigned int depth, bool lcdFiltering, const unsigned char* lcdWeights, bool sdf) {
    FT_Int32 flags = 0;
    // a distance field and a stroked outline are both built from the glyph's outline, so the
    // load must not render a bitmap. The caller renders afterwards, in the mode it needs
    if (outline != TextureFont::OUTLINE_TYPE_NONE || sdf) {
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
 * *glyph receives FT_Get_Glyph's copy, which the caller's GlyphHandle owns from then on.
 * FT_Glyph_Stroke and FT_Glyph_To_Bitmap are both told to destroy the glyph they replace, and
 * do so only when they succeed. After a failure the caller releases the face, because the
 * caller loaded it.
 **/
bool strokeGlyph(FT_Library library, FT_Face face, TextureFont::OutlineType outline, float thickness,
    FT_Render_Mode mode, FT_Glyph* glyph, internal::GlyphBitmap* out,
    const boost::shared_ptr<v3d::log::Logger>& logger) {
    FT_Stroker raw;
    FT_Error error = FT_Stroker_New(library, &raw);
    if (error != 0) {
        logger->get()->error("Error creating stroker!");
        return false;
    }
    const internal::StrokerHandle stroker(raw);
    FT_Stroker_Set(
        stroker.get(),
        static_cast<int>(thickness * internal::Freetype::FIXED_POINT_SCALE),
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
    if (error != 0) {
        logger->get()->error("Error setting glyph stroke border!");
        return false;
    }

    error = FT_Glyph_To_Bitmap(glyph, mode, 0, 1);
    if (error != 0) {
        logger->get()->error("Error converting glyph to bitmap!");
        return false;
    }

    FT_BitmapGlyph bitmapGlyph = (FT_BitmapGlyph)*glyph;
    out->bitmap = bitmapGlyph->bitmap;
    out->width = static_cast<int>(bitmapGlyph->bitmap.width);
    out->rows = static_cast<int>(bitmapGlyph->bitmap.rows);
    out->top = bitmapGlyph->top;
    out->left = bitmapGlyph->left;
    return true;
}

};  // namespace

TextureFont::TextureFont(const std::string& filename, float size, const boost::shared_ptr<v3d::log::Logger> & logger,
    unsigned int spread) :
    atlas_(nullptr),
    filename_(filename),
    size_(size),
    spread_(spread),
    hinting_(1),
    outline_(OUTLINE_TYPE_NONE),
    outlineThickness_(0.0F),
    lcdFiltering_(1),
    height_(0),
    ascender_(0),
    descender_(0),
    logger_(logger) {
    constexpr std::array<unsigned char, LCD_FILTER_TAPS> defaultLcdWeights{ 0x10, 0x40, 0x70, 0x40, 0x10 };
    std::copy(defaultLcdWeights.begin(), defaultLcdWeights.end(), std::begin(lcdWeights_));

    // what the face would have set, for a face that does not open
    underlinePosition_ = 0.0F;
    underlineThickness_ = 0.0F;
    linegap_ = 0.0F;

    freetype_ = boost::make_shared<internal::Freetype>(logger);

    // load the face at its own size. A hundred times the size, for more precise metrics,
    // overflows what FT_Set_Char_Size accepts at the 64x horizontal resolution, and every
    // metric then comes back zero.
    if (!freetype_->loadFace(filename_, size_)) {
        return;
    }

    // the face's units are 26.6 fixed point and scaled by the horizontal oversample of the
    // transform matrix loadFace sets
    underlinePosition_ = static_cast<float>(freetype_->face_->underline_position) /
        (internal::Freetype::FIXED_POINT_SCALE * static_cast<float>(internal::Freetype::HORIZONTAL_OVERSAMPLE)) * size_;
    underlinePosition_ = underlinePosition_ < 0.0F ? std::ceil(underlinePosition_ - 0.5F) : std::floor(underlinePosition_ + 0.5F);
    underlinePosition_ = std::min(underlinePosition_, -2.0F);

    underlineThickness_ = static_cast<float>(freetype_->face_->underline_thickness) /
        (internal::Freetype::FIXED_POINT_SCALE * static_cast<float>(internal::Freetype::HORIZONTAL_OVERSAMPLE)) * size_;
    underlineThickness_ = underlineThickness_ < 0.0F ? std::ceil(underlineThickness_ - 0.5F) : std::floor(underlineThickness_ + 0.5F);
    underlineThickness_ = std::max(underlineThickness_, 1.0F);

    // metrics are 26.6 fixed point, which already carries fractional pixels
    const FT_Size_Metrics metrics = freetype_->face_->size->metrics;
    ascender_ = static_cast<float>(metrics.ascender) / internal::Freetype::FIXED_POINT_SCALE;
    descender_ = static_cast<float>(metrics.descender) / internal::Freetype::FIXED_POINT_SCALE;
    height_ = static_cast<float>(metrics.height) / internal::Freetype::FIXED_POINT_SCALE;
    linegap_ = height_ - ascender_ + descender_;

    freetype_->release();
}

/**
 **/
void TextureFont::atlas(const boost::shared_ptr<v3d::image::TextureAtlas>& atlas) {
    atlas_ = atlas;
}

void TextureFont::fallback(const boost::shared_ptr<TextureFont>& font) {
    fallback_ = font;
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
    glyph->advance_.x = 0.0F;
    glyph->advance_.y = 0.0F;
    glyph->st_[0].s = 0.0F;
    glyph->st_[0].t = 0.0F;
    glyph->st_[1].s = 0.0F;
    glyph->st_[1].t = 0.0F;

    return glyph;
}

boost::shared_ptr<TextureFont::Glyph> TextureFont::packed(char32_t charcode) const {
    boost::shared_ptr<Glyph> found = own(charcode);
    if (found || !fallback_ || charcode == SOLID) {
        return found;
    }
    return fallback_->packed(charcode);
}

boost::shared_ptr<TextureFont::Glyph> TextureFont::own(char32_t charcode) const {
    for (const boost::shared_ptr<Glyph>& candidate : glyphs_) {
        if ((candidate->charcode_ == charcode) &&
            ((charcode == SOLID) ||
                ((candidate->outline_ == outline_) &&
                    (candidate->outlineThickness_ == outlineThickness_)))) {
            return candidate;
        }
    }
    return boost::shared_ptr<Glyph>();
}

boost::shared_ptr<TextureFont::Glyph> TextureFont::glyph(char32_t charcode) {
    boost::shared_ptr<Glyph> glyph = packed(charcode);
    if (glyph) {
        return glyph;
    }
    // the white square is drawn for lines (overline, underline, strikethrough) and backgrounds
    if (charcode == SOLID) {
        const glm::ivec4 region = atlas_->region(4, 4);

        if (region.x < 0) {
            logger_->get()->error("Texture atlas is full!");
            return glyph;
        }
        glyph = createGlyph();
        // opaque white, so a line takes the colour it is drawn with
        static constexpr unsigned char data[4 * 4 * 3] = {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
        };

        atlas_->region(glm::ivec4(region.x, region.y, 4, 4), data, 0);

        const float width = static_cast<float>(atlas_->width());
        const float height = static_cast<float>(atlas_->height());

        glyph->charcode_ = SOLID;

        glyph->st_[0].s = static_cast<float>(region.x + 2) / width;
        glyph->st_[0].t = static_cast<float>(region.y + 2) / height;
        glyph->st_[1].s = static_cast<float>(region.x + 3) / width;
        glyph->st_[1].t = static_cast<float>(region.y + 3) / height;

        glyphs_.push_back(glyph);

        return glyph;
    }

    if (pack(std::u32string(1, charcode))) {
        return glyphs_[glyphs_.size() - 1];
    }

    return glyph;
}

bool TextureFont::loadGlyphs(std::string_view utf8) {
    return pack(decode(utf8));
}

float TextureFont::width(std::string_view utf8, float size) const {
    float width = 0.0F;
    std::size_t index = 0;
    while (index < utf8.size()) {
        const boost::shared_ptr<Glyph> found = packed(next(utf8, &index));
        if (found) {
            width += found->advance_.x;
        }
    }
    // the metrics are in pixels of the size the face was rasterized at
    return size > 0.0F && size_ > 0.0F ? width * (size / size_) : width;
}

void TextureFont::divide(const std::u32string& charcodes, std::u32string* mine, std::u32string* lacking) const {
    for (const char32_t charcode : charcodes) {
        // a code point packed by an earlier call, or by another face sharing this fallback,
        // is not packed twice
        if (own(charcode)) {
            continue;
        }
        // index zero is the face's missing glyph, so the code point goes to the fallback
        if (fallback_ && FT_Get_Char_Index(freetype_->face_, charcode) == 0) {
            lacking->push_back(charcode);
        } else {
            mine->push_back(charcode);
        }
    }
}

bool TextureFont::pack(const std::u32string& charcodes) {
    if (!atlas_) {
        return false;
    }

    if (!freetype_->loadFace(filename_, size_)) {
        return false;
    }

    // a distance field is only meaningful in the one channel case - the three channel one
    // is subpixel coverage, where each channel is a different sample of the same edge
    const bool sdf = spread_ > 0 && atlas_->depth() == 1;
    const FT_Render_Mode mode = glyphRenderMode(atlas_->depth(), sdf);
    if (sdf) {
        // FreeType clamps the spread to 2..32. The spread is in pixels at the size the face
        // was loaded at, so the base size and the spread are chosen together
        FT_Int property = static_cast<FT_Int>(spread_);
        FT_Property_Set(freetype_->library_, "sdf", "spread", &property);
    }

    std::u32string mine;
    std::u32string lacking;
    divide(charcodes, &mine, &lacking);

    unsigned int missed = 0;
    for (std::size_t i = 0; i < mine.size(); ++i) {
        const FT_UInt glyphIndex = FT_Get_Char_Index(freetype_->face_, mine[i]);
        const FT_Int32 flags = glyphLoadFlags(freetype_->library_, outline_, hinting_ != 0,
            atlas_->depth(), lcdFiltering_ != 0, lcdWeights_, sdf);

        FT_Error error = FT_Load_Glyph(freetype_->face_, glyphIndex, flags);
        if (error != 0) {
            logger_->get()->error("Error loading glyph!");
            freetype_->release();
            return false;
        }

        // declared per iteration, so its destructor releases FT_Get_Glyph's copy whether the
        // iteration finishes, skips a glyph for a full atlas, or returns on a failed stroke
        internal::GlyphHandle stroked;
        internal::GlyphBitmap rendered;
        if (outline_ == OUTLINE_TYPE_NONE) {
            FT_GlyphSlot loaded = freetype_->face_->glyph;
            // the load left an outline rather than a bitmap when a distance field was
            // asked for, so this is where it becomes one
            if (sdf) {
                error = FT_Render_Glyph(loaded, mode);
                if (error != 0) {
                    logger_->get()->error("Error rendering glyph to a distance field!");
                    freetype_->release();
                    return false;
                }
            }
            rendered.bitmap = loaded->bitmap;
            rendered.width = static_cast<int>(loaded->bitmap.width);
            rendered.rows = static_cast<int>(loaded->bitmap.rows);
            rendered.top = loaded->bitmap_top;
            rendered.left = loaded->bitmap_left;
        } else if (!strokeGlyph(freetype_->library_, freetype_->face_, outline_, outlineThickness_,
                mode, stroked.address(), &rendered, logger_)) {
            freetype_->release();
            return false;
        }

        const unsigned int w = rendered.width / atlas_->depth();
        const unsigned int rows = rendered.rows;
        const glm::ivec4 region = atlas_->region(w, rows);
        if (region.x < 0) {
            missed++;
            continue;
        }
        const unsigned int x = region.x;
        const unsigned int y = region.y;
        atlas_->region(glm::ivec4(x, y, w, rows), rendered.bitmap.buffer, rendered.bitmap.pitch);

        const boost::shared_ptr<Glyph> glyph = createGlyph();
        glyph->charcode_ = mine[i];
        glyph->width_ = w;
        glyph->height_ = rows;
        glyph->outline_ = outline_;
        glyph->outlineThickness_ = outlineThickness_;
        glyph->offset_.x = rendered.left;
        glyph->offset_.y = rendered.top;
        const size_t width = atlas_->width();
        const size_t height = atlas_->height();
        glyph->st_[0].s = static_cast<float>(x) / static_cast<float>(width);
        glyph->st_[0].t = static_cast<float>(y) / static_cast<float>(height);
        glyph->st_[1].s = static_cast<float>(x + glyph->width_) / static_cast<float>(width);
        glyph->st_[1].t = static_cast<float>(y + glyph->height_) / static_cast<float>(height);

        // Discard hinting to get advance
        FT_Load_Glyph(freetype_->face_, glyphIndex, FT_LOAD_RENDER | FT_LOAD_NO_HINTING);
        FT_GlyphSlot advanced = freetype_->face_->glyph;
        glyph->advance_.x = static_cast<float>(advanced->advance.x) / internal::Freetype::FIXED_POINT_SCALE;
        glyph->advance_.y = static_cast<float>(advanced->advance.y) / internal::Freetype::FIXED_POINT_SCALE;

        glyphs_.push_back(glyph);
    }

    freetype_->release();

    if (missed > 0) {
        logger_->get()->error("{} of {} glyphs did not fit the {}x{} atlas at size {}",
            missed, mine.size(), atlas_->width(), atlas_->height(), size_);
        return false;
    }

    return lacking.empty() || fallback_->pack(lacking);
}

float TextureFont::size() const {
    return size_;
}

unsigned int TextureFont::spread() const {
    return spread_;
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
