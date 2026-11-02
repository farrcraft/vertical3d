/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Paint.h"

#include <api/font/internal/Composite.h>
#include <api/font/internal/LibraryHandle.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

#include "ColourGlyph.h"
#include "Curves.h"
#include "Face.h"
#include "GlyphStore.h"
#include "Library.h"
#include "Rasterize.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::font {

namespace {

/**
 * FreeType's outline coordinates are 26.6 fixed point, 64 to the pixel.
 **/
constexpr float FIXED_POINT = 64.0F;

/**
 * The most a byte holds.
 **/
constexpr float BYTE = 255.0F;

/**
 * The widest or tallest glyph painted, in pixels. No plot holds a larger one.
 **/
constexpr int LARGEST = 4096;

/**
 * How near a whole pixel a glyph's edge is taken to be on it, so that the error in scaling a box
 * that ends on a pixel does not add a row or a column.
 **/
constexpr float SNAP = 1.0F / 256.0F;

/**
 * The furthest a glyph's edge is converted to a pixel from the pen, in pixels.
 **/
constexpr float FURTHEST = 16777216.0F;

/**
 * The most samples taken across a pixel each way, when an image is shrunk.
 **/
constexpr int MOST_SAMPLES = 8;

/**
 * The colours four bytes of a key hold, from the highest.
 **/
constexpr std::uint32_t RED_SHIFT = 24;
constexpr std::uint32_t GREEN_SHIFT = 16;
constexpr std::uint32_t BLUE_SHIFT = 8;
constexpr std::uint32_t BYTE_MASK = 0xFFU;

/**
 * @return a value from 0 to 1, or 0 for one that is not a number
 **/
float unit(float value) {
    return std::isfinite(value) ? std::clamp(value, 0.0F, 1.0F) : 0.0F;
}

/**
 * @return a value from 0 to 1 as a byte
 **/
unsigned char toByte(float value) {
    return static_cast<unsigned char>(std::lround(unit(value) * BYTE));  // checked: unit() gives a finite value from 0 to 1
}

/**
 * Where a glyph is painted: a box of whole pixels, its lower left pixel at origin, with the pixel
 * grid's y upwards.
 **/
struct Grid {
    glm::ivec2 origin{ 0 };
    glm::ivec2 size{ 0 };
    float scale = 0.0F;  /**< pixels to the em **/
    float shift = 0.0F;  /**< how far the pen is moved right, in pixels **/

    std::size_t pixels() const {
        return static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y);
    }

    /**
     * @return the centre of a pixel, counted in rows from the top, in em units
     **/
    glm::vec2 em(int column, int row) const {
        return glm::vec2((static_cast<float>(origin.x + column) + 0.5F - shift) / scale, (static_cast<float>(origin.y + size.y - row) - 0.5F) / scale);
    }

    /**
     * @return a point in em units in FreeType's units from the box's lower left
     **/
    FT_Vector device(const glm::vec2& point) const {
        const glm::vec2 at = ((point * scale) + glm::vec2(shift, 0.0F) - glm::vec2(origin)) * FIXED_POINT;
        return FT_Vector{ .x = static_cast<FT_Pos>(std::lround(at.x)), .y = static_cast<FT_Pos>(std::lround(at.y)) };
    }
};

/**
 * @return how much of each pixel of the grid an outline covers, by the nonzero rule, from 0 to 1,
 *         in rows from the top
 **/
std::vector<float> coverage(FT_Library library, const std::vector<QuadraticCurve>& curves, const Grid& grid) {
    std::vector<float> covered(grid.pixels(), 0.0F);
    std::vector<FT_Vector> points;
    std::vector<unsigned char> tags;
    std::vector<std::uint16_t> ends;
    std::size_t first = 0;
    const auto end = [&points, &tags, &ends, &first]() {
        // FreeType closes a contour itself, so a last point back on the first is left off
        if (points.size() > first + 1 && points.back().x == points[first].x && points.back().y == points[first].y) {
            points.pop_back();
            tags.pop_back();
        }
        if (points.size() > first) {
            ends.push_back(static_cast<std::uint16_t>(points.size() - 1));
        }
        first = points.size();
    };
    for (std::size_t index = 0; index < curves.size(); index++) {
        const QuadraticCurve& curve = curves[index];
        if (index == 0 || curve.p1 != curves[index - 1].p3) {
            if (index > 0) {
                end();
            }
            points.push_back(grid.device(curve.p1));
            tags.push_back(FT_CURVE_TAG_ON);
        }
        if (curve.p2 != curve.p3 && curve.p2 != curve.p1) {
            points.push_back(grid.device(curve.p2));
            tags.push_back(FT_CURVE_TAG_CONIC);
        }
        points.push_back(grid.device(curve.p3));
        tags.push_back(FT_CURVE_TAG_ON);
    }
    end();
    if (ends.empty() || points.size() > std::numeric_limits<std::uint16_t>::max()) {
        return covered;
    }

    FT_Outline outline{ .n_contours = static_cast<std::uint16_t>(ends.size()), .n_points = static_cast<std::uint16_t>(points.size()),
        .points = points.data(), .tags = tags.data(), .contours = ends.data(), .flags = FT_OUTLINE_NONE };
    std::vector<unsigned char> bytes(grid.pixels(), 0);
    FT_Bitmap bitmap{};
    bitmap.rows = static_cast<unsigned int>(grid.size.y);
    bitmap.width = static_cast<unsigned int>(grid.size.x);
    bitmap.pitch = grid.size.x;
    bitmap.buffer = bytes.data();
    bitmap.num_grays = static_cast<std::uint16_t>(BYTE) + 1;
    bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
    if (FT_Outline_Get_Bitmap(library, &outline, &bitmap) != 0) {
        return covered;
    }
    std::transform(bytes.begin(), bytes.end(), covered.begin(), [](unsigned char value) { return static_cast<float>(value) / BYTE; });
    return covered;
}

/**
 * @return how much of each pixel is inside every one of a layer's clips
 **/
std::vector<float> mask(FT_Library library, const ColourLayer& layer, const Grid& grid) {
    std::vector<float> inside(grid.pixels(), 1.0F);
    for (const std::vector<QuadraticCurve>& clip : layer.clips) {
        const std::vector<float> covered = coverage(library, clip, grid);
        std::transform(inside.begin(), inside.end(), covered.begin(), inside.begin(), [](float kept, float clipped) { return kept * clipped; });
    }
    return inside;
}

/**
 * @return a stop's colour, premultiplied, in the text's colour when it asks for it
 **/
glm::vec4 premultiplied(const ColourStop& stop, const glm::vec4& foreground) {
    const glm::vec3 colour = stop.foreground ? glm::vec3(foreground) : glm::vec3(stop.colour);
    const float alpha = stop.colour.a * (stop.foreground ? foreground.a : 1.0F);
    return glm::vec4(colour * alpha, alpha);
}

/**
 * @return a place along a gradient brought inside its first and last stops, as it extends
 **/
float extended(const ColourFill& fill, float place) {
    const float low = fill.stops.front().offset;
    const float high = fill.stops.back().offset;
    if (high <= low) {
        return place;
    }
    const float span = high - low;
    const float along = (place - low) / span;
    switch (fill.extend) {
    case ColourExtend::Repeat:
        return low + ((along - std::floor(along)) * span);
    case ColourExtend::Reflect: {
        float folded = along - (2.0F * std::floor(along / 2.0F));
        folded = folded > 1.0F ? 2.0F - folded : folded;
        return low + (folded * span);
    }
    default:
        return std::clamp(place, low, high);
    }
}

/**
 * @return a gradient's colour at a place along it, premultiplied
 **/
glm::vec4 along(const ColourFill& fill, float place, const glm::vec4& foreground) {
    const float at = extended(fill, place);
    if (at <= fill.stops.front().offset) {
        return premultiplied(fill.stops.front(), foreground);
    }
    if (at >= fill.stops.back().offset) {
        return premultiplied(fill.stops.back(), foreground);
    }
    const auto next = std::find_if(fill.stops.begin(), fill.stops.end(), [at](const ColourStop& stop) { return stop.offset >= at; });
    const ColourStop& after = *next;
    const ColourStop& before = *(next - 1);
    const float gap = after.offset - before.offset;
    const float mix = gap > 0.0F ? (at - before.offset) / gap : 1.0F;
    return glm::mix(premultiplied(before, foreground), premultiplied(after, foreground), mix);
}

/**
 * @return where a point in a gradient's space is along it, or nothing where it paints nothing
 **/
std::optional<float> place(const ColourFill& fill, const glm::vec2& point) {
    switch (fill.kind) {
    case ColourFill::Kind::Linear: {
        const glm::vec2 run = fill.to - fill.from;
        const float length = glm::dot(run, run);
        if (length == 0.0F) {
            return std::nullopt;
        }
        return glm::dot(point - fill.from, run) / length;
    }
    case ColourFill::Kind::Radial: {
        // the largest t whose circle, centred from + t (to - from) with a radius that is not
        // negative, passes through the point: quadratic t^2 - 2 half t + constant = 0
        const glm::vec2 centres = fill.to - fill.from;
        const glm::vec2 offset = point - fill.from;
        const float grows = fill.toRadius - fill.fromRadius;
        const float quadratic = glm::dot(centres, centres) - (grows * grows);
        const float half = glm::dot(offset, centres) + (fill.fromRadius * grows);
        const float constant = glm::dot(offset, offset) - (fill.fromRadius * fill.fromRadius);
        const auto radius = [&fill, grows](float t) { return fill.fromRadius + (t * grows); };
        if (std::abs(quadratic) < std::numeric_limits<float>::epsilon()) {
            if (half == 0.0F) {
                return std::nullopt;
            }
            const float only = constant / (2.0F * half);
            return radius(only) >= 0.0F ? std::optional<float>(only) : std::nullopt;
        }
        const float discriminant = (half * half) - (quadratic * constant);
        if (discriminant < 0.0F) {
            return std::nullopt;
        }
        const float root = std::sqrt(discriminant);
        const float high = std::max((half + root) / quadratic, (half - root) / quadratic);
        const float low = std::min((half + root) / quadratic, (half - root) / quadratic);
        if (radius(high) >= 0.0F) {
            return high;
        }
        return radius(low) >= 0.0F ? std::optional<float>(low) : std::nullopt;
    }
    case ColourFill::Kind::Sweep: {
        const float span = fill.endAngle - fill.startAngle;
        if (span == 0.0F) {
            return std::nullopt;
        }
        float angle = std::atan2(point.y - fill.from.y, point.x - fill.from.x);
        angle = angle < 0.0F ? angle + (2.0F * std::numbers::pi_v<float>) : angle;
        return (angle - fill.startAngle) / span;
    }
    default:
        return 0.0F;
    }
}

/**
 * @return a fill's colour at a point in em units, premultiplied
 **/
glm::vec4 shade(const ColourFill& fill, const glm::mat3& inverse, const glm::vec2& point, const glm::vec4& foreground) {
    if (fill.stops.empty()) {
        return glm::vec4(0.0F);
    }
    if (fill.kind == ColourFill::Kind::Solid) {
        return premultiplied(fill.stops.front(), foreground);
    }
    const std::optional<float> at = place(fill, glm::vec2(inverse * glm::vec3(point, 1.0F)));
    return at ? along(fill, *at, foreground) : glm::vec4(0.0F);
}

/**
 * @return an image's texel, premultiplied
 **/
glm::vec4 texel(const ColourImage& image, int x, int y) {
    const int column = std::clamp(x, 0, static_cast<int>(image.size.x) - 1);
    const int row = std::clamp(y, 0, static_cast<int>(image.size.y) - 1);
    const unsigned char* at = image.pixels.data() + (((static_cast<std::size_t>(row) * image.size.x) + static_cast<std::size_t>(column)) * 4);
    const float alpha = static_cast<float>(at[3]) / BYTE;
    return glm::vec4(glm::vec3(static_cast<float>(at[0]), static_cast<float>(at[1]), static_cast<float>(at[2])) / BYTE * alpha, alpha);
}

/**
 * @return an image's colour at a point in its own space, mixed from the four texels around it,
 *         premultiplied, or nothing outside its box
 **/
glm::vec4 sample(const ColourImage& image, const glm::vec2& point) {
    const glm::vec2 box = image.max - image.min;
    if (box.x <= 0.0F || box.y <= 0.0F || point.x < image.min.x || point.x > image.max.x || point.y < image.min.y || point.y > image.max.y) {
        return glm::vec4(0.0F);
    }
    // texels count from the top left, and a texel's centre is half a texel in
    const glm::vec2 at = (glm::vec2(point.x - image.min.x, image.max.y - point.y) / box * glm::vec2(image.size)) - 0.5F;
    const glm::vec2 corner = glm::floor(at);
    const glm::vec2 mix = at - corner;
    const int x = static_cast<int>(corner.x);
    const int y = static_cast<int>(corner.y);
    const glm::vec4 top = glm::mix(texel(image, x, y), texel(image, x + 1, y), mix.x);
    const glm::vec4 bottom = glm::mix(texel(image, x, y + 1), texel(image, x + 1, y + 1), mix.x);
    return glm::mix(top, bottom, mix.y);
}

/**
 * @return an image's colour over a pixel centred on a point in em units: one sample, or the mean
 *         of a grid of them across the pixel where the image is shrunk
 **/
glm::vec4 cover(const ColourImage& image, const glm::mat3& inverse, int samples, const glm::vec2& point, float pixel) {
    glm::vec4 sum(0.0F);
    for (int y = 0; y < samples; y++) {
        for (int x = 0; x < samples; x++) {
            const glm::vec2 offset = ((glm::vec2(static_cast<float>(x), static_cast<float>(y)) + 0.5F) / static_cast<float>(samples)) - 0.5F;
            sum += sample(image, glm::vec2(inverse * glm::vec3(point + (offset * pixel), 1.0F)));
        }
    }
    return sum / static_cast<float>(samples * samples);
}

/**
 * @return how many samples are taken each way across a pixel, so each texel of a shrunk image
 *         is read about once
 **/
int samplesFor(const ColourImage& image, const glm::mat3& inverse, float pixel) {
    const glm::vec2 box = image.max - image.min;
    if (box.x <= 0.0F || box.y <= 0.0F) {
        return 1;
    }
    const glm::vec2 across = glm::vec2(inverse * glm::vec3(pixel, 0.0F, 0.0F));
    const glm::vec2 down = glm::vec2(inverse * glm::vec3(0.0F, pixel, 0.0F));
    const glm::vec2 texels = glm::vec2(image.size) / box;
    const float widest = std::max(glm::length(across * texels), glm::length(down * texels));
    if (!std::isfinite(widest)) {
        return 1;
    }
    const float samples = std::clamp(std::ceil(widest), 1.0F, static_cast<float>(MOST_SAMPLES));
    return static_cast<int>(samples);  // checked: finite, and clamped to 1 to MOST_SAMPLES
}

/**
 * Put a layer's colours down over a buffer, each pixel's colour taken where it is inside the
 * layer's clips.
 **/
template <typename Colour>
void lay(std::vector<glm::vec4>* buffer, const std::vector<float>& inside, const Grid& grid, const Colour& colourAt) {
    for (int row = 0; row < grid.size.y; row++) {
        for (int column = 0; column < grid.size.x; column++) {
            const std::size_t pixel = (static_cast<std::size_t>(row) * static_cast<std::size_t>(grid.size.x)) + static_cast<std::size_t>(column);
            if (inside[pixel] <= 0.0F) {
                continue;
            }
            const glm::vec4 painted = colourAt(grid.em(column, row)) * inside[pixel];
            (*buffer)[pixel] = painted + ((*buffer)[pixel] * (1.0F - painted.a));
        }
    }
}

};  // namespace

GlyphBitmap paint(const ColourGlyph& glyph, const Library& library, float size, std::uint8_t subpixel, const glm::vec4& foreground) {
    GlyphBitmap made{ .channels = 4 };
    FT_Library freetype = library.handle().library;
    if (glyph.empty() || !(size > 0.0F) || freetype == nullptr) {
        return made;
    }
    Grid grid{ .scale = size, .shift = static_cast<float>(subpixel % SUBPIXEL_STEPS) / static_cast<float>(SUBPIXEL_STEPS) };
    const glm::vec2 low = glm::floor((glyph.min * size) + glm::vec2(grid.shift + SNAP, SNAP));
    const glm::vec2 high = glm::ceil((glyph.max * size) + glm::vec2(grid.shift - SNAP, -SNAP));
    const auto within = [](float edge) { return std::isfinite(edge) && std::abs(edge) <= FURTHEST; };
    if (!within(low.x) || !within(low.y) || !within(high.x) || !within(high.y)) {
        return made;
    }
    const glm::ivec2 lowPixel(low);    // checked: finite and within FURTHEST, tested above
    const glm::ivec2 highPixel(high);  // checked: finite and within FURTHEST, tested above
    const int left = lowPixel.x;
    const int right = highPixel.x;
    const int bottom = lowPixel.y;
    const int top = highPixel.y;
    if (right - left > LARGEST || top - bottom > LARGEST) {
        return made;
    }
    grid.origin = glm::ivec2(left, bottom);
    grid.size = glm::ivec2(right - left, top - bottom);

    std::vector<std::vector<glm::vec4>> groups(1, std::vector<glm::vec4>(grid.pixels(), glm::vec4(0.0F)));
    const float pixel = 1.0F / size;
    for (const ColourLayer& layer : glyph.layers) {
        switch (layer.kind) {
        case ColourLayer::Kind::BeginGroup:
            groups.emplace_back(grid.pixels(), glm::vec4(0.0F));
            break;
        case ColourLayer::Kind::EndGroup:
            if (groups.size() > 1) {
                const std::vector<glm::vec4> gathered = std::move(groups.back());
                groups.pop_back();
                std::transform(gathered.begin(), gathered.end(), groups.back().begin(), groups.back().begin(),
                    [&layer](const glm::vec4& source, const glm::vec4& beneath) { return internal::composite(source, beneath, layer.composite); });
            }
            break;
        case ColourLayer::Kind::Image: {
            const ColourImage& image = glyph.images[layer.image];
            if (glm::determinant(image.transform) == 0.0F) {
                break;
            }
            const glm::mat3 inverse = glm::inverse(image.transform);
            const int samples = samplesFor(image, inverse, pixel);
            lay(&groups.back(), mask(freetype, layer, grid), grid,
                [&image, &inverse, pixel, samples](const glm::vec2& point) { return cover(image, inverse, samples, point, pixel); });
            break;
        }
        default: {
            if (glm::determinant(layer.fill.transform) == 0.0F) {
                break;
            }
            const glm::mat3 inverse = glm::inverse(layer.fill.transform);
            lay(&groups.back(), mask(freetype, layer, grid), grid,
                [&layer, &inverse, &foreground](const glm::vec2& point) { return shade(layer.fill, inverse, point, foreground); });
            break;
        }
        }
    }

    made.size = glm::uvec2(grid.size);
    made.bearing = glm::ivec2(left, -top);
    made.pixels.resize(grid.pixels() * 4);
    const std::vector<glm::vec4>& painted = groups.front();
    for (std::size_t index = 0; index < painted.size(); index++) {
        const float alpha = unit(painted[index].a);
        const glm::vec3 colour = alpha > 0.0F ? glm::vec3(painted[index]) / alpha : glm::vec3(0.0F);
        const glm::vec4 straight(unit(colour.r), unit(colour.g), unit(colour.b), alpha);
        for (int channel = 0; channel < 4; channel++) {
            made.pixels[(index * 4) + static_cast<std::size_t>(channel)] = toByte(straight[channel]);
        }
    }
    return made;
}

GlyphBitmap paint(const GlyphKey& key) {
    if (key.face == nullptr || key.mode != GlyphMode::Colour || !key.face->library()) {
        return GlyphBitmap{ .channels = 4 };
    }
    const ColourGlyph glyph = colour(*key.face, key.index, key.palette, key.size);
    return paint(glyph, *key.face->library(), key.size, key.subpixel, unpackColour(key.foreground));
}

std::uint32_t packColour(const glm::vec4& colour) {
    const auto channel = [](float value) { return static_cast<std::uint32_t>(toByte(unit(value))); };
    return (channel(colour.r) << RED_SHIFT) | (channel(colour.g) << GREEN_SHIFT) | (channel(colour.b) << BLUE_SHIFT) | channel(colour.a);
}

glm::vec4 unpackColour(std::uint32_t colour) {
    return glm::vec4(static_cast<float>((colour >> RED_SHIFT) & BYTE_MASK), static_cast<float>((colour >> GREEN_SHIFT) & BYTE_MASK),
        static_cast<float>((colour >> BLUE_SHIFT) & BYTE_MASK), static_cast<float>(colour & BYTE_MASK)) / BYTE;
}

};  // namespace v3d::font
