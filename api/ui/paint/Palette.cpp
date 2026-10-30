/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Palette.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "Dressing.h"

namespace v3d::ui::paint {

namespace {

/**
 * A colour in OKLCH: perceived lightness from 0 to 1, chroma, and hue in radians.
 **/
struct Lch final {
    float lightness;
    float chroma;
    float hue;
};

/** how many channels a colour has, not counting alpha **/
constexpr std::size_t AXES = 3;
using Triple = std::array<float, AXES>;
/** a 3 by 3 matrix, row by row **/
using Matrix = std::array<float, AXES * AXES>;

// the sRGB transfer function's two pieces, and where they meet
constexpr float GAMMA_KNEE = 0.04045F;
constexpr float LINEAR_KNEE = 0.0031308F;
constexpr float LINEAR_SLOPE = 12.92F;
constexpr float GAMMA_OFFSET = 0.055F;
constexpr float GAMMA_EXPONENT = 2.4F;

// linear sRGB to the cone responses OKLab starts from, row by row
constexpr Matrix RGB_TO_LMS{
0.4122214708F, 0.5363325363F, 0.0514459929F,
0.2119034982F, 0.6806995451F, 0.1073969566F,
0.0883024619F, 0.2817188376F, 0.6299787005F};
// the cube roots of the cone responses to L, a and b
constexpr Matrix LMS_TO_LAB{
0.2104542553F, 0.7936177850F, -0.0040720468F,
1.9779984951F, -2.4285922050F, 0.4505937099F,
0.0259040371F, 0.7827717662F, -0.8086757660F};
// L, a and b back to the cube roots of the cone responses
constexpr Matrix LAB_TO_LMS{
1.0F, 0.3963377774F, 0.2158037573F,
1.0F, -0.1055613458F, -0.0638541728F,
1.0F, -0.0894841775F, -1.2914855480F};
// the cone responses back to linear sRGB
constexpr Matrix LMS_TO_RGB{
4.0767416621F, -3.3077115913F, 0.2309699292F,
-1.2684380046F, 2.6097574011F, -0.3413193965F,
-0.0041960863F, -0.7034186147F, 1.7076147010F};

// the weights WCAG gives linear red, green and blue in a colour's luminance
constexpr float LUMINANCE_RED = 0.2126F;
constexpr float LUMINANCE_GREEN = 0.7152F;
constexpr float LUMINANCE_BLUE = 0.0722F;
// what WCAG adds to both luminances, for the glare on a screen
constexpr float LUMINANCE_FLARE = 0.05F;

// the neutral steps, darkest first in a dark theme
constexpr std::size_t GAP_STEP = 0;
constexpr std::size_t BAR_STEP = 1;
/** the base colour: the surface a panel is drawn on **/
constexpr std::size_t BASE_STEP = 2;
constexpr std::size_t TITLE_STEP = 3;
constexpr std::size_t FIELD_STEP = 4;
constexpr std::size_t WIDGET_STEP = 5;
constexpr std::size_t HOVER_STEP = 6;
constexpr std::size_t PRESSED_STEP = 7;
constexpr std::size_t OUTLINE_STEP = 8;
constexpr std::size_t DISABLED_STEP = 9;
constexpr std::size_t SECONDARY_STEP = 10;
constexpr std::size_t TEXT_STEP = 11;
/** the step of the accent scale that is the accent colour: the solid accent **/
constexpr std::size_t ACCENT_STEP = 8;
/** the step below the solid accent, which the highlight starts from **/
constexpr std::size_t DEEP_ACCENT_STEP = 7;

/**
 * How far each neutral step's lightness is from the base's, at a contrast of 1. In order: the
 * gap between areas, a bar, a panel, a title bar, a field, a widget, a hover, a pressed widget,
 * an outline, disabled text, secondary text and text.
 **/
constexpr std::array<float, Palette::steps> NEUTRAL_OFFSETS{
-0.0569F, -0.0296F, 0.0F, 0.0370F, 0.0573F, 0.0737F,
0.1133F, 0.1654F, 0.3051F, 0.3958F, 0.49F, 0.6857F};

/**
 * How much of the base's chroma each neutral step keeps. The middle steps carry more of the
 * base's hue than the darkest surfaces and the brightest text do.
 **/
constexpr std::array<float, Palette::steps> NEUTRAL_CHROMA{
0.53F, 0.74F, 1.0F, 1.24F, 1.22F, 1.40F,
1.56F, 1.57F, 1.86F, 1.85F, 1.80F, 0.71F};

/**
 * How far each solid accent step's lightness is from the accent's, at a contrast of 1. The
 * other accent steps are tints, which take the lightness of the neutral step they stand for.
 **/
constexpr std::size_t FIRST_SOLID_STEP = 6;
constexpr std::size_t LAST_SOLID_STEP = 10;
constexpr std::array<float, LAST_SOLID_STEP - FIRST_SOLID_STEP + 1> SOLID_OFFSETS{
-0.16F, -0.109F, 0.0F, 0.06F, 0.14F};
/** the first solid step that turns towards the text in a light theme rather than darkening **/
constexpr std::size_t FIRST_TURNING_STEP = 9;
/** how much of the accent's chroma a tint keeps **/
constexpr float TINT_CHROMA = 0.25F;

/** a base lighter than this is a light theme **/
constexpr float LIGHT_THEME = 0.6F;
/** the lightness of a light theme's active text **/
constexpr float DARK_INK = 0.15F;
/** the contrast the highlight keeps against the active text drawn over it **/
constexpr float LEGIBLE = 4.5F;
/** how far the highlight's lightness moves each time it is short of LEGIBLE **/
constexpr float LEGIBLE_STEP = 0.01F;

// how opaque the selection and the hairline are, in a dark theme and in a light one
constexpr float DARK_SELECTED_ALPHA = 0.24F;
constexpr float LIGHT_SELECTED_ALPHA = 0.18F;
constexpr float DARK_HAIRLINE_ALPHA = 0.07F;
constexpr float LIGHT_HAIRLINE_ALPHA = 0.10F;
// how dark a shadow is at its darkest, in a dark theme and in a light one. A light surface
// shows a shadow more, so it needs less
constexpr float DARK_SHADOW_ALPHA = 0.45F;
constexpr float LIGHT_SHADOW_ALPHA = 0.18F;
/** how much a backdrop darkens what is behind it, whatever the theme, since that is a game's scene **/
constexpr float BACKDROP_ALPHA = 0.6F;
/** what a shadow and a backdrop darken with **/
constexpr glm::vec4 BLACK(0.0F, 0.0F, 0.0F, 1.0F);

float linear(float channel) noexcept {
    return channel <= GAMMA_KNEE ? channel / LINEAR_SLOPE
        : std::pow((channel + GAMMA_OFFSET) / (1.0F + GAMMA_OFFSET), GAMMA_EXPONENT);
}

float encoded(float channel) noexcept {
    const float clipped = std::clamp(channel, 0.0F, 1.0F);
    return clipped <= LINEAR_KNEE ? clipped * LINEAR_SLOPE
        : ((1.0F + GAMMA_OFFSET) * std::pow(clipped, 1.0F / GAMMA_EXPONENT)) - GAMMA_OFFSET;
}

Triple multiply(const Matrix& matrix, const Triple& value) noexcept {
    Triple product{};
    for (std::size_t row = 0; row < AXES; row++) {
        for (std::size_t column = 0; column < AXES; column++) {
            product[row] += matrix[(row * AXES) + column] * value[column];
        }
    }
    return product;
}

Lch lch(const glm::vec4& colour) noexcept {
    Triple lms = multiply(RGB_TO_LMS, {linear(colour.r), linear(colour.g), linear(colour.b)});
    for (float& response : lms) {
        response = std::cbrt(response);
    }
    const Triple lab = multiply(LMS_TO_LAB, lms);
    return {.lightness = lab[0], .chroma = std::hypot(lab[1], lab[2]), .hue = std::atan2(lab[2], lab[1])};
}

/**
 * A colour out of the sRGB gamut is clipped channel by channel.
 **/
glm::vec4 rgb(const Lch& colour, float alpha) noexcept {
    const float lightness = std::clamp(colour.lightness, 0.0F, 1.0F);
    Triple lms = multiply(LAB_TO_LMS,
        {lightness, colour.chroma * std::cos(colour.hue), colour.chroma * std::sin(colour.hue)});
    for (float& response : lms) {
        response = response * response * response;
    }
    const Triple channels = multiply(LMS_TO_RGB, lms);
    return {encoded(channels[0]), encoded(channels[1]), encoded(channels[2]), alpha};
}

float luminance(const glm::vec4& colour) noexcept {
    return (LUMINANCE_RED * linear(colour.r)) + (LUMINANCE_GREEN * linear(colour.g)) +
        (LUMINANCE_BLUE * linear(colour.b));
}

glm::vec4 faded(const glm::vec4& colour, float alpha) noexcept {
    return {colour.r, colour.g, colour.b, alpha};
}

};  // namespace

Palette::Palette() noexcept :
Palette(base, accent, 1.0F) {
}

Palette::Palette(const glm::vec4& baseColour, const glm::vec4& accentColour, float contrast) noexcept :
neutral_(),
accented_(),
activeText_(),
highlight_(),
dark_(true) {
    const Lch surface = lch(baseColour);
    const Lch solid = lch(accentColour);
    const float spread = std::max(0.0F, contrast);
    dark_ = surface.lightness < LIGHT_THEME;
    // a light theme's steps run the other way, so text stays the step furthest from its surface
    const float away = dark_ ? 1.0F : -1.0F;

    for (std::size_t step = 0; step < steps; step++) {
        const float lightness = surface.lightness + (away * spread * NEUTRAL_OFFSETS[step]);
        neutral_[step] = rgb({.lightness = lightness, .chroma = surface.chroma * NEUTRAL_CHROMA[step],
            .hue = surface.hue}, 1.0F);

        if (step >= FIRST_SOLID_STEP && step <= LAST_SOLID_STEP) {
            // the steps below the accent darken in either theme, so a highlight behind light
            // text has somewhere to go. The steps above it turn towards the text
            const float turn = step >= FIRST_TURNING_STEP ? away : 1.0F;
            const float offset = SOLID_OFFSETS[step - FIRST_SOLID_STEP];
            accented_[step] = rgb({.lightness = solid.lightness + (turn * spread * offset),
                .chroma = solid.chroma, .hue = solid.hue}, 1.0F);
        } else {
            accented_[step] = rgb({.lightness = lightness, .chroma = solid.chroma * TINT_CHROMA,
                .hue = solid.hue}, 1.0F);
        }
    }

    activeText_ = dark_ ? glm::vec4(1.0F, 1.0F, 1.0F, 1.0F)
        : rgb({.lightness = DARK_INK, .chroma = surface.chroma, .hue = surface.hue}, 1.0F);

    // the highlight moves away from the active text until the two can be read together
    Lch deep = lch(accented_[DEEP_ACCENT_STEP]);
    highlight_ = accented_[DEEP_ACCENT_STEP];
    while (contrastRatio(highlight_, activeText_) < LEGIBLE && deep.lightness > 0.0F && deep.lightness < 1.0F) {
        deep.lightness += dark_ ? -LEGIBLE_STEP : LEGIBLE_STEP;
        highlight_ = rgb(deep, 1.0F);
    }
}

const Palette::Scale& Palette::neutral() const noexcept {
    return neutral_;
}

const Palette::Scale& Palette::accented() const noexcept {
    return accented_;
}

bool Palette::dark() const noexcept {
    return dark_;
}

Palette::Roles Palette::roles(Use use) const noexcept {
    const bool tools = use == Use::Tools;
    const Scale& grey = neutral_;
    const Scale& tint = accented_;
    return {{
        {.field = &Dressing::gap, .colour = grey[GAP_STEP]},
        {.field = &Dressing::bar, .colour = grey[BAR_STEP]},
        {.field = &Dressing::panel, .colour = grey[BASE_STEP]},
        {.field = &Dressing::titleBar, .colour = grey[TITLE_STEP]},
        {.field = &Dressing::track, .colour = tools ? grey[WIDGET_STEP] : grey[FIELD_STEP]},
        {.field = &Dressing::tab, .colour = tools ? grey[WIDGET_STEP] : grey[BAR_STEP]},
        {.field = &Dressing::widget, .colour = grey[WIDGET_STEP]},
        {.field = &Dressing::hover, .colour = grey[HOVER_STEP]},
        {.field = &Dressing::rule, .colour = grey[HOVER_STEP]},
        {.field = &Dressing::pressed, .colour = grey[PRESSED_STEP]},
        {.field = &Dressing::border, .colour = grey[OUTLINE_STEP]},
        {.field = &Dressing::thumb, .colour = grey[OUTLINE_STEP]},
        {.field = &Dressing::disabledText, .colour = grey[DISABLED_STEP]},
        {.field = &Dressing::placeholder, .colour = grey[DISABLED_STEP]},
        {.field = &Dressing::secondaryText, .colour = grey[SECONDARY_STEP]},
        {.field = &Dressing::text, .colour = grey[TEXT_STEP]},
        {.field = &Dressing::activeText, .colour = activeText_},
        {.field = &Dressing::hairline, .colour = faded(grey[TEXT_STEP], dark_ ? DARK_HAIRLINE_ALPHA : LIGHT_HAIRLINE_ALPHA)},
        {.field = &Dressing::highlight, .colour = highlight_},
        {.field = &Dressing::selected, .colour = faded(tint[ACCENT_STEP], dark_ ? DARK_SELECTED_ALPHA : LIGHT_SELECTED_ALPHA)},
        {.field = &Dressing::fill, .colour = tint[ACCENT_STEP]},
        {.field = &Dressing::mark, .colour = tint[ACCENT_STEP]},
        {.field = &Dressing::caret, .colour = tint[ACCENT_STEP]},
        {.field = &Dressing::focus, .colour = tint[ACCENT_STEP]},
        {.field = &Dressing::shadow, .colour = faded(BLACK, dark_ ? DARK_SHADOW_ALPHA : LIGHT_SHADOW_ALPHA)},
        {.field = &Dressing::backdrop, .colour = faded(BLACK, BACKDROP_ALPHA)}}};
}

void Palette::dress(Use use, Dressing* dressing) const noexcept {
    if (dressing == nullptr) {
        return;
    }
    for (const Role& role : roles(use)) {
        dressing->*(role.field) = role.colour;
    }
}

float contrastRatio(const glm::vec4& one, const glm::vec4& other) noexcept {
    const float first = luminance(one) + LUMINANCE_FLARE;
    const float second = luminance(other) + LUMINANCE_FLARE;
    return std::max(first, second) / std::min(first, second);
}

};  // namespace v3d::ui::paint
