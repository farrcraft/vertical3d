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

// the sRGB transfer function's two pieces, and where they meet
constexpr float GAMMA_KNEE = 0.04045F;
constexpr float LINEAR_SLOPE = 12.92F;
constexpr float GAMMA_OFFSET = 0.055F;
constexpr float GAMMA_EXPONENT = 2.4F;

// the weights WCAG gives linear red, green and blue in a colour's luminance
constexpr float LUMINANCE_RED = 0.2126F;
constexpr float LUMINANCE_GREEN = 0.7152F;
constexpr float LUMINANCE_BLUE = 0.0722F;
// what WCAG adds to both luminances, for the glare on a screen
constexpr float LUMINANCE_FLARE = 0.05F;

float linear(float channel) noexcept {
    return channel <= GAMMA_KNEE ? channel / LINEAR_SLOPE
        : std::pow((channel + GAMMA_OFFSET) / (1.0F + GAMMA_OFFSET), GAMMA_EXPONENT);
}

float luminance(const glm::vec4& colour) noexcept {
    return (LUMINANCE_RED * linear(colour.r)) + (LUMINANCE_GREEN * linear(colour.g)) +
        (LUMINANCE_BLUE * linear(colour.b));
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

float Palette::encoded(float channel) noexcept {
    const float clipped = std::clamp(channel, 0.0F, 1.0F);
    return clipped <= LINEAR_KNEE ? clipped * LINEAR_SLOPE
        : ((1.0F + GAMMA_OFFSET) * std::pow(clipped, 1.0F / GAMMA_EXPONENT)) - GAMMA_OFFSET;
}

Palette::Triple Palette::multiply(const Matrix& matrix, const Triple& value) noexcept {
    Triple product{};
    for (std::size_t row = 0; row < AXES; row++) {
        for (std::size_t column = 0; column < AXES; column++) {
            product[row] += matrix[(row * AXES) + column] * value[column];
        }
    }
    return product;
}

Palette::Lch Palette::lch(const glm::vec4& colour) noexcept {
    Triple lms = multiply(RGB_TO_LMS, {linear(colour.r), linear(colour.g), linear(colour.b)});
    for (float& response : lms) {
        response = std::cbrt(response);
    }
    const Triple lab = multiply(LMS_TO_LAB, lms);
    return {.lightness = lab[0], .chroma = std::hypot(lab[1], lab[2]), .hue = std::atan2(lab[2], lab[1])};
}

glm::vec4 Palette::rgb(const Lch& colour, float alpha) noexcept {
    const float lightness = std::clamp(colour.lightness, 0.0F, 1.0F);
    Triple lms = multiply(LAB_TO_LMS,
        {lightness, colour.chroma * std::cos(colour.hue), colour.chroma * std::sin(colour.hue)});
    for (float& response : lms) {
        response = response * response * response;
    }
    const Triple channels = multiply(LMS_TO_RGB, lms);
    return {encoded(channels[0]), encoded(channels[1]), encoded(channels[2]), alpha};
}

glm::vec4 Palette::faded(const glm::vec4& colour, float alpha) noexcept {
    return {colour.r, colour.g, colour.b, alpha};
}

};  // namespace v3d::ui::paint
