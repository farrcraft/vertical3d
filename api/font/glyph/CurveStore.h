/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "ColourGlyph.h"
#include "Curves.h"

#include <glm/vec2.hpp>

namespace v3d::font::face {

class Face;

};  // namespace v3d::font::face

namespace v3d::font::glyph {


/**
 * Where a glyph's curves are stored, and the part of the em square they cover.
 **/
struct StoredCurves final {
    std::uint32_t bands = 0;   /**< the value its bands start at **/
    glm::vec2 min{ 0.0F };     /**< its outline's lower left, in em units **/
    glm::vec2 max{ 0.0F };
    bool empty = true;         /**< whether it has no curves, as a space has none, and draws nothing **/
};

/**
 * Where a colour glyph's program is stored, and the part of the em square it draws.
 **/
struct StoredColour final {
    std::uint32_t program = 0;  /**< the value its program starts at **/
    glm::vec2 min{ 0.0F };      /**< what it draws' lower left, in em units **/
    glm::vec2 max{ 0.0F };
    bool empty = true;          /**< whether it draws nothing from curves **/
    bool pictures = false;      /**< whether it places pictures, which a program does not draw **/
};

/**
 * The curves text is drawn from at any size and under any transform. A glyph's curves are read
 * the first time it is drawn, and kept in two arrays of floats, four to a texel, in rows of ROW
 * texels.
 *
 * Curve c is texels 2c and 2c + 1: p1 and p2, then p3. Band value k is float k of the bands. A
 * glyph's bands start at its value g:
 *
 * - values g to g + 3 are its band transform, x and y scale then x and y offset;
 * - values g + 4 and g + 5 are how many vertical and horizontal bands it has;
 * - from g + 8, each horizontal band has its curve count and the value its list starts at, then
 *   each vertical band has the same;
 * - each list holds curve indices, in the band's order.
 *
 * A colour glyph is a program among the band values, which draws its layers in order. Each
 * clip of each fill is an outline stored as a glyph is. A program starts at its value p:
 *
 * - value p is how many steps it has, and its steps start at p + 4, STEP values each;
 * - a step's value 0 is its kind: 0 a fill, 2 the beginning of a group, 3 a group's end;
 * - an end's values 1 and 2 are its composite, as ColourComposite numbers them, and its opacity;
 * - a fill's value 3 is how many clips it has, at most CLIPS, and values 4 to 6 the values
 *   their bands start at. Bit c of value 7 says clip c is filled by the even-odd rule;
 * - value 8 is the fill's kind: 0 solid, 1 linear, 2 radial, 3 sweep. Value 9 is its extend, as
 *   ColourExtend numbers it, value 10 how many stops it has and value 11 the value they start
 *   at, STOP values each: offset, red, green, blue, alpha, and 1 for the text's colour;
 * - values 12 to 17 map a point in em units into the gradient's space: x' = a x + c y + e and
 *   y' = b x + d y + f, in the order a to f;
 * - values 18 and 19 are its start or centre, 20 and 21 its end or second centre, and 22 and 23
 *   its radii, or a sweep's angles in radians.
 *
 * A vertex names a program, rather than a glyph, by setting the bit COLOUR in its glyph value.
 *
 * A float holds an index exactly up to 2 to the 24th, so the store holds that many curves and
 * values. A glyph is found by its face, its index and a variable font's normalized coordinates,
 * and kept as long as the store is: its curves are the same at every size. A key names its face
 * by its address, so a face the store holds glyphs from stays open while the store is used. A
 * store is used from one thread.
 **/
class CurveStore final {
 public:
    /**
     * The texels in each row of either array.
     **/
    static constexpr std::uint32_t ROW = 1024;

    /**
     * The bit a vertex's glyph value sets to name a colour program.
     **/
    static constexpr std::uint32_t COLOUR = 0x80000000U;

    /**
     * The values in a program's step, and in one of its stops.
     **/
    static constexpr std::uint32_t STEP = 24;
    static constexpr std::uint32_t STOP = 6;

    /**
     * The most clips a fill is drawn inside. A layer with more is drawn inside the first of
     * them.
     **/
    static constexpr std::uint32_t CLIPS = 3;

    /**
     * The rows of an array changed since they were last asked for.
     **/
    struct Changed final {
        std::uint32_t first = 0;
        std::uint32_t rows = 0;
    };

    /**
     * @return where a glyph's curves are stored, read and stored the first time it is asked for
     *         at its coordinates
     **/
    const StoredCurves& glyph(const face::Face& face, unsigned int index, const std::vector<std::int32_t>& coordinates = {});

    /**
     * @return where a colour glyph's program is stored, read and stored the first time it is
     *         asked for in a palette at its coordinates. A glyph with no colour stores nothing
     **/
    const StoredColour& colour(const face::Face& face, unsigned int index, unsigned int palette, const std::vector<std::int32_t>& coordinates = {});

    /**
     * @return the curves, in whole rows
     **/
    const std::vector<float>& curves() const noexcept;

    /**
     * @return the bands, in whole rows
     **/
    const std::vector<float>& bands() const noexcept;

    /**
     * @return how many rows each array holds. Each doubles as it fills, so a renderer makes its
     *         texture again only when one has grown
     **/
    std::uint32_t curveRows() const noexcept;
    std::uint32_t bandRows() const noexcept;

    /**
     * @return the curve rows changed since this was last called, and forget them
     **/
    Changed changedCurves();

    /**
     * @return the band rows changed since this was last called, and forget them
     **/
    Changed changedBands();

    /**
     * @return how many glyphs have been read rather than found
     **/
    std::size_t read() const noexcept;

 private:
    /**
     * The floats in a texel.
     **/
    static constexpr std::uint32_t TEXEL = 4;

    /**
     * Store a banded outline's curves and bands.
     *
     * @return the value its bands start at
     **/
    std::uint32_t store(const CurveGlyph& outlined);

    /**
     * The size in pixels a colour glyph is read at. It picks only a picture's strike, and a
     * program draws no pictures.
     **/
    static constexpr float READ_SIZE = 64.0F;

    /**
     * The values of a program's header, before its first step.
     **/
    static constexpr std::uint32_t HEADER = 4;

    /**
     * Where each of a step's values is, as the class describes them.
     **/
    static constexpr std::size_t KIND = 0;
    static constexpr std::size_t COMPOSITE = 1;
    static constexpr std::size_t OPACITY = 2;
    static constexpr std::size_t CLIP_COUNT = 3;
    static constexpr std::size_t FIRST_CLIP = 4;
    static constexpr std::size_t EVEN_ODD = 7;
    static constexpr std::size_t FILL_KIND = 8;
    static constexpr std::size_t EXTEND = 9;
    static constexpr std::size_t STOP_COUNT = 10;
    static constexpr std::size_t FIRST_STOP = 11;
    static constexpr std::size_t GEOMETRY = 12;

    /**
     * A step's kinds.
     **/
    static constexpr float FILL = 0.0F;
    static constexpr float BEGIN_GROUP = 2.0F;
    static constexpr float END_GROUP = 3.0F;

    /**
     * @return a fill's step, its clips stored and its stops added to the stops given, with its
     *         stops' place among them in value 11. Nothing for a fill whose clips are empty, or
     *         whose gradient cannot be mapped back
     **/
    std::optional<std::vector<float>> fillStep(const ColourLayer& layer, std::vector<float>* stops);

    /**
     * Where an array has been written since its rows were last asked for: the first and one past
     * the last float.
     **/
    struct Written {
        std::size_t from = 0;
        std::size_t to = 0;
    };

    /**
     * Put values at the end of an array, growing it a doubling of rows at a time.
     *
     * @return where they start
     **/
    static std::size_t append(std::vector<float>* array, std::size_t* used, const std::vector<float>& values, Written* written);

    /**
     * @return the rows changed, and forget them
     **/
    static Changed take(Written* written);

    std::map<std::tuple<const face::Face*, unsigned int, std::vector<std::int32_t>>, StoredCurves> glyphs_;
    std::map<std::tuple<const face::Face*, unsigned int, unsigned int, std::vector<std::int32_t>>, StoredColour> colours_;
    std::vector<float> curves_;
    std::vector<float> bands_;
    std::size_t curveFloats_ = 0;
    std::size_t bandFloats_ = 0;
    Written curvesWritten_;
    Written bandsWritten_;
    std::size_t read_ = 0;
};

};  // namespace v3d::font::glyph
