/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shaper.h"

#include <api/font/internal/FaceHandle.h>
#include <hb.h>
#include <unicode/uchar.h>
#include <unicode/umachine.h>

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
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
#include "Variation.h"

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
 * The characters that change what joins a cluster, and how it asks to be shown.
 **/
constexpr char32_t ZERO_WIDTH_JOINER = 0x200D;
constexpr char32_t TEXT_PRESENTATION = 0xFE0E;
constexpr char32_t EMOJI_PRESENTATION = 0xFE0F;
constexpr char32_t KEYCAP = 0x20E3;

/**
 * @return whether a character has one of ICU's binary properties
 **/
bool property(char32_t charcode, UProperty which) {
    return u_hasBinaryProperty(static_cast<UChar32>(charcode), which) != 0;
}

/**
 * @return whether a character joins the cluster before it: a mark, a joiner, a variation
 *         selector, an emoji modifier or a tag
 **/
bool extends(char32_t charcode) {
    return charcode == ZERO_WIDTH_JOINER || property(charcode, UCHAR_GRAPHEME_EXTEND) || property(charcode, UCHAR_EMOJI_MODIFIER) ||
        u_charType(static_cast<UChar32>(charcode)) == U_COMBINING_SPACING_MARK;
}

/**
 * Read the cluster a string starts with.
 *
 * @param characters set to its characters, in order
 * @return how many bytes it takes
 **/
std::size_t cluster(std::string_view utf8, std::vector<char32_t>* characters) {
    characters->clear();
    std::size_t index = 0;
    characters->push_back(next(utf8, &index));
    bool joined = false;
    while (index < utf8.size()) {
        std::size_t after = index;
        const char32_t charcode = next(utf8, &after);
        const bool flag = characters->size() == 1 && property(characters->front(), UCHAR_REGIONAL_INDICATOR) && property(charcode, UCHAR_REGIONAL_INDICATOR);
        if (!joined && !flag && !extends(charcode)) {
            break;
        }
        joined = charcode == ZERO_WIDTH_JOINER;
        characters->push_back(charcode);
        index = after;
    }
    return index;
}

/**
 * @return whether a cluster asks to be shown as an emoji
 **/
bool asksForEmoji(const std::vector<char32_t>& characters) {
    bool asked = property(characters.front(), UCHAR_EMOJI_PRESENTATION) || property(characters.front(), UCHAR_REGIONAL_INDICATOR);
    for (const char32_t charcode : characters) {
        if (charcode == TEXT_PRESENTATION) {
            return false;
        }
        asked = asked || charcode == EMOJI_PRESENTATION || charcode == KEYCAP || property(charcode, UCHAR_EMOJI_MODIFIER);
    }
    return asked;
}

/**
 * @return whether a face has every character of a cluster but the invisible ones
 **/
bool holds(const Face& face, const std::vector<char32_t>& characters) {
    return std::ranges::all_of(characters, [&face](char32_t charcode) { return property(charcode, UCHAR_DEFAULT_IGNORABLE_CODE_POINT) || face.has(charcode); });
}

/**
 * @return whether a face draws in colour, read once for each face and kept in a map
 **/
bool coloured(const Face& face, std::map<const Face*, bool>* colourFaces) {
    const auto found = colourFaces->find(&face);
    return found != colourFaces->end() ? found->second : colourFaces->emplace(&face, face.colour().any()).first->second;
}

/**
 * @return the first of a chain's faces a cluster is set in: a colour face with its first
 *         character when it asks for emoji, and otherwise a face with all its visible characters,
 *         or none
 **/
boost::shared_ptr<Face> faceFor(const std::vector<boost::shared_ptr<Face>>& faces, const std::vector<char32_t>& characters, bool emoji,
    std::map<const Face*, bool>* colourFaces) {
    for (const boost::shared_ptr<Face>& each : faces) {
        if (each && (emoji ? coloured(*each, colourFaces) && each->has(characters.front()) : holds(*each, characters))) {
            return each;
        }
    }
    return boost::shared_ptr<Face>();
}

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
 * @return features as HarfBuzz takes them, each across the whole string, leaving out a tag that
 *         is not four letters
 **/
std::vector<hb_feature_t> settingsOf(const std::vector<Feature>& features) {
    std::vector<hb_feature_t> settings;
    for (const Feature& feature : features) {
        if (feature.tag.size() == TAG_LENGTH) {
            settings.push_back(hb_feature_t{ .tag = hb_tag_from_string(feature.tag.data(), static_cast<int>(TAG_LENGTH)), .value = feature.value,
                .start = HB_FEATURE_GLOBAL_START, .end = HB_FEATURE_GLOBAL_END });
        }
    }
    return settings;
}

/**
 * @return the coordinates a face is shaped at, found once for each face of a string
 **/
const std::vector<std::int32_t>& coordinatesOf(const Face& face, const FaceStyle& style, float size, const std::vector<Variation>& variations,
    std::map<const Face*, std::vector<std::int32_t>>* found) {
    auto at = found->find(&face);
    if (at == found->end()) {
        at = found->emplace(&face, face.coordinates(style, size, variations)).first;
    }
    return at->second;
}

/**
 * @return a BCP 47 tag as HarfBuzz takes it, or none for an empty tag
 **/
hb_language_t languageOf(std::string_view tag) {
    return tag.empty() ? HB_LANGUAGE_INVALID : hb_language_from_string(tag.data(), static_cast<int>(tag.size()));
}

/**
 * Shape one run in place, filling its glyphs and its advance.
 **/
void shapeRun(std::string_view utf8, hb_script_t script, hb_language_t language, float size, const std::vector<hb_feature_t>& features, ShapedRun* run) {
    const hb_direction_t direction = run->direction == ShapedRun::Direction::RightToLeft ? HB_DIRECTION_RTL : HB_DIRECTION_LTR;
    const std::unique_ptr<hb_buffer_t, decltype(&hb_buffer_destroy)> buffer(hb_buffer_create(), &hb_buffer_destroy);
    // the whole string is the context, so a run is shaped knowing what is either side of it
    hb_buffer_add_utf8(buffer.get(), utf8.data(), static_cast<int>(utf8.size()), static_cast<unsigned int>(run->start),
        static_cast<int>(run->end - run->start));
    hb_buffer_set_script(buffer.get(), script);
    if (language != HB_LANGUAGE_INVALID) {
        hb_buffer_set_language(buffer.get(), language);
    }
    hb_buffer_set_direction(buffer.get(), direction);
    hb_buffer_set_cluster_level(buffer.get(), HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
    run->script = named(script);

    // advances come from the face's own metrics, unhinted, at the size in 26.6 fixed point
    const std::unique_ptr<hb_font_t, decltype(&hb_font_destroy)> font(hb_font_create(run->face->handle().shaping), &hb_font_destroy);
    const int scale = static_cast<int>(size * FIXED_POINT_SCALE);  // checked: size is from 0 to Shaper::LARGEST
    hb_font_set_scale(font.get(), scale, scale);
    if (!run->coordinates.empty()) {
        hb_font_set_var_coords_normalized(font.get(), run->coordinates.data(), static_cast<unsigned int>(run->coordinates.size()));
    }
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
    const std::vector<Variation>& variations, std::string_view language, BaseDirection base) const {
    if (utf8.size() > static_cast<std::size_t>(INT_MAX)) {
        return {};
    }
    return shape(utf8, 0, utf8.size(), Bidi(utf8, base), style, size, features, variations, language);
}

std::vector<ShapedRun> Shaper::shape(std::string_view utf8, std::size_t from, std::size_t to, const Bidi& bidi, const FaceStyle& style, float size,
    const std::vector<Feature>& features, const std::vector<Variation>& variations, std::string_view language) const {
    std::vector<ShapedRun> runs;
    if (!chain_ || from >= to || to > utf8.size() || utf8.size() > static_cast<std::size_t>(INT_MAX) || !std::isfinite(size) || size <= 0.0F ||
        size > LARGEST) {
        return runs;
    }

    // split into runs of one face, one script and one embedding level, a cluster at a time. A run
    // whose characters so far have no script takes the first script that follows
    std::vector<hb_script_t> scripts;
    hb_unicode_funcs_t* unicode = hb_unicode_funcs_get_default();
    const std::vector<boost::shared_ptr<Face>> faces = chain_->faces(style);
    std::map<const Face*, bool> colourFaces;
    std::vector<char32_t> characters;
    std::size_t index = from;
    while (index < to) {
        const std::size_t start = index;
        index += cluster(utf8.substr(start, to - start), &characters);
        const char32_t charcode = characters.front();
        const hb_script_t script = hb_unicode_script(unicode, charcode);
        const std::uint8_t level = bidi.level(start);
        const bool emoji = asksForEmoji(characters);
        boost::shared_ptr<Face> face = faceFor(faces, characters, emoji, &colourFaces);
        face = face ? face : chain_->find(charcode, style).face;
        if (!face) {
            return {};
        }
        if (!runs.empty()) {
            ShapedRun& last = runs.back();
            const bool fits = neutral(script) || neutral(scripts.back()) || script == scripts.back();
            const bool held = !emoji && !coloured(*last.face, &colourFaces) && holds(*last.face, characters);
            if (fits && last.level == level && (last.face == face || held)) {
                last.end = index;
                scripts.back() = neutral(script) ? scripts.back() : script;
                continue;
            }
        }
        const ShapedRun::Direction direction = (level & 1U) != 0 ? ShapedRun::Direction::RightToLeft : ShapedRun::Direction::LeftToRight;
        runs.push_back(ShapedRun{ .face = face, .direction = direction, .level = level, .start = start, .end = index });
        scripts.push_back(script);
    }

    const std::vector<hb_feature_t> settings = settingsOf(features);
    const hb_language_t tagged = languageOf(language);
    std::map<const Face*, std::vector<std::int32_t>> coordinates;
    for (std::size_t run = 0; run < runs.size(); run++) {
        runs[run].coordinates = coordinatesOf(*runs[run].face, style, size, variations, &coordinates);
        // text with no script at all, such as digits and spaces alone, is shaped as Latin
        shapeRun(utf8, neutral(scripts[run]) ? HB_SCRIPT_LATIN : scripts[run], tagged, size, settings, &runs[run]);
    }
    return runs;
}

};  // namespace v3d::font
