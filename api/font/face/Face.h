/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/glyph/ColourGlyph.h>
#include <api/log/Logger.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "Metrics.h"
#include "Style.h"
#include "Variation.h"

#include <boost/shared_ptr.hpp>

namespace v3d::font::internal {
class FaceHandle;
};  // namespace v3d::font::internal

namespace v3d::font::face {

class Library;

/**
 * One axis of a variable font, in the units its values are given in.
 **/
struct Axis final {
    std::string tag;           /**< its four letter name, such as "wght" **/
    float min = 0.0F;
    float initial = 0.0F;      /**< the value the face is drawn at when none is given **/
    float max = 0.0F;
};

/**
 * One font file, held open from open() until the last holder lets it go.
 *
 * A face names its family and style as the file does. Its weight is the file's OS/2 weight
 * class, from 100 for thin to 900 for black, with 400 regular and 700 bold. A file with no OS/2
 * table is 700 when it says it is bold, and 400 otherwise.
 *
 * A glyph is named by its index in the face. Index 0 is the face's missing glyph.
 *
 * A variable font's axes are each set by a normalized coordinate in 2.14 fixed point. The
 * coordinate is -1 at the axis's least value, 0 at its default and 1 at its greatest, after the
 * font's avar table maps it. No coordinates at all draw every axis at its default, as does a
 * face with no axes.
 **/
class Face final {
 public:
    /**
     * Open a font file through a library.
     * @return the face, or null when the file does not open or has no Unicode character map,
     *         which is logged
     **/
    static boost::shared_ptr<Face> open(const boost::shared_ptr<Library>& library, const std::string& filename,
        const boost::shared_ptr<v3d::log::Logger>& logger);

    const std::string& filename() const noexcept;
    const std::string& family() const noexcept;
    const std::string& style() const noexcept;
    unsigned int weight() const noexcept;
    bool italic() const noexcept;

    /**
     * @return the lightest and the heaviest weight the face draws: the range of its weight axis,
     *         or its weight for both when it has none
     **/
    std::pair<unsigned int, unsigned int> weights() const noexcept;

    /**
     * @return a variable font's axes, or none
     **/
    const std::vector<Axis>& axes() const noexcept;

    /**
     * Find the coordinates text is drawn at, as CSS sets a variable font's axes. The weight axis
     * takes the style's weight and the optical size axis the size in pixels, and then each
     * variation is set over them. A variation whose tag is not four letters is left out.
     *
     * @return the normalized coordinates of every axis, or none when each is at its default
     **/
    std::vector<std::int32_t> coordinates(const Style& style, float size, const std::vector<Variation>& variations) const;

    /**
     * @return the index of a code point's glyph, or 0 when the face has none
     **/
    unsigned int glyphIndex(char32_t charcode) const;

    /**
     * @return whether the face has a glyph for a code point
     **/
    bool has(char32_t charcode) const;

    /**
     * @return the face's vertical measures at a size in pixels and at normalized coordinates. A
     *         face whose OS/2 table asks for its typographic metrics is measured by them, and
     *         any other by its hhea table
     **/
    Metrics metrics(float size, const std::vector<std::int32_t>& coordinates = {}) const;

    /**
     * @return the colour tables the face holds
     **/
    glyph::ColourTables colour() const;

    /**
     * @return whether a glyph is drawn in colour, from any of the face's colour tables
     **/
    bool coloured(unsigned int index) const;

    /**
     * @return the library the face was opened through
     **/
    const boost::shared_ptr<Library>& library() const noexcept;

    /**
     * @return the FreeType face, for the library's own sources
     **/
    internal::FaceHandle& handle() const;

 private:
    Face() = default;

    // declared before the face, so the face is closed before its library
    boost::shared_ptr<Library> library_;
    boost::shared_ptr<internal::FaceHandle> handle_;
    std::string filename_;
    std::string family_;
    std::string style_;
    unsigned int weight_ = 0;
    std::pair<unsigned int, unsigned int> weights_{ 0, 0 };
    std::vector<Axis> axes_;
    bool italic_ = false;
};

};  // namespace v3d::font::face
