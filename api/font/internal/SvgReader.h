/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/ColourGlyph.h>
#include <api/font/Curves.h>
#include <thorvg.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::font::internal {

/**
 * An SVG table's document, as the table holds it.
 **/
struct SvgDocument final {
    std::string_view text;       /**< plain, or compressed by gzip **/
    unsigned int unitsPerEm = 0; /**< the face's, which is the document's first viewport each way **/
    FT_Memory memory = nullptr;  /**< FreeType's, which inflates a compressed document **/
};

/**
 * Reads one glyph of an OpenType SVG document into a colour glyph's layers, through ThorVG.
 *
 * The glyph is the element whose id is "glyph" and its index. Its document's first viewport is
 * the em square, which a viewBox on the document's root maps onto, and nothing is clipped to it.
 * Each shape's fill and stroke is a fill layer inside the shape's outline, or the outline
 * FreeType's stroker draws around it, and inside every clip path above it. A shape's opacity,
 * and the opacities above it, multiply its colours' alpha. A masked element is gathered as a
 * group, and the mask's alpha keeps or removes it. A luminance mask is read as an alpha mask,
 * and a dashed stroke is drawn whole.
 **/
class SvgReader final {
 public:
    /**
     * Lets a picture go when it leaves scope.
     **/
    struct Release final {
        void operator()(tvg::Picture* picture) const {
            picture->unref();
        }
    };

    /**
     * Parse a document, inflated when it is compressed and given the em square as its first
     * viewport.
     *
     * @return its picture, or null when ThorVG cannot start or the document does not parse
     **/
    static std::unique_ptr<tvg::Picture, Release> load(const SvgDocument& document);

    /**
     * Read a glyph from a parsed document.
     *
     * @param picture the document, which may hold other glyphs
     * @param toEm maps the document's units, with y downwards, into em units
     * @param library the FreeType library whose stroker outlines strokes
     * @return whether the glyph was found, its layers added to the glyph's
     **/
    static bool read(tvg::Picture* picture, unsigned int index, const glm::mat3& toEm, FT_Library library, ColourGlyph* glyph);

 private:
    /**
     * ThorVG started once for the life of the program, with no threads of its own, so a picture
     * is loaded before load() returns.
     **/
    class Started final {
     public:
        Started() :
            started_(tvg::Initializer::init(0) == tvg::Result::Success) {
        }
        ~Started() {
            if (started_) {
                tvg::Initializer::term();
            }
        }
        Started(const Started&) = delete;
        Started& operator=(const Started&) = delete;

        bool started() const noexcept {
            return started_;
        }

     private:
        bool started_;
    };

    /**
     * A shape's path as ThorVG holds it.
     **/
    struct Path final {
        const tvg::PathCommand* commands = nullptr;
        std::uint32_t commandCount = 0;
        const tvg::Point* points = nullptr;
        std::uint32_t pointCount = 0;

        explicit Path(const tvg::Shape& shape) {
            if (shape.path(&commands, &commandCount, &points, &pointCount) != tvg::Result::Success) {
                commandCount = 0;
                pointCount = 0;
            }
        }

        /**
         * @return how many points a command takes
         **/
        static std::uint32_t takes(tvg::PathCommand command) {
            switch (command) {
            case tvg::PathCommand::MoveTo:
            case tvg::PathCommand::LineTo:
                return 1;
            case tvg::PathCommand::CubicTo:
                return 3;
            default:
                return 0;
            }
        }

        glm::vec2 at(std::uint32_t point) const {
            return glm::vec2(points[point].x, points[point].y);
        }
    };

    /**
     * Where a walk through a glyph's paints puts what it reads.
     **/
    struct Reading final {
        FT_Library library = nullptr;
        ColourGlyph* glyph = nullptr;
    };

    /**
     * The most a colour channel or an opacity holds.
     **/
    static constexpr float CHANNEL = 255.0F;

    /**
     * FreeType's stroker takes points in 26.6 fixed point and a miter limit in 16.16.
     **/
    static constexpr float FIXED_POINT = 64.0F;
    static constexpr float FIXED_16 = 65536.0F;

    /**
     * The first bytes of a document compressed by gzip.
     **/
    static constexpr std::array<unsigned char, 3> GZIP = { 0x1F, 0x8B, 0x08 };

    /**
     * The largest document inflated, in bytes.
     **/
    static constexpr std::uint32_t LARGEST_DOCUMENT = 64U * 1024U * 1024U;

    /**
     * @return whether ThorVG has started, starting it the first time
     **/
    static bool started();

    /**
     * @return a ThorVG matrix, x' = e11 x + e12 y + e13, as a transform
     **/
    static glm::mat3 matrixOf(const tvg::Matrix& matrix);

    /**
     * @return the transform a paint sets on what it draws
     **/
    static glm::mat3 transformOf(const tvg::Paint* paint);

    /**
     * @return a shape's outline, every contour closed, in em units
     **/
    static std::vector<QuadraticCurve> outlineOf(const tvg::Shape& shape, const glm::mat3& toEm);

    /**
     * @return a point in the stroker's fixed point
     **/
    static FT_Vector fixed(const glm::vec2& point);

    /**
     * @return the outline FreeType's stroker draws around a shape's path, in em units, or none
     *         for a shape with no stroke
     **/
    static std::vector<QuadraticCurve> strokeOf(const tvg::Shape& shape, const glm::mat3& toEm, FT_Library library);

    /**
     * @return FreeType's cap for ThorVG's
     **/
    static FT_Stroker_LineCap capOf(tvg::StrokeCap cap);

    /**
     * @return FreeType's join for ThorVG's
     **/
    static FT_Stroker_LineJoin joinOf(tvg::StrokeJoin join);

    /**
     * Give the stroker each subpath of a path: each run from a move, closed when a close ends it.
     **/
    static void trace(const Path& path, FT_Stroker stroker);

    /**
     * @return a solid fill of a colour's red, green, blue and alpha bytes, its alpha times an
     *         opacity
     **/
    static ColourFill solidFill(const glm::u8vec4& bytes, float opacity);

    /**
     * @return a gradient fill, its stops' alpha times an opacity, or nothing for a fill that is
     *         not a gradient ThorVG reads
     **/
    static std::optional<ColourFill> gradientFill(const tvg::Fill& gradient, const glm::mat3& toEm, float opacity);

    /**
     * Add a layer filled one way or another inside an outline and the clips above it.
     **/
    static void addLayer(const std::vector<ColourClip>& clips, ColourClip&& outline, std::optional<ColourFill>&& fill, Reading* reading);

    /**
     * Add a shape's fill, then its stroke.
     **/
    static void addShape(const tvg::Shape& shape, const glm::mat3& toEm, float opacity, const std::vector<ColourClip>& clips,
        Reading* reading);

    /**
     * Add a paint and everything beneath it.
     *
     * @param above the transform from the paint's parent's units into em units
     **/
    static void addPaint(const tvg::Paint* paint, const glm::mat3& above, float opacity, std::vector<ColourClip> clips, Reading* reading);

    /**
     * @return a document as text, inflated when gzip compressed it, or nothing when it will not
     *         inflate
     **/
    static std::optional<std::string> inflated(const SvgDocument& document);

    /**
     * @return a document whose root element is given the em square as its viewport. The root is
     *         made as wide and as tall as the units to the em, and given that square as its
     *         viewBox, wherever it does not say otherwise
     **/
    static std::string withViewport(const std::string& text, unsigned int unitsPerEm);
};

};  // namespace v3d::font::internal
