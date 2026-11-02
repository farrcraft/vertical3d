/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Composite.h"

#include <api/font/ColourGlyph.h>

#include <algorithm>
#include <cmath>

#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::font::internal {

namespace {

/**
 * How much red, green and blue each give to a colour's luminosity.
 **/
constexpr float RED_LUMINOSITY = 0.3F;
constexpr float GREEN_LUMINOSITY = 0.59F;
constexpr float BLUE_LUMINOSITY = 0.11F;

/**
 * Halfway, where the light modes turn from multiplying to screening.
 **/
constexpr float HALF = 0.5F;

/**
 * Where soft light's curve turns from a cubic to a square root.
 **/
constexpr float QUARTER = 0.25F;

/**
 * The cubic soft light follows below a quarter: ((16 b - 12) b + 4) b.
 **/
constexpr float SOFT_CUBE = 16.0F;
constexpr float SOFT_SQUARE = 12.0F;
constexpr float SOFT_LINEAR = 4.0F;

/**
 * @return how much of the source and of the destination a Porter and Duff mode keeps, by the
 *         source's and the destination's alpha, or nothing for a mode that blends
 **/
bool porterDuff(ColourComposite mode, float source, float destination, glm::vec2* keeps) {
    switch (mode) {
    case ColourComposite::Clear:
        *keeps = glm::vec2(0.0F, 0.0F);
        return true;
    case ColourComposite::Source:
        *keeps = glm::vec2(1.0F, 0.0F);
        return true;
    case ColourComposite::Destination:
        *keeps = glm::vec2(0.0F, 1.0F);
        return true;
    case ColourComposite::SourceOver:
        *keeps = glm::vec2(1.0F, 1.0F - source);
        return true;
    case ColourComposite::DestinationOver:
        *keeps = glm::vec2(1.0F - destination, 1.0F);
        return true;
    case ColourComposite::SourceIn:
        *keeps = glm::vec2(destination, 0.0F);
        return true;
    case ColourComposite::DestinationIn:
        *keeps = glm::vec2(0.0F, source);
        return true;
    case ColourComposite::SourceOut:
        *keeps = glm::vec2(1.0F - destination, 0.0F);
        return true;
    case ColourComposite::DestinationOut:
        *keeps = glm::vec2(0.0F, 1.0F - source);
        return true;
    case ColourComposite::SourceAtop:
        *keeps = glm::vec2(destination, 1.0F - source);
        return true;
    case ColourComposite::DestinationAtop:
        *keeps = glm::vec2(1.0F - destination, source);
        return true;
    case ColourComposite::Xor:
        *keeps = glm::vec2(1.0F - destination, 1.0F - source);
        return true;
    case ColourComposite::Plus:
        *keeps = glm::vec2(1.0F, 1.0F);
        return true;
    default:
        return false;
    }
}

float multiply(float backdrop, float source) {
    return backdrop * source;
}

float screen(float backdrop, float source) {
    return backdrop + source - (backdrop * source);
}

float hardLight(float backdrop, float source) {
    return source <= HALF ? multiply(backdrop, 2.0F * source) : screen(backdrop, (2.0F * source) - 1.0F);
}

float softLight(float backdrop, float source) {
    if (source <= HALF) {
        return backdrop - ((1.0F - (2.0F * source)) * backdrop * (1.0F - backdrop));
    }
    const float lifted = backdrop <= QUARTER ? ((((SOFT_CUBE * backdrop) - SOFT_SQUARE) * backdrop) + SOFT_LINEAR) * backdrop : std::sqrt(backdrop);
    return backdrop + (((2.0F * source) - 1.0F) * (lifted - backdrop));
}

float colourDodge(float backdrop, float source) {
    if (backdrop <= 0.0F) {
        return 0.0F;
    }
    return source >= 1.0F ? 1.0F : std::min(1.0F, backdrop / (1.0F - source));
}

float colourBurn(float backdrop, float source) {
    if (backdrop >= 1.0F) {
        return 1.0F;
    }
    return source <= 0.0F ? 0.0F : 1.0F - std::min(1.0F, (1.0F - backdrop) / source);
}

/**
 * @return one channel of a separable blend mode
 **/
float separable(ColourComposite mode, float backdrop, float source) {
    switch (mode) {
    case ColourComposite::Screen:
        return screen(backdrop, source);
    case ColourComposite::Overlay:
        // overlay is hard light with its layers the other way round
        return hardLight(source, backdrop);  // NOLINT(readability-suspicious-call-argument) - swapped on purpose
    case ColourComposite::Darken:
        return std::min(backdrop, source);
    case ColourComposite::Lighten:
        return std::max(backdrop, source);
    case ColourComposite::ColourDodge:
        return colourDodge(backdrop, source);
    case ColourComposite::ColourBurn:
        return colourBurn(backdrop, source);
    case ColourComposite::HardLight:
        return hardLight(backdrop, source);
    case ColourComposite::SoftLight:
        return softLight(backdrop, source);
    case ColourComposite::Difference:
        return std::abs(backdrop - source);
    case ColourComposite::Exclusion:
        return backdrop + source - (2.0F * backdrop * source);
    default:
        return multiply(backdrop, source);
    }
}

float luminosity(const glm::vec3& colour) {
    return (colour.r * RED_LUMINOSITY) + (colour.g * GREEN_LUMINOSITY) + (colour.b * BLUE_LUMINOSITY);
}

/**
 * @return a colour brought back inside 0 to 1 with its luminosity kept
 **/
glm::vec3 clip(const glm::vec3& colour) {
    const float lum = luminosity(colour);
    const float low = std::min({ colour.r, colour.g, colour.b });
    const float high = std::max({ colour.r, colour.g, colour.b });
    glm::vec3 clipped = colour;
    if (low < 0.0F && lum != low) {
        clipped = lum + (((clipped - lum) * lum) / (lum - low));
    }
    if (high > 1.0F && high != lum) {
        clipped = lum + (((clipped - lum) * (1.0F - lum)) / (high - lum));
    }
    return clipped;
}

glm::vec3 withLuminosity(const glm::vec3& colour, float lum) {
    return clip(colour + (lum - luminosity(colour)));
}

float saturation(const glm::vec3& colour) {
    return std::max({ colour.r, colour.g, colour.b }) - std::min({ colour.r, colour.g, colour.b });
}

/**
 * @return a colour with its saturation set, its channels keeping their order
 **/
glm::vec3 withSaturation(const glm::vec3& colour, float sat) {
    int low = 0;
    int high = 0;
    for (int channel = 1; channel < 3; channel++) {
        low = colour[channel] < colour[low] ? channel : low;
        high = colour[channel] > colour[high] ? channel : high;
    }
    glm::vec3 made(0.0F);
    if (low == high || colour[high] <= colour[low]) {
        return made;
    }
    const int middle = 3 - low - high;
    made[middle] = ((colour[middle] - colour[low]) * sat) / (colour[high] - colour[low]);
    made[high] = sat;
    return made;
}

/**
 * @return the colour of a blend mode, unpremultiplied
 **/
glm::vec3 blend(ColourComposite mode, const glm::vec3& backdrop, const glm::vec3& source) {
    switch (mode) {
    case ColourComposite::Hue:
        return withLuminosity(withSaturation(source, saturation(backdrop)), luminosity(backdrop));
    case ColourComposite::Saturation:
        return withLuminosity(withSaturation(backdrop, saturation(source)), luminosity(backdrop));
    case ColourComposite::Colour:
        return withLuminosity(source, luminosity(backdrop));
    case ColourComposite::Luminosity:
        return withLuminosity(backdrop, luminosity(source));
    default:
        return glm::vec3(separable(mode, backdrop.r, source.r), separable(mode, backdrop.g, source.g), separable(mode, backdrop.b, source.b));
    }
}

};  // namespace

glm::vec4 composite(const glm::vec4& source, const glm::vec4& destination, ColourComposite mode) {
    glm::vec2 keeps(0.0F);
    if (porterDuff(mode, source.a, destination.a, &keeps)) {
        return glm::min((source * keeps.x) + (destination * keeps.y), glm::vec4(1.0F));
    }
    // where both are present the blend shows, and where one is alone it shows as it is
    const glm::vec3 backdrop = destination.a > 0.0F ? glm::vec3(destination) / destination.a : glm::vec3(0.0F);
    const glm::vec3 painted = source.a > 0.0F ? glm::vec3(source) / source.a : glm::vec3(0.0F);
    const glm::vec3 mixed = ((1.0F - destination.a) * glm::vec3(source)) + ((1.0F - source.a) * glm::vec3(destination))
        + (source.a * destination.a * blend(mode, backdrop, painted));
    return glm::vec4(mixed, source.a + destination.a - (source.a * destination.a));
}

};  // namespace v3d::font::internal
