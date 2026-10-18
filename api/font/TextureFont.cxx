/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureFont.h"

#include <api/image/TextureAtlas.h>
#include <api/log/Logger.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_LCD_FILTER_H
#include FT_MODULE_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cwchar>
#include <iterator>
#include <string>

#include <boost/make_shared.hpp>

namespace v3d::font {

namespace {

// metrics and advances are 26.6 fixed point, 64 units to the pixel
constexpr float FIXED_POINT_SCALE = 64.0F;
// the face is loaded at this many times its horizontal resolution, and the transform matrix
// scales x back down by the same factor
constexpr FT_UInt HORIZONTAL_OVERSAMPLE = 64;
// the resolution FT_Set_Char_Size is given, in dots per inch, at which one point is one pixel
constexpr FT_UInt RESOLUTION_DPI = 72;

};  // namespace

/**
 * The FreeType library and face a font is read through, open only while it is being read.
 * release() closes whatever is open, and so do loadFace() and the destructor. A load that
 * fails part way therefore never has to track which of the two it opened.
 **/
class TextureFont::Freetype {
 public:
    explicit Freetype(const boost::shared_ptr<v3d::log::Logger> & logger);
    ~Freetype();

    Freetype(const Freetype&) = delete;
    Freetype& operator=(const Freetype&) = delete;

    bool loadFace(const std::string& filename, float size);

    void release();

    FT_Library library_ = nullptr;
    FT_Face face_ = nullptr;
    boost::shared_ptr<v3d::log::Logger> logger_;
};

TextureFont::Freetype::Freetype(const boost::shared_ptr<v3d::log::Logger>& logger) : logger_(logger) {
}

TextureFont::Freetype::~Freetype() {
    release();
}

void TextureFont::Freetype::release() {
    if (face_ != nullptr) {
        FT_Done_Face(face_);
        face_ = nullptr;
    }
    if (library_ != nullptr) {
        FT_Done_FreeType(library_);
        library_ = nullptr;
    }
}

bool TextureFont::Freetype::loadFace(const std::string& filename, float size) {
    release();

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
        release();
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
    error = FT_Set_Char_Size(face_, static_cast<int>((size * FIXED_POINT_SCALE)), 0, RESOLUTION_DPI * HORIZONTAL_OVERSAMPLE, RESOLUTION_DPI);
    if (error != 0) {
        logger_->get()->error("Error setting freetype char size!");
        release();
        return false;
    }

    // set transform matrix
    FT_Matrix matrix = {
        static_cast<int>((1.0 / HORIZONTAL_OVERSAMPLE) * 0x10000L),
        static_cast<int>(0.0 * 0x10000L),
        static_cast<int>(0.0 * 0x10000L),
        static_cast<int>(1.0 * 0x10000L)
    };
    FT_Set_Transform(face_, &matrix, NULL);

    return true;
}

TextureFont::TextureFont(const std::string& filename, float size, const boost::shared_ptr<v3d::log::Logger> & logger,
    unsigned int spread) :
    atlas_(nullptr),
    filename_(filename),
    size_(size),
    spread_(spread),
    height_(0),
    ascender_(0),
    descender_(0),
    outline_(OUTLINE_TYPE_NONE),
    outlineThickness_(0.0F),
    hinting_(1),
    lcdFiltering_(1),
    logger_(logger) {
    constexpr std::array<unsigned char, LCD_FILTER_TAPS> defaultLcdWeights{ 0x10, 0x40, 0x70, 0x40, 0x10 };
    std::copy(defaultLcdWeights.begin(), defaultLcdWeights.end(), std::begin(lcdWeights_));

    // what the face would have set, for a face that does not open
    underlinePosition_ = 0.0F;
    underlineThickness_ = 0.0F;
    linegap_ = 0.0F;

    freetype_ = boost::make_shared<Freetype>(logger);

    // load the face at its own size. A hundred times the size, for more precise metrics,
    // overflows what FT_Set_Char_Size accepts at the 64x horizontal resolution, and every
    // metric then comes back zero.
    if (!freetype_->loadFace(filename_, size_)) {
        return;
    }

    // the face's units are 26.6 fixed point and scaled by the horizontal oversample of the
    // transform matrix loadFace sets
    underlinePosition_ = static_cast<float>(freetype_->face_->underline_position) / (FIXED_POINT_SCALE * static_cast<float>(HORIZONTAL_OVERSAMPLE)) * size_;
    underlinePosition_ = underlinePosition_ < 0.0F ? std::ceil(underlinePosition_ - 0.5F) : std::floor(underlinePosition_ + 0.5F);
    underlinePosition_ = std::min(underlinePosition_, -2.0F);

    underlineThickness_ = static_cast<float>(freetype_->face_->underline_thickness) / (FIXED_POINT_SCALE * static_cast<float>(HORIZONTAL_OVERSAMPLE)) * size_;
    underlineThickness_ = underlineThickness_ < 0.0F ? std::ceil(underlineThickness_ - 0.5F) : std::floor(underlineThickness_ + 0.5F);
    underlineThickness_ = std::max(underlineThickness_, 1.0F);

    // metrics are 26.6 fixed point, which already carries fractional pixels
    const FT_Size_Metrics metrics = freetype_->face_->size->metrics;
    ascender_ = static_cast<float>(metrics.ascender) / FIXED_POINT_SCALE;
    descender_ = static_cast<float>(metrics.descender) / FIXED_POINT_SCALE;
    height_ = static_cast<float>(metrics.height) / FIXED_POINT_SCALE;
    linegap_ = height_ - ascender_ + descender_;

    freetype_->release();
}

/**
 **/
void TextureFont::atlas(const boost::shared_ptr<v3d::image::TextureAtlas>& atlas) {
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
    glyph->advance_.x = 0.0F;
    glyph->advance_.y = 0.0F;
    glyph->st_[0].s = 0.0F;
    glyph->st_[0].t = 0.0F;
    glyph->st_[1].s = 0.0F;
    glyph->st_[1].t = 0.0F;

    return glyph;
}

boost::shared_ptr<TextureFont::Glyph> TextureFont::packed(wchar_t charcode) const {
    const wchar_t lineCode = static_cast<wchar_t>(-1);

    for (const boost::shared_ptr<Glyph>& candidate : glyphs_) {
        if ((candidate->charcode_ == charcode) &&
            ((charcode == lineCode) ||
                ((candidate->outline_ == outline_) &&
                    (candidate->outlineThickness_ == outlineThickness_)))) {
            return candidate;
        }
    }
    return boost::shared_ptr<Glyph>();
}

boost::shared_ptr<TextureFont::Glyph> TextureFont::glyph(wchar_t charcode) {
    const wchar_t lineCode = static_cast<wchar_t>(-1);

    boost::shared_ptr<Glyph> glyph = packed(charcode);
    if (glyph) {
        return glyph;
    }
    // -1 is used for line drawing (overline, underline, strikethrough) and background
    if (charcode == lineCode) {
        const glm::ivec4 region = atlas_->region(4, 4);

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

        atlas_->region(glm::ivec4(region.x, region.y, 4, 4), data, 0);

        const float width = static_cast<float>(atlas_->width());
        const float height = static_cast<float>(atlas_->height());

        glyph->charcode_ = lineCode;

        glyph->st_[0].s = static_cast<float>(region.x + 2) / width;
        glyph->st_[0].t = static_cast<float>(region.y + 2) / height;
        glyph->st_[1].s = static_cast<float>(region.x + 3) / width;
        glyph->st_[1].t = static_cast<float>(region.y + 3) / height;

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
 * Owns the stroker while the outline is built.
 *
 * Each step after FT_Stroker_New can fail and return, so the destructor releases the stroker
 * on every path.
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
 * Owns the glyph the outline path builds, until the end of the loop iteration that built it.
 *
 * It usually holds nothing: a glyph with no outline is read from the face's own slot, which
 * the face owns. Only FT_Get_Glyph returns a copy that must be released. The destructor
 * releases it however the iteration ends, whether a full atlas skips to the next glyph or a
 * failed stroke ends the load.
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
     * Where FT_Get_Glyph writes. A failure before that call leaves it null. After that call,
     * it holds the glyph from the last step that succeeded.
     **/
    FT_Glyph* address() {
        return &glyph_;
    }

 private:
    FT_Glyph glyph_ = nullptr;
};

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
    FT_Render_Mode mode, FT_Glyph* glyph, GlyphBitmap* out,
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
        static_cast<int>(thickness * FIXED_POINT_SCALE),
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

bool TextureFont::loadGlyphs(const wchar_t* charcodes) {
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

    unsigned int missed = 0;
    for (unsigned int i = 0; i < wcslen(charcodes); ++i) {
        const FT_UInt glyphIndex = FT_Get_Char_Index(freetype_->face_, charcodes[i]);
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
        GlyphHandle stroked;
        GlyphBitmap rendered;
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
        glyph->charcode_ = charcodes[i];
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
        glyph->advance_.x = static_cast<float>(advanced->advance.x) / FIXED_POINT_SCALE;
        glyph->advance_.y = static_cast<float>(advanced->advance.y) / FIXED_POINT_SCALE;

        glyphs_.push_back(glyph);
    }

    freetype_->release();

    if (missed > 0) {
        logger_->get()->error("{} of {} glyphs did not fit the {}x{} atlas at size {}",
            missed, wcslen(charcodes), atlas_->width(), atlas_->height(), size_);
        return false;
    }

    return true;
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
