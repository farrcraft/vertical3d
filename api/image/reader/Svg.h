/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Reader.h>

#include <cstddef>

#include <glm/vec2.hpp>

namespace v3d::image::reader {
/**
 * An SVG document, drawn into an RGBA image.
 *
 * SVG is drawn rather than decoded, so it can be read at any size. Lengths are read as pixels at
 * 96 to the inch. Only the subset NanoSVG understands is drawn: shapes, paths, fills, strokes
 * and gradients, but no text, filters or embedded images.
 **/
class Svg : public v3d::image::Reader {
 public:
    explicit Svg(const boost::shared_ptr<v3d::log::Logger> & logger);
    ~Svg() = default;

    // the path form is the base's, written in terms of this one
    using v3d::image::Reader::read;

    /**
     * @return the document drawn at the size it gives itself, or null when it cannot be read or
     *         gives itself no size
     **/
    boost::shared_ptr<Image> read(const unsigned char* encoded, std::size_t size) override;

    /**
     * @param into the size of the image made. The document is scaled to fit inside it, keeping
     *             its proportions, and centred in it
     * @return the document drawn into an image of that size, or null when it cannot be read,
     *         gives itself no size, or the size asked for is empty
     **/
    boost::shared_ptr<Image> read(const unsigned char* encoded, std::size_t size, const glm::uvec2& into);
};

};  // namespace v3d::image::reader
