/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Face.h"

#include <api/font/internal/FaceHandle.h>
#include <api/font/internal/LibraryHandle.h>
#include <api/log/Logger.h>
#include <hb-ot.h>
#include <hb.h>

#include "ColourGlyph.h"
#include "FaceMetrics.h"
#include "FaceStyle.h"
#include "Library.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

#include <memory>
#include <string>

#include <boost/make_shared.hpp>

namespace v3d::font {

namespace {

/**
 * The weight of a bold face, as the OS/2 weight class names it.
 **/
constexpr unsigned int BOLD = 700;

};  // namespace

boost::shared_ptr<Face> Face::open(const boost::shared_ptr<Library>& library, const std::string& filename,
    const boost::shared_ptr<v3d::log::Logger>& logger) {
    if (!library || !library->valid()) {
        logger->get()->error("{} could not be opened, because FreeType was not initialized", filename);
        return boost::shared_ptr<Face>();
    }
    boost::shared_ptr<Face> made(new Face());
    made->library_ = library;
    made->handle_ = boost::make_shared<internal::FaceHandle>(library->handle().library, filename.c_str());
    FT_Face face = made->handle_->face;
    if (face == nullptr) {
        logger->get()->error("{} could not be opened as a font with a Unicode character map", filename);
        return boost::shared_ptr<Face>();
    }
    made->filename_ = filename;
    made->family_ = face->family_name != nullptr ? face->family_name : "";
    made->style_ = face->style_name != nullptr ? face->style_name : "";
    made->italic_ = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
    const TT_OS2* os2 = static_cast<const TT_OS2*>(FT_Get_Sfnt_Table(face, FT_SFNT_OS2));
    if (os2 != nullptr && os2->usWeightClass > 0) {
        made->weight_ = os2->usWeightClass;
    } else {
        made->weight_ = (face->style_flags & FT_STYLE_FLAG_BOLD) != 0 ? BOLD : FaceStyle::REGULAR;
    }
    return made;
}

const std::string& Face::filename() const noexcept {
    return filename_;
}

const std::string& Face::family() const noexcept {
    return family_;
}

const std::string& Face::style() const noexcept {
    return style_;
}

unsigned int Face::weight() const noexcept {
    return weight_;
}

bool Face::italic() const noexcept {
    return italic_;
}

unsigned int Face::glyphIndex(char32_t charcode) const {
    return FT_Get_Char_Index(handle_->face, charcode);
}

bool Face::has(char32_t charcode) const {
    return glyphIndex(charcode) != 0;
}

FaceMetrics Face::metrics(float size) const {
    const std::unique_ptr<hb_font_t, decltype(&hb_font_destroy)> font(hb_font_create(handle_->shaping), &hb_font_destroy);
    // measured in design units and scaled here, so a small size loses no precision to rounding
    const auto units = static_cast<int>(hb_face_get_upem(handle_->shaping));
    hb_font_set_scale(font.get(), units, units);
    const float scale = units > 0 ? size / static_cast<float>(units) : 0.0F;
    const auto measure = [&font, scale](hb_ot_metrics_tag_t tag) {
        hb_position_t position = 0;
        hb_ot_metrics_get_position_with_fallback(font.get(), tag, &position);
        return static_cast<float>(position) * scale;
    };
    // OpenType's y grows upwards, so a descender and an underline offset are negative there
    return FaceMetrics{ .ascender = measure(HB_OT_METRICS_TAG_HORIZONTAL_ASCENDER),
        .descender = -measure(HB_OT_METRICS_TAG_HORIZONTAL_DESCENDER),
        .lineGap = measure(HB_OT_METRICS_TAG_HORIZONTAL_LINE_GAP),
        .underlinePosition = -measure(HB_OT_METRICS_TAG_UNDERLINE_OFFSET),
        .underlineThickness = measure(HB_OT_METRICS_TAG_UNDERLINE_SIZE),
        .strikeoutPosition = measure(HB_OT_METRICS_TAG_STRIKEOUT_OFFSET),
        .strikeoutThickness = measure(HB_OT_METRICS_TAG_STRIKEOUT_SIZE) };
}

ColourTables Face::colour() const {
    hb_face_t* face = handle_->shaping;
    const auto holds = [face](hb_tag_t tag) {
        hb_blob_t* table = hb_face_reference_table(face, tag);
        const bool held = hb_blob_get_length(table) > 0;
        hb_blob_destroy(table);
        return held;
    };
    return ColourTables{ .layers = hb_ot_color_has_layers(face) != 0,
        .paint = hb_ot_color_has_paint(face) != 0,
        .svg = hb_ot_color_has_svg(face) != 0,
        .cbdt = holds(HB_TAG('C', 'B', 'D', 'T')),
        .sbix = holds(HB_TAG('s', 'b', 'i', 'x')) };
}

bool Face::coloured(unsigned int index) const {
    hb_face_t* face = handle_->shaping;
    if (face == nullptr) {
        return false;
    }
    if (hb_ot_color_glyph_has_paint(face, index) != 0 || hb_ot_color_glyph_get_layers(face, index, 0, nullptr, nullptr) > 0) {
        return true;
    }
    const auto holds = [](hb_blob_t* blob) {
        const bool held = hb_blob_get_length(blob) > 0;
        hb_blob_destroy(blob);
        return held;
    };
    if (holds(hb_ot_color_glyph_reference_svg(face, index))) {
        return true;
    }
    if (hb_ot_color_has_png(face) == 0) {
        return false;
    }
    const std::unique_ptr<hb_font_t, decltype(&hb_font_destroy)> font(hb_font_create(face), &hb_font_destroy);
    return holds(hb_ot_color_glyph_reference_png(font.get(), index));
}

const boost::shared_ptr<Library>& Face::library() const noexcept {
    return library_;
}

internal::FaceHandle& Face::handle() const {
    return *handle_;
}

};  // namespace v3d::font
