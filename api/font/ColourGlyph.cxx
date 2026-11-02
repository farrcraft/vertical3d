/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "ColourGlyph.h"

#include <api/font/internal/CurveBuilder.h>
#include <api/font/internal/FaceHandle.h>
#include <api/font/internal/LibraryHandle.h>
#include <api/font/internal/PaintWalk.h>
#include <api/font/internal/SvgReader.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/log/Logger.h>
#include <hb-ot.h>
#include <hb.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "Curves.h"
#include "Face.h"
#include "Library.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::font {

namespace {

/**
 * The most a colour channel holds, as HarfBuzz packs one into a byte.
 **/
constexpr float CHANNEL = 255.0F;

/**
 * How many times finer than its design units a glyph is walked. HarfBuzz gives a picture's box
 * in whole units of the font's scale, and a strike's pixel is rarely a whole design unit.
 **/
constexpr int FINER = 64;

/**
 * The largest size, in pixels to the em, a strike is chosen for.
 **/
constexpr float LARGEST_STRIKE = 4096.0F;

/**
 * The stops a colour line is read in at a time.
 **/
constexpr unsigned int STOPS_AT_ONCE = 16;

/**
 * @return a colour HarfBuzz packs, as red, green, blue and alpha from 0 to 1
 **/
glm::vec4 unpack(hb_color_t colour) {
    return glm::vec4(static_cast<float>(hb_color_get_red(colour)), static_cast<float>(hb_color_get_green(colour)),
        static_cast<float>(hb_color_get_blue(colour)), static_cast<float>(hb_color_get_alpha(colour))) / CHANNEL;
}

/**
 * @return the transform x' = xx x + xy y + dx, y' = yx x + yy y + dy
 **/
glm::mat3 affine(float xx, float yx, float xy, float yy, float dx, float dy) {
    return glm::mat3(glm::vec3(xx, yx, 0.0F), glm::vec3(xy, yy, 0.0F), glm::vec3(dx, dy, 1.0F));
}

// the draw functions build an outline through the builder they are given
void moveTo(hb_draw_funcs_t* /*funcs*/, void* data, hb_draw_state_t* /*state*/, float x, float y, void* /*user*/) {
    static_cast<internal::CurveBuilder*>(data)->moveTo(glm::vec2(x, y));
}

void lineTo(hb_draw_funcs_t* /*funcs*/, void* data, hb_draw_state_t* /*state*/, float x, float y, void* /*user*/) {
    static_cast<internal::CurveBuilder*>(data)->lineTo(glm::vec2(x, y));
}

void quadraticTo(hb_draw_funcs_t* /*funcs*/, void* data, hb_draw_state_t* /*state*/, float cx, float cy, float x, float y, void* /*user*/) {
    static_cast<internal::CurveBuilder*>(data)->quadraticTo(glm::vec2(cx, cy), glm::vec2(x, y));
}

void cubicTo(hb_draw_funcs_t* /*funcs*/, void* data, hb_draw_state_t* /*state*/, float c1x, float c1y, float c2x, float c2y, float x, float y,
    void* /*user*/) {
    static_cast<internal::CurveBuilder*>(data)->cubicTo(glm::vec2(c1x, c1y), glm::vec2(c2x, c2y), glm::vec2(x, y));
}

void closePath(hb_draw_funcs_t* /*funcs*/, void* data, hb_draw_state_t* /*state*/, void* /*user*/) {
    static_cast<internal::CurveBuilder*>(data)->close();
}

/**
 * @return the draw functions, made once and never changed
 **/
hb_draw_funcs_t* drawFuncs() {
    static const std::unique_ptr<hb_draw_funcs_t, decltype(&hb_draw_funcs_destroy)> funcs = [] {
        std::unique_ptr<hb_draw_funcs_t, decltype(&hb_draw_funcs_destroy)> made(hb_draw_funcs_create(), &hb_draw_funcs_destroy);
        hb_draw_funcs_set_move_to_func(made.get(), moveTo, nullptr, nullptr);
        hb_draw_funcs_set_line_to_func(made.get(), lineTo, nullptr, nullptr);
        hb_draw_funcs_set_quadratic_to_func(made.get(), quadraticTo, nullptr, nullptr);
        hb_draw_funcs_set_cubic_to_func(made.get(), cubicTo, nullptr, nullptr);
        hb_draw_funcs_set_close_path_func(made.get(), closePath, nullptr, nullptr);
        hb_draw_funcs_make_immutable(made.get());
        return made;
    }();
    return funcs.get();
}

void pushTransform(hb_paint_funcs_t* /*funcs*/, void* data, float xx, float yx, float xy, float yy, float dx, float dy, void* /*user*/) {
    internal::PaintWalk* walk = static_cast<internal::PaintWalk*>(data);
    walk->transforms.push_back(walk->transforms.back() * affine(xx, yx, xy, yy, dx, dy));
}

void popTransform(hb_paint_funcs_t* /*funcs*/, void* data, void* /*user*/) {
    internal::PaintWalk* walk = static_cast<internal::PaintWalk*>(data);
    if (walk->transforms.size() > 1) {
        walk->transforms.pop_back();
    }
}

void pushClipGlyph(hb_paint_funcs_t* /*funcs*/, void* data, hb_codepoint_t glyph, hb_font_t* font, void* /*user*/) {
    internal::PaintWalk* walk = static_cast<internal::PaintWalk*>(data);
    internal::CurveBuilder builder(walk->current());
    hb_font_draw_glyph_or_fail(font, glyph, drawFuncs(), &builder);
    walk->clips.push_back(ColourClip{ .curves = builder.take() });
}

void pushClipRectangle(hb_paint_funcs_t* /*funcs*/, void* data, float xmin, float ymin, float xmax, float ymax, void* /*user*/) {
    internal::PaintWalk* walk = static_cast<internal::PaintWalk*>(data);
    internal::CurveBuilder builder(walk->current());
    builder.moveTo(glm::vec2(xmin, ymin));
    builder.lineTo(glm::vec2(xmin, ymax));
    builder.lineTo(glm::vec2(xmax, ymax));
    builder.lineTo(glm::vec2(xmax, ymin));
    walk->clips.push_back(ColourClip{ .curves = builder.take() });
}

hb_draw_funcs_t* pushClipPathStart(hb_paint_funcs_t* /*funcs*/, void* data, void** drawData, void* /*user*/) {
    internal::PaintWalk* walk = static_cast<internal::PaintWalk*>(data);
    walk->path.emplace(walk->current());
    *drawData = &*walk->path;
    return drawFuncs();
}

void pushClipPathEnd(hb_paint_funcs_t* /*funcs*/, void* data, void* /*user*/) {
    internal::PaintWalk* walk = static_cast<internal::PaintWalk*>(data);
    walk->clips.push_back(ColourClip{ .curves = walk->path ? walk->path->take() : std::vector<QuadraticCurve>() });
    walk->path.reset();
}

void popClip(hb_paint_funcs_t* /*funcs*/, void* data, void* /*user*/) {
    internal::PaintWalk* walk = static_cast<internal::PaintWalk*>(data);
    if (!walk->clips.empty()) {
        walk->clips.pop_back();
    }
}

void solid(hb_paint_funcs_t* /*funcs*/, void* data, hb_bool_t foreground, hb_color_t colour, void* /*user*/) {
    ColourFill fill;
    fill.stops.push_back(ColourStop{ .offset = 0.0F, .colour = unpack(colour), .foreground = foreground != 0 });
    static_cast<internal::PaintWalk*>(data)->fill(std::move(fill));
}

/**
 * @return a gradient's stops in order of offset, and how it extends past them
 **/
ColourFill gradient(const internal::PaintWalk& walk, hb_color_line_t* line, ColourFill::Kind kind) {
    ColourFill fill{ .kind = kind, .transform = walk.current() };
    std::array<hb_color_stop_t, STOPS_AT_ONCE> read{};
    unsigned int start = 0;
    unsigned int count = STOPS_AT_ONCE;
    const unsigned int total = hb_color_line_get_color_stops(line, 0, nullptr, nullptr);
    while (start < total) {
        count = STOPS_AT_ONCE;
        hb_color_line_get_color_stops(line, start, &count, read.data());
        if (count == 0) {
            break;
        }
        for (unsigned int stop = 0; stop < count; stop++) {
            fill.stops.push_back(ColourStop{ .offset = read[stop].offset, .colour = unpack(read[stop].color), .foreground = read[stop].is_foreground != 0 });
        }
        start += count;
    }
    std::stable_sort(fill.stops.begin(), fill.stops.end(), [](const ColourStop& one, const ColourStop& other) { return one.offset < other.offset; });
    switch (hb_color_line_get_extend(line)) {
    case HB_PAINT_EXTEND_REPEAT:
        fill.extend = ColourExtend::Repeat;
        break;
    case HB_PAINT_EXTEND_REFLECT:
        fill.extend = ColourExtend::Reflect;
        break;
    default:
        fill.extend = ColourExtend::Pad;
        break;
    }
    return fill;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - HarfBuzz fixes the parameters
void linear(hb_paint_funcs_t* /*funcs*/, void* data, hb_color_line_t* line, float x0, float y0, float x1, float y1, float x2, float y2, void* /*user*/) {
    internal::PaintWalk* walk = static_cast<internal::PaintWalk*>(data);
    ColourFill fill = gradient(*walk, line, ColourFill::Kind::Linear);
    // the third point turns the gradient's lines of equal colour, which run from the first point
    // towards it. The end the gradient runs to is the second point moved along those lines until
    // it is straight across them from the first
    const glm::vec2 start(x0, y0);
    const glm::vec2 across = glm::vec2(x2, y2) - start;
    const glm::vec2 end(x1, y1);
    const float length = glm::dot(across, across);
    const float along = length == 0.0F ? 0.0F : glm::dot(across, end - start) / length;
    fill.from = start;
    fill.to = end - (along * across);
    walk->fill(std::move(fill));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - HarfBuzz fixes the parameters
void radial(hb_paint_funcs_t* /*funcs*/, void* data, hb_color_line_t* line, float x0, float y0, float r0, float x1, float y1, float r1, void* /*user*/) {
    internal::PaintWalk* walk = static_cast<internal::PaintWalk*>(data);
    ColourFill fill = gradient(*walk, line, ColourFill::Kind::Radial);
    fill.from = glm::vec2(x0, y0);
    fill.fromRadius = r0;
    fill.to = glm::vec2(x1, y1);
    fill.toRadius = r1;
    walk->fill(std::move(fill));
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - HarfBuzz fixes the parameters
void sweep(hb_paint_funcs_t* /*funcs*/, void* data, hb_color_line_t* line, float x, float y, float start, float end, void* /*user*/) {
    internal::PaintWalk* walk = static_cast<internal::PaintWalk*>(data);
    ColourFill fill = gradient(*walk, line, ColourFill::Kind::Sweep);
    fill.from = glm::vec2(x, y);
    fill.startAngle = start;
    fill.endAngle = end;
    walk->fill(std::move(fill));
}

/**
 * @return an image's pixels from a PNG, as red, green, blue and alpha, or none
 **/
std::optional<ColourImage> decodePng(hb_blob_t* blob) {
    unsigned int length = 0;
    const char* bytes = hb_blob_get_data(blob, &length);
    // a picture that will not decode is logged to the shared log, as any other image is
    image::reader::Png reader(boost::make_shared<log::Logger>());
    const boost::shared_ptr<image::Image> decoded = reader.read(reinterpret_cast<const unsigned char*>(bytes), length);
    if (!decoded || decoded->width() == 0 || decoded->height() == 0) {
        return std::nullopt;
    }
    ColourImage made{ .size = glm::uvec2(decoded->width(), decoded->height()) };
    const std::size_t pixels = static_cast<std::size_t>(made.size.x) * made.size.y;
    made.pixels.resize(pixels * 4);
    const auto channels = static_cast<std::size_t>(decoded->format());
    const unsigned char* from = decoded->data();
    for (std::size_t pixel = 0; pixel < pixels; pixel++) {
        const unsigned char* texel = from + (pixel * channels);
        unsigned char* to = made.pixels.data() + (pixel * 4);
        // a grey picture is decoded as red, green and blue, so one channel is never seen here
        to[0] = texel[0];
        to[1] = channels >= 3 ? texel[1] : texel[0];
        to[2] = channels >= 3 ? texel[2] : texel[0];
        to[3] = channels == 4 ? texel[3] : std::numeric_limits<unsigned char>::max();
    }
    return made;
}

/**
 * @return an image's pixels from blue, green, red and alpha, premultiplied, as red, green, blue
 *         and alpha, not
 **/
ColourImage decodeBgra(hb_blob_t* blob, unsigned int width, unsigned int height) {
    unsigned int length = 0;
    const auto* bytes = reinterpret_cast<const unsigned char*>(hb_blob_get_data(blob, &length));
    ColourImage made{ .size = glm::uvec2(width, height) };
    const std::size_t pixels = static_cast<std::size_t>(width) * height;
    if (length < pixels * 4) {
        return ColourImage();
    }
    made.pixels.resize(pixels * 4);
    for (std::size_t pixel = 0; pixel < pixels; pixel++) {
        const unsigned char* texel = bytes + (pixel * 4);
        unsigned char* to = made.pixels.data() + (pixel * 4);
        const unsigned int alpha = texel[3];
        const auto straight = [alpha](unsigned char value) {
            return alpha == 0 ? static_cast<unsigned char>(0) : static_cast<unsigned char>(std::min(255U, ((value * 255U) + (alpha / 2)) / alpha));
        };
        to[0] = straight(texel[2]);
        to[1] = straight(texel[1]);
        to[2] = straight(texel[0]);
        to[3] = texel[3];
    }
    return made;
}

hb_bool_t image(hb_paint_funcs_t* /*funcs*/, void* data, hb_blob_t* blob, unsigned int width, unsigned int height, hb_tag_t format, float /*slant*/,
    hb_glyph_extents_t* extents, void* /*user*/) {
    internal::PaintWalk* walk = static_cast<internal::PaintWalk*>(data);
    if (format == HB_PAINT_IMAGE_FORMAT_SVG) {
        // a document's units are design units with y downwards
        unsigned int length = 0;
        const char* text = hb_blob_get_data(blob, &length);
        glm::mat3 flip(1.0F);
        flip[0][0] = walk->finer;
        flip[1][1] = -walk->finer;
        const internal::SvgDocument document{ .text = std::string_view(text, length), .unitsPerEm = walk->units, .memory = walk->memory };
        return internal::SvgReader::read(document, walk->index, walk->current() * flip, walk->library, &walk->glyph) ? 1 : 0;
    }
    std::optional<ColourImage> made;
    if (format == HB_PAINT_IMAGE_FORMAT_PNG) {
        made = decodePng(blob);
    } else if (format == HB_PAINT_IMAGE_FORMAT_BGRA && width > 0 && height > 0) {
        made = decodeBgra(blob, width, height);
    }
    if (!made || made->pixels.empty() || extents == nullptr) {
        return 0;
    }
    // the extents are the box the picture fills: from its left and its top, its width rightwards
    // and its height, which is negative, downwards
    made->min = glm::vec2(static_cast<float>(extents->x_bearing), static_cast<float>(extents->y_bearing + extents->height));
    made->max = glm::vec2(static_cast<float>(extents->x_bearing + extents->width), static_cast<float>(extents->y_bearing));
    // a strike's picture starts and ends on its own pixels, which HarfBuzz may round to design
    // units, so its box is put back on them
    const glm::vec2 texel = (made->max - made->min) / glm::vec2(made->size);
    if (texel.x > 0.0F && texel.y > 0.0F) {
        made->min = glm::round(made->min / texel) * texel;
        made->max = made->min + (texel * glm::vec2(made->size));
    }
    made->transform = walk->current();
    ColourLayer layer{ .kind = ColourLayer::Kind::Image, .clips = walk->clips, .image = walk->glyph.images.size() };
    walk->glyph.images.push_back(std::move(*made));
    walk->glyph.layers.push_back(std::move(layer));
    return 1;
}

void pushGroup(hb_paint_funcs_t* /*funcs*/, void* data, void* /*user*/) {
    static_cast<internal::PaintWalk*>(data)->glyph.layers.push_back(ColourLayer{ .kind = ColourLayer::Kind::BeginGroup });
}

void popGroup(hb_paint_funcs_t* /*funcs*/, void* data, hb_paint_composite_mode_t mode, void* /*user*/) {
    // the modes are numbered as HarfBuzz numbers them
    const auto composite = static_cast<int>(mode) <= static_cast<int>(ColourComposite::Luminosity) ? static_cast<ColourComposite>(mode)
                                                                                                    : ColourComposite::SourceOver;
    static_cast<internal::PaintWalk*>(data)->glyph.layers.push_back(ColourLayer{ .kind = ColourLayer::Kind::EndGroup, .composite = composite });
}

/**
 * @return the paint functions, made once and never changed
 **/
hb_paint_funcs_t* paintFuncs() {
    static const std::unique_ptr<hb_paint_funcs_t, decltype(&hb_paint_funcs_destroy)> funcs = [] {
        std::unique_ptr<hb_paint_funcs_t, decltype(&hb_paint_funcs_destroy)> made(hb_paint_funcs_create(), &hb_paint_funcs_destroy);
        hb_paint_funcs_t* set = made.get();
        hb_paint_funcs_set_push_transform_func(set, pushTransform, nullptr, nullptr);
        hb_paint_funcs_set_pop_transform_func(set, popTransform, nullptr, nullptr);
        hb_paint_funcs_set_push_clip_glyph_func(set, pushClipGlyph, nullptr, nullptr);
        hb_paint_funcs_set_push_clip_rectangle_func(set, pushClipRectangle, nullptr, nullptr);
        hb_paint_funcs_set_push_clip_path_start_func(set, pushClipPathStart, nullptr, nullptr);
        hb_paint_funcs_set_push_clip_path_end_func(set, pushClipPathEnd, nullptr, nullptr);
        hb_paint_funcs_set_pop_clip_func(set, popClip, nullptr, nullptr);
        hb_paint_funcs_set_color_func(set, solid, nullptr, nullptr);
        hb_paint_funcs_set_linear_gradient_func(set, linear, nullptr, nullptr);
        hb_paint_funcs_set_radial_gradient_func(set, radial, nullptr, nullptr);
        hb_paint_funcs_set_sweep_gradient_func(set, sweep, nullptr, nullptr);
        hb_paint_funcs_set_image_func(set, image, nullptr, nullptr);
        hb_paint_funcs_set_push_group_func(set, pushGroup, nullptr, nullptr);
        hb_paint_funcs_set_pop_group_func(set, popGroup, nullptr, nullptr);
        hb_paint_funcs_make_immutable(set);
        return made;
    }();
    return funcs.get();
}

/**
 * Set a glyph's box to hold everything it draws: each fill within the box shared by its clips,
 * and each image within its corners.
 **/
void bound(ColourGlyph* glyph) {
    internal::PaintWalk::Box drawn;
    for (const ColourLayer& layer : glyph->layers) {
        if (layer.kind == ColourLayer::Kind::Image) {
            const ColourImage& placed = glyph->images[layer.image];
            for (const glm::vec2& corner : { placed.min, glm::vec2(placed.max.x, placed.min.y), placed.max, glm::vec2(placed.min.x, placed.max.y) }) {
                drawn.hold(glm::vec2(placed.transform * glm::vec3(corner, 1.0F)));
            }
        } else if (layer.kind == ColourLayer::Kind::Fill && !layer.clips.empty()) {
            glm::vec2 low(-std::numeric_limits<float>::max());
            glm::vec2 high(std::numeric_limits<float>::max());
            for (const ColourClip& clip : layer.clips) {
                internal::PaintWalk::Box outline;
                for (const QuadraticCurve& curve : clip.curves) {
                    for (const glm::vec2& point : { curve.p1, curve.p2, curve.p3 }) {
                        outline.hold(point);
                    }
                }
                low = glm::max(low, outline.min);
                high = glm::min(high, outline.max);
            }
            if (low.x < high.x && low.y < high.y) {
                drawn.hold(low);
                drawn.hold(high);
            }
        }
    }
    glyph->min = drawn.min;
    glyph->max = drawn.max;
}

};  // namespace

bool ColourTables::any() const noexcept {
    return layers || paint || svg || cbdt || sbix;
}

bool ColourGlyph::empty() const noexcept {
    return min.x >= max.x || min.y >= max.y;
}

bool ColourGlyph::foreground() const noexcept {
    return std::any_of(layers.begin(), layers.end(), [](const ColourLayer& layer) {
        return std::any_of(layer.fill.stops.begin(), layer.fill.stops.end(), [](const ColourStop& stop) { return stop.foreground; });
    });
}

// a glyph's index, then its palette, as GlyphKey orders them
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
ColourGlyph colour(const Face& face, unsigned int index, unsigned int palette, float size) {
    hb_face_t* shaping = face.handle().shaping;
    if (shaping == nullptr || !face.coloured(index)) {
        return ColourGlyph();
    }
    const std::unique_ptr<hb_font_t, decltype(&hb_font_destroy)> font(hb_font_create(shaping), &hb_font_destroy);
    const int units = static_cast<int>(hb_face_get_upem(shaping)) * FINER;
    hb_font_set_scale(font.get(), units, units);
    const float strike = std::isfinite(size) ? std::clamp(size, 1.0F, LARGEST_STRIKE) : 1.0F;
    const auto ppem = static_cast<unsigned int>(strike);  // checked: finite, and clamped to 1 to LARGEST_STRIKE
    hb_font_set_ppem(font.get(), ppem, ppem);

    internal::PaintWalk walk{ .library = face.library() ? face.library()->handle().library : nullptr,
        .memory = face.handle().face != nullptr ? face.handle().face->memory : nullptr,
        .index = index,
        .units = hb_face_get_upem(shaping),
        .finer = static_cast<float>(FINER) };
    walk.toEm = glm::mat3(1.0F / static_cast<float>(std::max(units, 1)));
    walk.toEm[2][2] = 1.0F;
    const unsigned int palettes = hb_ot_color_palette_get_count(shaping);
    // an opaque white foreground leaves each colour's alpha what the paint made it
    if (hb_font_paint_glyph_or_fail(font.get(), index, paintFuncs(), &walk, palette < palettes ? palette : 0, HB_COLOR(255, 255, 255, 255)) == 0) {
        return ColourGlyph();
    }
    bound(&walk.glyph);
    return std::move(walk.glyph);
}

};  // namespace v3d::font
