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

#include <api/font/glyph/ColourGlyph.h>
#include "Metrics.h"
#include "Style.h"
#include "Library.h"
#include "Variation.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::font::face {

namespace {

/**
 * The weight of a bold face, as the OS/2 weight class names it.
 **/
constexpr unsigned int BOLD = 700;

/**
 * The heaviest weight OpenType's weight axis reaches.
 **/
constexpr float HEAVIEST = 1000.0F;

/**
 * The letters in an axis's tag.
 **/
constexpr std::size_t TAG_LENGTH = 4;

constexpr hb_tag_t WEIGHT_AXIS = HB_TAG('w', 'g', 'h', 't');
constexpr hb_tag_t OPTICAL_SIZE_AXIS = HB_TAG('o', 'p', 's', 'z');

/**
 * @return a variable font's axes, read through HarfBuzz
 **/
std::vector<Axis> axesOf(hb_face_t* face) {
    std::vector<hb_ot_var_axis_info_t> infos(hb_ot_var_get_axis_count(face));
    auto count = static_cast<unsigned int>(infos.size());
    hb_ot_var_get_axis_infos(face, 0, &count, infos.data());
    std::vector<Axis> axes;
    axes.reserve(count);
    for (unsigned int axis = 0; axis < count; axis++) {
        std::array<char, TAG_LENGTH> tag{};
        hb_tag_to_string(infos[axis].tag, tag.data());
        axes.push_back(Axis{ .tag = std::string(tag.data(), tag.size()), .min = infos[axis].min_value, .initial = infos[axis].default_value,
            .max = infos[axis].max_value });
    }
    return axes;
}

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
        made->weight_ = (face->style_flags & FT_STYLE_FLAG_BOLD) != 0 ? BOLD : Style::REGULAR;
    }
    made->weights_ = { made->weight_, made->weight_ };
    made->axes_ = axesOf(made->handle_->shaping);
    for (const Axis& axis : made->axes_) {
        // a weight axis runs from 1 to 1000, and a face whose axis does not is chosen by its own
        // weight alone
        if (axis.tag == "wght" && axis.min >= 1.0F && axis.max >= axis.min && axis.max <= HEAVIEST) {
            const auto lightest = static_cast<unsigned int>(std::lround(axis.min));  // checked: from 1 to HEAVIEST
            const auto heaviest = static_cast<unsigned int>(std::lround(axis.max));  // checked: from 1 to HEAVIEST
            made->weights_ = { lightest, heaviest };
        }
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

std::pair<unsigned int, unsigned int> Face::weights() const noexcept {
    return weights_;
}

const std::vector<Axis>& Face::axes() const noexcept {
    return axes_;
}

std::vector<std::int32_t> Face::coordinates(const Style& style, float size, const std::vector<Variation>& variations) const {
    if (axes_.empty()) {
        return {};
    }
    // set in order, so a variation given for the weight or the size is set over the style's
    std::vector<hb_variation_t> settings{ hb_variation_t{ .tag = WEIGHT_AXIS, .value = static_cast<float>(style.weight) } };
    if (std::isfinite(size)) {
        settings.push_back(hb_variation_t{ .tag = OPTICAL_SIZE_AXIS, .value = size });
    }
    for (const Variation& variation : variations) {
        if (variation.tag.size() == TAG_LENGTH && std::isfinite(variation.value)) {
            settings.push_back(hb_variation_t{ .tag = hb_tag_from_string(variation.tag.data(), static_cast<int>(TAG_LENGTH)), .value = variation.value });
        }
    }
    const std::unique_ptr<hb_font_t, decltype(&hb_font_destroy)> font(hb_font_create(handle_->shaping), &hb_font_destroy);
    hb_font_set_variations(font.get(), settings.data(), static_cast<unsigned int>(settings.size()));
    unsigned int count = 0;
    const int* normalized = hb_font_get_var_coords_normalized(font.get(), &count);
    const auto atDefault = [](int value) { return value == 0; };
    if (normalized == nullptr || std::all_of(normalized, normalized + count, atDefault)) {
        return {};
    }
    return std::vector<std::int32_t>(normalized, normalized + count);
}

unsigned int Face::glyphIndex(char32_t charcode) const {
    return FT_Get_Char_Index(handle_->face, charcode);
}

bool Face::has(char32_t charcode) const {
    return glyphIndex(charcode) != 0;
}

Metrics Face::metrics(float size, const std::vector<std::int32_t>& coordinates) const {
    const std::unique_ptr<hb_font_t, decltype(&hb_font_destroy)> font(hb_font_create(handle_->shaping), &hb_font_destroy);
    if (!coordinates.empty()) {
        hb_font_set_var_coords_normalized(font.get(), coordinates.data(), static_cast<unsigned int>(coordinates.size()));
    }
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
    return Metrics{ .ascender = measure(HB_OT_METRICS_TAG_HORIZONTAL_ASCENDER),
        .descender = -measure(HB_OT_METRICS_TAG_HORIZONTAL_DESCENDER),
        .lineGap = measure(HB_OT_METRICS_TAG_HORIZONTAL_LINE_GAP),
        .underlinePosition = -measure(HB_OT_METRICS_TAG_UNDERLINE_OFFSET),
        .underlineThickness = measure(HB_OT_METRICS_TAG_UNDERLINE_SIZE),
        .strikeoutPosition = measure(HB_OT_METRICS_TAG_STRIKEOUT_OFFSET),
        .strikeoutThickness = measure(HB_OT_METRICS_TAG_STRIKEOUT_SIZE) };
}

glyph::ColourTables Face::colour() const {
    hb_face_t* face = handle_->shaping;
    const auto holds = [face](hb_tag_t tag) {
        hb_blob_t* table = hb_face_reference_table(face, tag);
        const bool held = hb_blob_get_length(table) > 0;
        hb_blob_destroy(table);
        return held;
    };
    return glyph::ColourTables{ .layers = hb_ot_color_has_layers(face) != 0,
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

};  // namespace v3d::font::face
