/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

#include "Curves.h"

#include <glm/vec2.hpp>

namespace v3d::font {

class Face;

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
 * A float holds an index exactly up to 2 to the 24th, so the store holds that many curves and
 * values. A glyph is found by its face and its index, and kept as long as the store is: its
 * curves are the same at every size. A key names its face by its address, so a face the store
 * holds glyphs from stays open while the store is used. A store is used from one thread.
 **/
class CurveStore final {
 public:
    /**
     * The texels in each row of either array.
     **/
    static constexpr std::uint32_t ROW = 1024;

    /**
     * The rows of an array changed since they were last asked for.
     **/
    struct Changed final {
        std::uint32_t first = 0;
        std::uint32_t rows = 0;
    };

    /**
     * @return where a glyph's curves are stored, read and stored the first time it is asked for
     **/
    const StoredCurves& glyph(const Face& face, unsigned int index);

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

    std::map<std::pair<const Face*, unsigned int>, StoredCurves> glyphs_;
    std::vector<float> curves_;
    std::vector<float> bands_;
    std::size_t curveFloats_ = 0;
    std::size_t bandFloats_ = 0;
    Written curvesWritten_;
    Written bandsWritten_;
    std::size_t read_ = 0;
};

};  // namespace v3d::font
