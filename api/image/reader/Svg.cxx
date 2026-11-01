/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Svg.h"

#include <api/image/Image.h>
#include <api/image/Reader.h>
#include <api/log/Logger.h>

#include <nanosvg.h>
#include <nanosvgrast.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <boost/make_shared.hpp>

namespace v3d::image::reader {

namespace {

/**
 * The resolution a length in inches, points or millimetres is read at, which is the one SVG
 * itself assumes.
 **/
constexpr float DPI = 96.0F;

/**
 * The bits in a pixel of the image made: four channels of eight bits.
 **/
constexpr std::uint8_t RGBA_BITS = 32;

using Document = std::unique_ptr<NSVGimage, decltype(&nsvgDelete)>;
using Rasteriser = std::unique_ptr<NSVGrasterizer, decltype(&nsvgDeleteRasterizer)>;

/**
 * @return the document parsed, or null. The parser writes into the text it is given, so it is
 *         given a copy that ends in a null
 **/
Document parse(const unsigned char* encoded, std::size_t size) {
    if (encoded == nullptr || size == 0) {
        return Document(nullptr, &nsvgDelete);
    }
    std::string text(reinterpret_cast<const char*>(encoded), size);
    return Document(nsvgParse(text.data(), "px", DPI), &nsvgDelete);
}

};  // namespace

/**
 **/
Svg::Svg(const boost::shared_ptr<v3d::log::Logger> & logger) :
    Reader(logger) {
}

/**
 **/
boost::shared_ptr<Image> Svg::read(const unsigned char* encoded, std::size_t size) {
    const Document document = parse(encoded, size);
    if (!document || !(document->width > 0.0F) || !(document->height > 0.0F)) {
        logger()->get()->error("The SVG document could not be read, or gives itself no size");
        return nullptr;
    }
    const glm::uvec2 own(static_cast<unsigned int>(std::ceil(document->width)), static_cast<unsigned int>(std::ceil(document->height)));
    return read(encoded, size, own);
}

/**
 **/
boost::shared_ptr<Image> Svg::read(const unsigned char* encoded, std::size_t size, const glm::uvec2& into) {
    if (into.x == 0 || into.y == 0) {
        logger()->get()->error("An SVG document cannot be drawn into an empty image");
        return nullptr;
    }
    const Document document = parse(encoded, size);
    if (!document || !(document->width > 0.0F) || !(document->height > 0.0F)) {
        logger()->get()->error("The SVG document could not be read, or gives itself no size");
        return nullptr;
    }
    const Rasteriser rasteriser(nsvgCreateRasterizer(), &nsvgDeleteRasterizer);
    if (!rasteriser) {
        logger()->get()->error("No rasteriser could be made to draw an SVG document");
        return nullptr;
    }

    const float scale = std::fmin(static_cast<float>(into.x) / document->width, static_cast<float>(into.y) / document->height);
    const float x = (static_cast<float>(into.x) - (document->width * scale)) / 2.0F;
    const float y = (static_cast<float>(into.y) - (document->height * scale)) / 2.0F;
    boost::shared_ptr<Image> image = boost::make_shared<Image>(into, RGBA_BITS);
    // the rasteriser fills every pixel, transparent where nothing is drawn
    nsvgRasterize(rasteriser.get(), document.get(), x, y, scale, image->data(), static_cast<int>(into.x), static_cast<int>(into.y),
        static_cast<int>(into.x * 4));
    return image;
}

};  // namespace v3d::image::reader
