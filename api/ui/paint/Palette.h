/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <glm/vec4.hpp>

#include "Dressing.h"

namespace v3d::ui::paint {

/**
 * Two scales of colour worked out from a base colour, an accent colour and a contrast, and
 * the colour of every Dressing role taken from them.
 *
 * The neutral scale has twelve steps of one hue, from the darkest surface to the brightest
 * text. The base colour is its third step, the surface a panel is drawn on. The accent scale
 * has twelve steps of the accent's hue, and the accent colour is its ninth step, the solid
 * accent. Each step's lightness is a fixed distance from the base's or the accent's, in the
 * OKLCH colour space, where equal distances look equally far apart.
 *
 * The contrast multiplies every distance, so above 1 the steps spread apart and below 1 they
 * draw together. A base lighter than mid grey is a light theme: the steps run the other way,
 * and text is darker than its surface.
 *
 * Each role is one step of one scale. The highlight is the exception: it starts one step below
 * the solid accent, and moves away from the active text until the two meet 4.5:1.
 **/
class Palette final {
 public:
    /** How many steps each scale has. **/
    static constexpr std::size_t steps = 12;

    using Scale = std::array<glm::vec4, steps>;

    /**
     * Which ui model the roles are for. The immediate layer's tracks and tabs sit at the level
     * of its widgets, a step above the retained ui's.
     **/
    enum class Use : std::uint8_t {
        Chrome,
        Tools
    };

    /**
     * One role and the colour the palette gives it.
     **/
    struct Role final {
        glm::vec4 Dressing::* field;
        glm::vec4 colour;
    };

    /** How many colour roles a Dressing has, each of which a palette gives a colour. **/
    static constexpr std::size_t roleCount = 26;

    using Roles = std::array<Role, roleCount>;

    /** The base colour a palette is built from when a theme names none, #1d1f23. **/
    static constexpr glm::vec4 base{0.113725F, 0.121569F, 0.137255F, 1.0F};
    /** The accent colour a palette is built from when a theme names none, #4d8ce6. **/
    static constexpr glm::vec4 accent{0.301961F, 0.549020F, 0.901961F, 1.0F};

    /**
     * The palette built from the built-in base and accent, at a contrast of 1.
     **/
    Palette() noexcept;

    /**
     * @param baseColour the surface a panel is drawn on. Its alpha is ignored
     * @param accentColour the solid accent. Its alpha is ignored
     * @param contrast how far apart the steps are, where 1 is the built-in spread. A value
     *      below zero counts as zero
     **/
    Palette(const glm::vec4& baseColour, const glm::vec4& accentColour, float contrast) noexcept;

    const Scale& neutral() const noexcept;
    const Scale& accented() const noexcept;

    /**
     * @return whether text is lighter than the surfaces it is drawn on
     **/
    bool dark() const noexcept;

    /**
     * @return every colour role of a Dressing, with the colour this palette gives it
     **/
    Roles roles(Use use) const noexcept;

    /**
     * Write every colour role into a dressing. Metrics are left as they are.
     **/
    void dress(Use use, Dressing* dressing) const noexcept;

 private:
    /**
     * A colour in OKLCH: perceived lightness from 0 to 1, chroma, and hue in radians.
     **/
    struct Lch final {
        float lightness;
        float chroma;
        float hue;
    };

    /** how many channels a colour has, not counting alpha **/
    static constexpr std::size_t AXES = 3;
    using Triple = std::array<float, AXES>;

    /** a 3 by 3 matrix, row by row **/
    using Matrix = std::array<float, AXES * AXES>;

    // where the sRGB transfer function's two pieces meet, in linear light
    static constexpr float LINEAR_KNEE = 0.0031308F;

    // linear sRGB to the cone responses OKLab starts from, row by row
    static constexpr Matrix RGB_TO_LMS{
        0.4122214708F, 0.5363325363F, 0.0514459929F,
        0.2119034982F, 0.6806995451F, 0.1073969566F,
        0.0883024619F, 0.2817188376F, 0.6299787005F};

    // the cube roots of the cone responses to L, a and b
    static constexpr Matrix LMS_TO_LAB{
        0.2104542553F, 0.7936177850F, -0.0040720468F,
        1.9779984951F, -2.4285922050F, 0.4505937099F,
        0.0259040371F, 0.7827717662F, -0.8086757660F};

    // L, a and b back to the cube roots of the cone responses
    static constexpr Matrix LAB_TO_LMS{
        1.0F, 0.3963377774F, 0.2158037573F,
        1.0F, -0.1055613458F, -0.0638541728F,
        1.0F, -0.0894841775F, -1.2914855480F};

    // the cone responses back to linear sRGB
    static constexpr Matrix LMS_TO_RGB{
        4.0767416621F, -3.3077115913F, 0.2309699292F,
        -1.2684380046F, 2.6097574011F, -0.3413193965F,
        -0.0041960863F, -0.7034186147F, 1.7076147010F};

    // the neutral steps, darkest first in a dark theme
    static constexpr std::size_t GAP_STEP = 0;
    static constexpr std::size_t BAR_STEP = 1;

    /** the base colour: the surface a panel is drawn on **/
    static constexpr std::size_t BASE_STEP = 2;
    static constexpr std::size_t TITLE_STEP = 3;
    static constexpr std::size_t FIELD_STEP = 4;
    static constexpr std::size_t WIDGET_STEP = 5;
    static constexpr std::size_t HOVER_STEP = 6;
    static constexpr std::size_t PRESSED_STEP = 7;
    static constexpr std::size_t OUTLINE_STEP = 8;
    static constexpr std::size_t DISABLED_STEP = 9;
    static constexpr std::size_t SECONDARY_STEP = 10;
    static constexpr std::size_t TEXT_STEP = 11;

    /** the step of the accent scale that is the accent colour: the solid accent **/
    static constexpr std::size_t ACCENT_STEP = 8;

    /** the step below the solid accent, which the highlight starts from **/
    static constexpr std::size_t DEEP_ACCENT_STEP = 7;

    /**
     * How far each neutral step's lightness is from the base's, at a contrast of 1. In order: the
     * gap between areas, a bar, a panel, a title bar, a field, a widget, a hover, a pressed widget,
     * an outline, disabled text, secondary text and text.
     **/
    static constexpr std::array<float, Palette::steps> NEUTRAL_OFFSETS{
        -0.0569F, -0.0296F, 0.0F, 0.0370F, 0.0573F, 0.0737F,
        0.1133F, 0.1654F, 0.3051F, 0.3958F, 0.49F, 0.6857F};

    /**
     * How much of the base's chroma each neutral step keeps. The middle steps carry more of the
     * base's hue than the darkest surfaces and the brightest text do.
     **/
    static constexpr std::array<float, Palette::steps> NEUTRAL_CHROMA{
        0.53F, 0.74F, 1.0F, 1.24F, 1.22F, 1.40F,
        1.56F, 1.57F, 1.86F, 1.85F, 1.80F, 0.71F};

    /**
     * How far each solid accent step's lightness is from the accent's, at a contrast of 1. The
     * other accent steps are tints, which take the lightness of the neutral step they stand for.
     **/
    static constexpr std::size_t FIRST_SOLID_STEP = 6;
    static constexpr std::size_t LAST_SOLID_STEP = 10;
    static constexpr std::array<float, LAST_SOLID_STEP - FIRST_SOLID_STEP + 1> SOLID_OFFSETS{
        -0.16F, -0.109F, 0.0F, 0.06F, 0.14F};

    /** the first solid step that turns towards the text in a light theme rather than darkening **/
    static constexpr std::size_t FIRST_TURNING_STEP = 9;

    /** how much of the accent's chroma a tint keeps **/
    static constexpr float TINT_CHROMA = 0.25F;

    /** a base lighter than this is a light theme **/
    static constexpr float LIGHT_THEME = 0.6F;

    /** the lightness of a light theme's active text **/
    static constexpr float DARK_INK = 0.15F;

    /** the contrast the highlight keeps against the active text drawn over it **/
    static constexpr float LEGIBLE = 4.5F;

    /** how far the highlight's lightness moves each time it is short of LEGIBLE **/
    static constexpr float LEGIBLE_STEP = 0.01F;

    // how opaque the selection and the hairline are, in a dark theme and in a light one
    static constexpr float DARK_SELECTED_ALPHA = 0.24F;
    static constexpr float LIGHT_SELECTED_ALPHA = 0.18F;
    static constexpr float DARK_HAIRLINE_ALPHA = 0.07F;
    static constexpr float LIGHT_HAIRLINE_ALPHA = 0.10F;

    // how dark a shadow is at its darkest, in a dark theme and in a light one. A light surface
    // shows a shadow more, so it needs less
    static constexpr float DARK_SHADOW_ALPHA = 0.45F;
    static constexpr float LIGHT_SHADOW_ALPHA = 0.18F;

    /**
     * how much a backdrop darkens what is behind it, whatever the theme, since that is a game's
     * scene
     **/
    static constexpr float BACKDROP_ALPHA = 0.6F;

    /** what a shadow and a backdrop darken with **/
    static constexpr glm::vec4 BLACK{0.0F, 0.0F, 0.0F, 1.0F};

    static float encoded(float channel) noexcept;
    static Triple multiply(const Matrix& matrix, const Triple& value) noexcept;
    static Lch lch(const glm::vec4& colour) noexcept;

    /**
     * A colour out of the sRGB gamut is clipped channel by channel.
     **/
    static glm::vec4 rgb(const Lch& colour, float alpha) noexcept;
    static glm::vec4 faded(const glm::vec4& colour, float alpha) noexcept;

    Scale neutral_;
    Scale accented_;
    glm::vec4 activeText_;
    glm::vec4 highlight_;
    bool dark_;
};

/**
 * The contrast ratio between two colours, as WCAG defines it: from 1, for two colours of the
 * same luminance, to 21, for black against white. Alpha is ignored.
 **/
float contrastRatio(const glm::vec4& one, const glm::vec4& other) noexcept;

};  // namespace v3d::ui::paint
