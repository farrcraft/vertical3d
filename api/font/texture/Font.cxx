/*
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 */

#include "Font.h"

#include <api/font/internal/FaceHandle.h>
#include <api/font/internal/LibraryHandle.h>
#include <api/font/texture/internal/GlyphBitmap.h>
#include <api/font/texture/internal/GlyphHandle.h>
#include <api/font/internal/StrokerHandle.h>
#include <api/image/TextureAtlas.h>
#include <api/log/Logger.h>

#include <api/font/face/Face.h>
#include <api/font/face/Library.h>
#include <api/font/text/Utf8.h>

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

namespace v3d::font::texture {

namespace {

/** A linear advance is 16.16 fixed point, 65536 units to the pixel. **/
constexpr float LINEAR_SCALE = 65536.0F;

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
 * A distance field is built from the outline unhinted, because it is drawn at sizes other than
 * the one it is built at. A coverage glyph is hinted lightly, to whole pixels vertically only,
 * so it keeps its shape across. No load renders: the caller renders in the mode its atlas holds,
 * which for a three channel atlas is subpixel coverage whatever the hinting. For that atlas this
 * also sets the library's LCD filter, which the render applies.
 **/
FT_Int32 glyphLoadFlags(FT_Library library, Font::OutlineType outline,
    unsigned int depth, bool lcdFiltering, const unsigned char* lcdWeights, bool sdf) {
    FT_Int32 flags = sdf ? FT_LOAD_NO_HINTING : FT_LOAD_TARGET_LIGHT;
    // a distance field and a stroked outline are both built from the glyph's outline, so the
    // load must not take a bitmap the face carries instead
    if (outline != Font::OUTLINE_TYPE_NONE || sdf) {
        flags |= FT_LOAD_NO_BITMAP;
    }

    if (depth == 3) {
        FT_Library_SetLcdFilter(library, FT_LCD_FILTER_LIGHT);
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
 * do so only when they succeed.
 **/
bool strokeGlyph(FT_Library library, FT_Face face, Font::OutlineType outline, float thickness,
    FT_Render_Mode mode, FT_Glyph* glyph, internal::GlyphBitmap* out,
    const boost::shared_ptr<v3d::log::Logger>& logger) {
    FT_Stroker raw;
    FT_Error error = FT_Stroker_New(library, &raw);
    if (error != 0) {
        logger->get()->error("Error creating stroker!");
        return false;
    }
    const font::internal::StrokerHandle stroker(raw);
    FT_Stroker_Set(
        stroker.get(),
        static_cast<int>(thickness * font::internal::FaceHandle::FIXED_POINT_SCALE),
        FT_STROKER_LINECAP_ROUND,
        FT_STROKER_LINEJOIN_ROUND,
        0);
    error = FT_Get_Glyph(face->glyph, glyph);
    if (error != 0) {
        logger->get()->error("Error getting glyph!");
        return false;
    }

    if (outline == Font::OUTLINE_TYPE_LINE) {
        error = FT_Glyph_Stroke(glyph, stroker.get(), 1);
    } else if (outline == Font::OUTLINE_TYPE_INNER) {
        error = FT_Glyph_StrokeBorder(glyph, stroker.get(), 0, 1);
    } else if (outline == Font::OUTLINE_TYPE_OUTER) {
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

Font::Font(const std::string& filename, float size, const boost::shared_ptr<v3d::log::Logger> & logger,
    unsigned int spread) :
    Font(face::Face::open(boost::make_shared<face::Library>(), filename, logger), size, logger, spread) {
    filename_ = filename;
}

Font::Font(const boost::shared_ptr<face::Face>& face, float size, const boost::shared_ptr<v3d::log::Logger> & logger,
    unsigned int spread) :
    atlas_(nullptr),
    filename_(face ? face->filename() : std::string()),
    size_(size),
    spread_(spread),
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

    face_ = face;
    if (!face_ || !face_->handle().size(size_)) {
        return;
    }
    FT_Face loaded = face_->handle().face;

    // the underline is in the face's design units, of which there are units_per_EM to the size
    if (loaded->units_per_EM > 0) {
        const float units = size_ / static_cast<float>(loaded->units_per_EM);
        underlinePosition_ = std::min(std::round(static_cast<float>(loaded->underline_position) * units), -1.0F);
        underlineThickness_ = std::max(std::round(static_cast<float>(loaded->underline_thickness) * units), 1.0F);
    }

    // metrics are 26.6 fixed point, which already carries fractional pixels
    const FT_Size_Metrics metrics = loaded->size->metrics;
    ascender_ = static_cast<float>(metrics.ascender) / font::internal::FaceHandle::FIXED_POINT_SCALE;
    descender_ = static_cast<float>(metrics.descender) / font::internal::FaceHandle::FIXED_POINT_SCALE;
    height_ = static_cast<float>(metrics.height) / font::internal::FaceHandle::FIXED_POINT_SCALE;
    linegap_ = height_ - ascender_ + descender_;
}

/**
 **/
void Font::atlas(const boost::shared_ptr<v3d::image::TextureAtlas>& atlas) {
    atlas_ = atlas;
}

void Font::fallback(const boost::shared_ptr<Font>& font) {
    fallback_ = font;
}

boost::shared_ptr<Font::Glyph> Font::createGlyph() {
    boost::shared_ptr<Glyph> glyph(new Glyph);

    glyph->id_ = 0;
    glyph->index_ = 0;
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

boost::shared_ptr<Font::Glyph> Font::packed(char32_t charcode) const {
    boost::shared_ptr<Glyph> found = own(charcode);
    if (found || !fallback_ || charcode == SOLID) {
        return found;
    }
    return fallback_->packed(charcode);
}

boost::shared_ptr<Font::Glyph> Font::own(char32_t charcode) const {
    const auto found = byCode_.find(charcode);
    if (found == byCode_.end()) {
        return boost::shared_ptr<Glyph>();
    }
    const boost::shared_ptr<Glyph>& candidate = found->second;
    const bool outlined = candidate->outline_ == outline_ && candidate->outlineThickness_ == outlineThickness_;
    return charcode == SOLID || outlined ? candidate : boost::shared_ptr<Glyph>();
}

boost::shared_ptr<Font::Glyph> Font::glyph(char32_t charcode) {
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
        byCode_[SOLID] = glyph;

        return glyph;
    }

    if (pack(std::u32string(1, charcode))) {
        return packed(charcode);
    }

    return glyph;
}

bool Font::loadGlyphs(std::string_view utf8) {
    return pack(text::decode(utf8));
}

float Font::width(std::string_view utf8, float size) const {
    float width = 0.0F;
    std::size_t index = 0;
    while (index < utf8.size()) {
        const boost::shared_ptr<Glyph> found = packed(text::next(utf8, &index));
        if (found) {
            width += found->advance_.x;
        }
    }
    // the metrics are in pixels of the size the face was rasterized at
    return size > 0.0F && size_ > 0.0F ? width * (size / size_) : width;
}

void Font::divide(const std::u32string& charcodes, std::u32string* mine, std::u32string* lacking) const {
    for (const char32_t charcode : charcodes) {
        // a code point packed by an earlier call, or by another face sharing this fallback,
        // is not packed twice
        if (own(charcode)) {
            continue;
        }
        // a code point this face lacks goes down the chain, unless no font there has it either
        if (!face_->has(charcode) && fallback_ && fallback_->covers(charcode)) {
            lacking->push_back(charcode);
        } else {
            mine->push_back(charcode);
        }
    }
}

bool Font::covers(char32_t charcode) const {
    return (face_ && face_->has(charcode)) || (fallback_ && fallback_->covers(charcode));
}

bool Font::packGlyph(char32_t charcode, bool sdf, unsigned int* missed) {
    const FT_UInt glyphIndex = face_->glyphIndex(charcode);
    // another code point drawn with the same glyph, such as the missing glyph, packed it already
    if (const auto shared = byIndex_.find(glyphIndex); shared != byIndex_.end()) {
        byCode_[charcode] = shared->second;
        return true;
    }
    const FT_Render_Mode mode = glyphRenderMode(atlas_->depth(), sdf);
    FT_Face loaded = face_->handle().face;
    FT_Library library = face_->library()->handle().library;
    const FT_Int32 flags = glyphLoadFlags(library, outline_, atlas_->depth(), lcdFiltering_ != 0, lcdWeights_, sdf);

    // drawn at the face's default, whatever coordinates another load left it at
    face_->handle().vary({});
    FT_Error error = FT_Load_Glyph(loaded, glyphIndex, flags);
    if (error != 0) {
        logger_->get()->error("Error loading glyph!");
        return false;
    }
    // the outline's own advance, in 16.16 fixed point: light hinting rounds the slot's advance to
    // whole pixels, and this one keeps the fraction, as the shaped advances do
    const FT_Fixed advance = loaded->glyph->linearHoriAdvance;

    // its destructor releases FT_Get_Glyph's copy whether the glyph is packed, skipped for a
    // full atlas, or fails to stroke
    internal::GlyphHandle stroked;
    internal::GlyphBitmap rendered;
    if (outline_ == OUTLINE_TYPE_NONE) {
        FT_GlyphSlot slot = loaded->glyph;
        // the load left the outline, so this is where it becomes coverage or a distance field.
        // A bitmap the face carries is left as it is
        error = FT_Render_Glyph(slot, mode);
        if (error != 0) {
            logger_->get()->error("Error rendering glyph!");
            return false;
        }
        rendered.bitmap = slot->bitmap;
        rendered.width = static_cast<int>(slot->bitmap.width);
        rendered.rows = static_cast<int>(slot->bitmap.rows);
        rendered.top = slot->bitmap_top;
        rendered.left = slot->bitmap_left;
    } else if (!strokeGlyph(library, loaded, outline_, outlineThickness_,
            mode, stroked.address(), &rendered, logger_)) {
        return false;
    }

    const unsigned int w = rendered.width / atlas_->depth();
    const unsigned int rows = rendered.rows;
    const glm::ivec4 region = atlas_->region(w, rows);
    if (region.x < 0) {
        (*missed)++;
        return true;
    }
    const unsigned int x = region.x;
    const unsigned int y = region.y;
    atlas_->region(glm::ivec4(x, y, w, rows), rendered.bitmap.buffer, rendered.bitmap.pitch);

    const boost::shared_ptr<Glyph> glyph = createGlyph();
    glyph->charcode_ = charcode;
    glyph->index_ = glyphIndex;
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

    // text runs across, so a glyph advances along x alone
    glyph->advance_.x = static_cast<float>(advance) / LINEAR_SCALE;
    glyph->advance_.y = 0.0F;

    glyphs_.push_back(glyph);
    byCode_[charcode] = glyph;
    byIndex_[glyphIndex] = glyph;
    return true;
}

bool Font::pack(const std::u32string& charcodes) {
    if (!atlas_) {
        return false;
    }

    // another font may have sized the face since this one last did
    if (!face_ || !face_->handle().size(size_)) {
        return false;
    }

    // a distance field is only meaningful in the one channel case - the three channel one
    // is subpixel coverage, where each channel is a different sample of the same edge
    const bool sdf = spread_ > 0 && atlas_->depth() == 1;
    if (sdf) {
        // FreeType clamps the spread to 2..32. The spread is in pixels at the size the face
        // was loaded at, so the base size and the spread are chosen together
        FT_Int property = static_cast<FT_Int>(spread_);
        FT_Property_Set(face_->library()->handle().library, "sdf", "spread", &property);
    }

    std::u32string mine;
    std::u32string lacking;
    divide(charcodes, &mine, &lacking);

    unsigned int missed = 0;
    for (const char32_t charcode : mine) {
        if (!packGlyph(charcode, sdf, &missed)) {
            return false;
        }
    }

    if (missed > 0) {
        logger_->get()->error("{} of {} glyphs did not fit the {}x{} atlas at size {}",
            missed, mine.size(), atlas_->width(), atlas_->height(), size_);
        return false;
    }

    return lacking.empty() || fallback_->pack(lacking);
}

float Font::size() const {
    return size_;
}

unsigned int Font::spread() const {
    return spread_;
}

std::string Font::filename() const {
    return filename_;
}

const boost::shared_ptr<face::Face>& Font::face() const noexcept {
    return face_;
}

float Font::ascender() const {
    return ascender_;
}

float Font::descender() const {
    return descender_;
}

float Font::height() const {
    return height_;
}

float Font::linegap() const {
    return linegap_;
}

float Font::underlinePosition() const {
    return underlinePosition_;
}

float Font::underlineThickness() const {
    return underlineThickness_;
}

};  // namespace v3d::font::texture
