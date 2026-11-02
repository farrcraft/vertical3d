/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SvgReader.h"

#include <api/font/ColourGlyph.h>
#include <api/font/Curves.h>
#include <api/font/internal/CurveBuilder.h>
#include <api/font/internal/OutlineHandle.h>
#include <api/font/internal/OutlineReader.h>
#include <api/font/internal/StrokerHandle.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GZIP_H
#include FT_OUTLINE_H
#include FT_STROKER_H

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::font::internal {

bool SvgReader::started() {
    static const Started once;  // process-wide: ThorVG registers its loaders once for the process
    return once.started();
}

glm::mat3 SvgReader::matrixOf(const tvg::Matrix& matrix) {
    return glm::mat3(glm::vec3(matrix.e11, matrix.e21, matrix.e31), glm::vec3(matrix.e12, matrix.e22, matrix.e32),
        glm::vec3(matrix.e13, matrix.e23, matrix.e33));
}

glm::mat3 SvgReader::transformOf(const tvg::Paint* paint) {
    // ThorVG gives a paint's transform only through a non-const accessor
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return matrixOf(const_cast<tvg::Paint*>(paint)->transform());
}

std::vector<QuadraticCurve> SvgReader::outlineOf(const tvg::Shape& shape, const glm::mat3& toEm) {
    const Path path(shape);
    CurveBuilder builder(toEm);
    std::uint32_t point = 0;
    for (std::uint32_t command = 0; command < path.commandCount; command++) {
        const tvg::PathCommand next = path.commands[command];
        if (point + Path::takes(next) > path.pointCount) {
            break;
        }
        switch (next) {
        case tvg::PathCommand::MoveTo:
            builder.moveTo(path.at(point));
            break;
        case tvg::PathCommand::LineTo:
            builder.lineTo(path.at(point));
            break;
        case tvg::PathCommand::CubicTo:
            builder.cubicTo(path.at(point), path.at(point + 1), path.at(point + 2));
            break;
        default:
            builder.close();
            break;
        }
        point += Path::takes(next);
    }
    return builder.take();
}

FT_Vector SvgReader::fixed(const glm::vec2& point) {
    // the stroker works in the shape's own units, which a glyph keeps within a few thousand
    return FT_Vector{ .x = static_cast<FT_Pos>(std::lround(point.x * FIXED_POINT)), .y = static_cast<FT_Pos>(std::lround(point.y * FIXED_POINT)) };
}

std::vector<QuadraticCurve> SvgReader::strokeOf(const tvg::Shape& shape, const glm::mat3& toEm, FT_Library library) {
    const float width = shape.strokeWidth();
    FT_Stroker raw = nullptr;
    if (!(width > 0.0F) || !std::isfinite(width) || FT_Stroker_New(library, &raw) != 0) {
        return std::vector<QuadraticCurve>();
    }
    const StrokerHandle stroker(raw);
    FT_Stroker_Set(stroker.get(), static_cast<FT_Fixed>(std::lround(width * FIXED_POINT / 2.0F)), capOf(shape.strokeCap()), joinOf(shape.strokeJoin()),
        static_cast<FT_Fixed>(std::lround(std::max(1.0F, shape.strokeMiterlimit()) * FIXED_16)));
    trace(Path(shape), stroker.get());

    FT_UInt points = 0;
    FT_UInt contours = 0;
    OutlineHandle outline(library);
    if (FT_Stroker_GetCounts(stroker.get(), &points, &contours) != 0 || points == 0 || !outline.make(points, static_cast<FT_Int>(contours))) {
        return std::vector<QuadraticCurve>();
    }
    outline.get()->n_points = 0;
    outline.get()->n_contours = 0;
    FT_Stroker_Export(stroker.get(), outline.get());
    glm::mat3 unfixed(1.0F / FIXED_POINT);
    unfixed[2][2] = 1.0F;
    OutlineReader reader(toEm * unfixed);
    return reader.read(outline.get());
}

FT_Stroker_LineCap SvgReader::capOf(tvg::StrokeCap cap) {
    switch (cap) {
    case tvg::StrokeCap::Butt:
        return FT_STROKER_LINECAP_BUTT;
    case tvg::StrokeCap::Round:
        return FT_STROKER_LINECAP_ROUND;
    default:
        return FT_STROKER_LINECAP_SQUARE;
    }
}

FT_Stroker_LineJoin SvgReader::joinOf(tvg::StrokeJoin join) {
    switch (join) {
    case tvg::StrokeJoin::Miter:
        return FT_STROKER_LINEJOIN_MITER;
    case tvg::StrokeJoin::Round:
        return FT_STROKER_LINEJOIN_ROUND;
    default:
        return FT_STROKER_LINEJOIN_BEVEL;
    }
}

void SvgReader::trace(const Path& path, FT_Stroker stroker) {
    std::uint32_t command = 0;
    std::uint32_t point = 0;
    while (command < path.commandCount) {
        if (path.commands[command] != tvg::PathCommand::MoveTo || point >= path.pointCount) {
            point += Path::takes(path.commands[command]);
            command++;
            continue;
        }
        std::uint32_t end = command + 1;
        while (end < path.commandCount && path.commands[end] != tvg::PathCommand::MoveTo && path.commands[end] != tvg::PathCommand::Close) {
            end++;
        }
        const bool closed = end < path.commandCount && path.commands[end] == tvg::PathCommand::Close;
        FT_Vector start = fixed(path.at(point));
        FT_Stroker_BeginSubPath(stroker, &start, closed ? 0 : 1);
        point += 1;
        for (std::uint32_t step = command + 1; step < end && point + Path::takes(path.commands[step]) <= path.pointCount; step++) {
            if (path.commands[step] == tvg::PathCommand::LineTo) {
                FT_Vector to = fixed(path.at(point));
                FT_Stroker_LineTo(stroker, &to);
            } else if (path.commands[step] == tvg::PathCommand::CubicTo) {
                FT_Vector first = fixed(path.at(point));
                FT_Vector second = fixed(path.at(point + 1));
                FT_Vector to = fixed(path.at(point + 2));
                FT_Stroker_CubicTo(stroker, &first, &second, &to);
            }
            point += Path::takes(path.commands[step]);
        }
        FT_Stroker_EndSubPath(stroker);
        command = closed ? end + 1 : end;
    }
}

ColourFill SvgReader::solidFill(const glm::u8vec4& bytes, float opacity) {
    ColourFill fill;
    const glm::vec4 colour = glm::vec4(bytes) / CHANNEL;
    fill.stops.push_back(ColourStop{ .colour = glm::vec4(glm::vec3(colour), colour.a * opacity) });
    return fill;
}

std::optional<ColourFill> SvgReader::gradientFill(const tvg::Fill& gradient, const glm::mat3& toEm, float opacity) {
    ColourFill fill{ .transform = toEm * matrixOf(gradient.transform()) };
    if (gradient.type() == tvg::Type::LinearGradient) {
        glm::vec2 from(0.0F);
        glm::vec2 to(0.0F);
        static_cast<const tvg::LinearGradient&>(gradient).linear(&from.x, &from.y, &to.x, &to.y);
        fill.kind = ColourFill::Kind::Linear;
        fill.from = from;
        fill.to = to;
    } else if (gradient.type() == tvg::Type::RadialGradient) {
        glm::vec2 centre(0.0F);
        glm::vec2 focus(0.0F);
        float radius = 0.0F;
        float focusRadius = 0.0F;
        static_cast<const tvg::RadialGradient&>(gradient).radial(&centre.x, &centre.y, &radius, &focus.x, &focus.y, &focusRadius);
        // SVG's gradient runs out from its focal circle to its outer one
        fill.kind = ColourFill::Kind::Radial;
        fill.from = focus;
        fill.fromRadius = focusRadius;
        fill.to = centre;
        fill.toRadius = radius;
    } else {
        return std::nullopt;
    }
    const tvg::Fill::ColorStop* stops = nullptr;
    const std::uint32_t count = gradient.colorStops(&stops);
    for (std::uint32_t stop = 0; stop < count; stop++) {
        const glm::vec4 colour = glm::vec4(static_cast<float>(stops[stop].r), static_cast<float>(stops[stop].g), static_cast<float>(stops[stop].b),
                                     static_cast<float>(stops[stop].a)) / CHANNEL;
        fill.stops.push_back(ColourStop{ .offset = stops[stop].offset, .colour = glm::vec4(glm::vec3(colour), colour.a * opacity) });
    }
    if (fill.stops.empty()) {
        return std::nullopt;
    }
    switch (gradient.spread()) {
    case tvg::FillSpread::Reflect:
        fill.extend = ColourExtend::Reflect;
        break;
    case tvg::FillSpread::Repeat:
        fill.extend = ColourExtend::Repeat;
        break;
    default:
        fill.extend = ColourExtend::Pad;
        break;
    }
    return fill;
}

void SvgReader::addLayer(const std::vector<ColourClip>& clips, ColourClip&& outline, std::optional<ColourFill>&& fill, Reading* reading) {
    if (!fill || outline.curves.empty()) {
        return;
    }
    ColourLayer layer{ .kind = ColourLayer::Kind::Fill, .clips = clips };
    layer.clips.push_back(std::move(outline));
    layer.fill = std::move(*fill);
    reading->glyph->layers.push_back(std::move(layer));
}

void SvgReader::addShape(const tvg::Shape& shape, const glm::mat3& toEm, float opacity, const std::vector<ColourClip>& clips, Reading* reading) {
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;
    std::uint8_t alpha = 0;
    const tvg::Fill* gradient = shape.fill();
    std::optional<ColourFill> fill;
    if (gradient != nullptr) {
        fill = gradientFill(*gradient, toEm, opacity);
    } else if (shape.fill(&red, &green, &blue, &alpha) == tvg::Result::Success && alpha > 0) {
        fill = solidFill(glm::u8vec4(red, green, blue, alpha), opacity);
    }
    if (fill) {
        addLayer(clips, ColourClip{ .curves = outlineOf(shape, toEm), .evenOdd = shape.fillRule() == tvg::FillRule::EvenOdd }, std::move(fill), reading);
    }

    const tvg::Fill* strokeGradient = shape.strokeFill();
    std::optional<ColourFill> stroke;
    if (strokeGradient != nullptr) {
        stroke = gradientFill(*strokeGradient, toEm, opacity);
    } else if (shape.strokeFill(&red, &green, &blue, &alpha) == tvg::Result::Success && alpha > 0) {
        stroke = solidFill(glm::u8vec4(red, green, blue, alpha), opacity);
    }
    if (stroke) {
        addLayer(clips, ColourClip{ .curves = strokeOf(shape, toEm, reading->library) }, std::move(stroke), reading);
    }
}

// NOLINTNEXTLINE(misc-no-recursion) - a scene holds paints, which may be scenes
void SvgReader::addPaint(const tvg::Paint* paint, const glm::mat3& above, float opacity, std::vector<ColourClip> clips, Reading* reading) {
    if (paint == nullptr || !paint->visible()) {
        return;
    }
    const glm::mat3 toEm = above * transformOf(paint);
    const float faded = opacity * static_cast<float>(paint->opacity()) / CHANNEL;
    if (!(faded > 0.0F)) {
        return;
    }
    if (const tvg::Shape* clip = paint->clip(); clip != nullptr) {
        clips.push_back(ColourClip{ .curves = outlineOf(*clip, toEm * transformOf(clip)), .evenOdd = clip->fillRule() == tvg::FillRule::EvenOdd });
    }
    const tvg::Paint* mask = nullptr;
    const tvg::MaskMethod method = paint->mask(&mask);
    const bool masked = mask != nullptr && method != tvg::MaskMethod::None;
    if (masked) {
        reading->glyph->layers.push_back(ColourLayer{ .kind = ColourLayer::Kind::BeginGroup });
    }

    if (paint->type() == tvg::Type::Shape) {
        addShape(static_cast<const tvg::Shape&>(*paint), toEm, faded, clips, reading);
    } else if (paint->type() == tvg::Type::Scene) {
        for (const tvg::Paint* child : static_cast<const tvg::Scene&>(*paint).paints()) {
            addPaint(child, toEm, faded, clips, reading);
        }
    }

    if (masked) {
        // the mask's own alpha keeps what is beneath it, or with an inverse mask removes it
        const bool inverse = method == tvg::MaskMethod::InvAlpha || method == tvg::MaskMethod::InvLuma;
        reading->glyph->layers.push_back(ColourLayer{ .kind = ColourLayer::Kind::BeginGroup });
        addPaint(mask, toEm, 1.0F, std::vector<ColourClip>(), reading);
        reading->glyph->layers.push_back(ColourLayer{ .kind = ColourLayer::Kind::EndGroup,
            .composite = inverse ? ColourComposite::DestinationOut : ColourComposite::DestinationIn });
        reading->glyph->layers.push_back(ColourLayer{ .kind = ColourLayer::Kind::EndGroup });
    }
}

std::optional<std::string> SvgReader::inflated(const SvgDocument& document) {
    const std::string_view text = document.text;
    const auto* bytes = reinterpret_cast<const unsigned char*>(text.data());
    if (text.size() < GZIP.size() + 4 || !std::equal(GZIP.begin(), GZIP.end(), bytes)) {
        return std::string(text);
    }
    if (document.memory == nullptr) {
        return std::nullopt;
    }
    // gzip ends with the inflated length, little-endian
    const unsigned char* end = bytes + text.size() - 4;
    const std::uint32_t length = static_cast<std::uint32_t>(end[0]) | (static_cast<std::uint32_t>(end[1]) << 8U) | (static_cast<std::uint32_t>(end[2]) << 16U)
        | (static_cast<std::uint32_t>(end[3]) << 24U);
    if (length == 0 || length > LARGEST_DOCUMENT) {
        return std::nullopt;
    }
    std::string out(length, '\0');
    FT_ULong written = length;
    if (FT_Gzip_Uncompress(document.memory, reinterpret_cast<FT_Byte*>(out.data()), &written, bytes, static_cast<FT_ULong>(text.size())) != 0) {
        return std::nullopt;
    }
    out.resize(written);
    return out;
}

std::string SvgReader::withViewport(const std::string& text, unsigned int unitsPerEm) {
    std::size_t root = text.find("<svg");
    while (root != std::string::npos && root + 4 < text.size() && std::isspace(static_cast<unsigned char>(text[root + 4])) == 0 && text[root + 4] != '>') {
        root = text.find("<svg", root + 4);
    }
    const std::size_t close = root == std::string::npos ? std::string::npos : text.find('>', root);
    if (close == std::string::npos || unitsPerEm == 0) {
        return text;
    }
    const std::string_view tag(text.data() + root, close - root);
    const auto has = [&tag](std::string_view attribute) {
        for (std::size_t at = tag.find(attribute); at != std::string_view::npos; at = tag.find(attribute, at + 1)) {
            if (at > 0 && std::isspace(static_cast<unsigned char>(tag[at - 1])) != 0) {
                return true;
            }
        }
        return false;
    };
    const std::string units = std::to_string(unitsPerEm);
    std::string added;
    if (!has("width=")) {
        added += " width=\"" + units + "\"";
    }
    if (!has("height=")) {
        added += " height=\"" + units + "\"";
    }
    if (!has("viewBox=")) {
        added += " viewBox=\"0 0 " + units + " " + units + "\"";
    }
    // a root that closes itself has nothing to draw, but keeps its slash
    const std::size_t at = close > root && text[close - 1] == '/' ? close - 1 : close;
    return text.substr(0, at) + added + text.substr(at);
}

bool SvgReader::read(const SvgDocument& document, unsigned int index, const glm::mat3& toEm, FT_Library library, ColourGlyph* glyph) {
    if (!started() || library == nullptr) {
        return false;
    }
    const std::optional<std::string> text = inflated(document);
    if (!text || text->empty()) {
        return false;
    }
    const std::string sized = withViewport(*text, document.unitsPerEm);
    const std::unique_ptr<tvg::Picture, Release> picture(tvg::Picture::gen());
    if (!picture) {
        return false;
    }
    picture->ref();
    picture->accessible = true;
    if (picture->load(sized.data(), static_cast<std::uint32_t>(sized.size()), "svg", nullptr, true) != tvg::Result::Success) {
        return false;
    }
    const std::string name = "glyph" + std::to_string(index);
    // the element is found by its id among every paint the document made
    const std::uint32_t wanted = tvg::Accessor::id(name.c_str());
    const tvg::Paint* found = nullptr;
    const std::unique_ptr<tvg::Accessor> accessor(tvg::Accessor::gen());
    if (!accessor) {
        return false;
    }
    accessor->set(
        picture.get(),
        [wanted, &found](const tvg::Paint* paint, void* /*data*/) {
            if (paint->id == wanted) {
                found = paint;
                return false;
            }
            return true;
        },
        nullptr);
    if (found == nullptr) {
        return false;
    }
    // the transforms of the elements around the glyph's apply to it too, and the root's maps its
    // viewBox onto the em square
    glm::mat3 around(1.0F);
    for (const tvg::Paint* parent = found->parent(); parent != nullptr; parent = parent->parent()) {
        around = transformOf(parent) * around;
    }
    Reading reading{ .library = library, .glyph = glyph };
    const std::size_t before = glyph->layers.size();
    addPaint(found, toEm * around, 1.0F, std::vector<ColourClip>(), &reading);
    return glyph->layers.size() > before;
}

};  // namespace v3d::font::internal
