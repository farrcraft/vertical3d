/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shaper.h"

#include <api/font/internal/FaceHandle.h>
#include <hb.h>

#include <array>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Bidi.h"
#include "Face.h"
#include "FaceStyle.h"
#include "FallbackChain.h"
#include "Feature.h"
#include "ShapedGlyph.h"
#include "ShapedRun.h"
#include "Utf8.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::font {

namespace {

/**
 * HarfBuzz positions are in 26.6 fixed point, 64 units to the pixel, at the scale they are
 * given here.
 **/
constexpr float FIXED_POINT_SCALE = 64.0F;

/**
 * How many letters an OpenType tag has.
 **/
constexpr std::size_t TAG_LENGTH = 4;

/**
 * @return whether a character takes its script from the characters around it
 **/
bool neutral(hb_script_t script) {
    return script == HB_SCRIPT_COMMON || script == HB_SCRIPT_INHERITED || script == HB_SCRIPT_UNKNOWN;
}

/**
 * @return a script's ISO 15924 code
 **/
std::string named(hb_script_t script) {
    std::array<char, TAG_LENGTH> tag{};
    hb_tag_to_string(hb_script_to_iso15924_tag(script), tag.data());
    return std::string(tag.data(), tag.size());
}

/**
 * Shape one run in place, filling its glyphs and its advance.
 **/
void shapeRun(std::string_view utf8, hb_script_t script, float size, const std::vector<hb_feature_t>& features, ShapedRun* run) {
    const hb_direction_t direction = run->direction == ShapedRun::Direction::RightToLeft ? HB_DIRECTION_RTL : HB_DIRECTION_LTR;
    const std::unique_ptr<hb_buffer_t, decltype(&hb_buffer_destroy)> buffer(hb_buffer_create(), &hb_buffer_destroy);
    // the whole string is the context, so a run is shaped knowing what is either side of it
    hb_buffer_add_utf8(buffer.get(), utf8.data(), static_cast<int>(utf8.size()), static_cast<unsigned int>(run->start),
        static_cast<int>(run->end - run->start));
    hb_buffer_set_script(buffer.get(), script);
    hb_buffer_set_direction(buffer.get(), direction);
    hb_buffer_set_cluster_level(buffer.get(), HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
    run->script = named(script);

    // advances come from the face's own metrics, unhinted, at the size in 26.6 fixed point
    const std::unique_ptr<hb_font_t, decltype(&hb_font_destroy)> font(hb_font_create(run->face->handle().shaping), &hb_font_destroy);
    const int scale = static_cast<int>(size * FIXED_POINT_SCALE);  // checked: size is from 0 to Shaper::LARGEST
    hb_font_set_scale(font.get(), scale, scale);
    hb_shape(font.get(), buffer.get(), features.data(), static_cast<unsigned int>(features.size()));

    unsigned int count = 0;
    const hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer.get(), &count);
    const hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer.get(), &count);
    run->glyphs.reserve(count);
    for (unsigned int index = 0; index < count; index++) {
        // HarfBuzz's y grows upwards
        const ShapedGlyph glyph{ .index = infos[index].codepoint,
            .advance = static_cast<float>(positions[index].x_advance) / FIXED_POINT_SCALE,
            .offset = glm::vec2(static_cast<float>(positions[index].x_offset), -static_cast<float>(positions[index].y_offset)) / FIXED_POINT_SCALE,
            .cluster = infos[index].cluster };
        run->advance += glyph.advance;
        run->glyphs.push_back(glyph);
    }
}

};  // namespace

Shaper::Shaper(const boost::shared_ptr<FallbackChain>& chain) :
    chain_(chain) {
}

std::vector<ShapedRun> Shaper::shape(std::string_view utf8, const FaceStyle& style, float size, const std::vector<Feature>& features,
    BaseDirection base) const {
    std::vector<ShapedRun> runs;
    if (!chain_ || utf8.empty() || utf8.size() > static_cast<std::size_t>(INT_MAX) || !std::isfinite(size) || size <= 0.0F || size > LARGEST) {
        return runs;
    }

    // split into runs of one face, one script and one embedding level. A run whose characters so
    // far have no script takes the first script that follows
    const Bidi bidi(utf8, base);
    std::vector<hb_script_t> scripts;
    hb_unicode_funcs_t* unicode = hb_unicode_funcs_get_default();
    std::size_t index = 0;
    while (index < utf8.size()) {
        const std::size_t start = index;
        const char32_t charcode = next(utf8, &index);
        const hb_script_t script = hb_unicode_script(unicode, charcode);
        const std::uint8_t level = bidi.level(start);
        const boost::shared_ptr<Face> face = chain_->find(charcode, style).face;
        if (!face) {
            return {};
        }
        if (!runs.empty()) {
            ShapedRun& last = runs.back();
            const bool fits = neutral(script) || neutral(scripts.back()) || script == scripts.back();
            if (fits && last.level == level && (last.face == face || last.face->has(charcode))) {
                last.end = index;
                scripts.back() = neutral(script) ? scripts.back() : script;
                continue;
            }
        }
        const ShapedRun::Direction direction = (level & 1U) != 0 ? ShapedRun::Direction::RightToLeft : ShapedRun::Direction::LeftToRight;
        runs.push_back(ShapedRun{ .face = face, .direction = direction, .level = level, .start = start, .end = index });
        scripts.push_back(script);
    }

    std::vector<hb_feature_t> settings;
    for (const Feature& feature : features) {
        if (feature.tag.size() == TAG_LENGTH) {
            settings.push_back(hb_feature_t{ .tag = hb_tag_from_string(feature.tag.data(), static_cast<int>(TAG_LENGTH)), .value = feature.value,
                .start = HB_FEATURE_GLOBAL_START, .end = HB_FEATURE_GLOBAL_END });
        }
    }
    for (std::size_t run = 0; run < runs.size(); run++) {
        // text with no script at all, such as digits and spaces alone, is shaped as Latin
        shapeRun(utf8, neutral(scripts[run]) ? HB_SCRIPT_LATIN : scripts[run], size, settings, &runs[run]);
    }
    return runs;
}

};  // namespace v3d::font
