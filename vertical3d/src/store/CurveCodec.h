/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/curve/Curve.h>
#include <api/curve/Curves.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/store/Blob.h>
#include <vertical3d/src/store/Cursor.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "Codec.h"

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::store {

/**
 * A curve item's payload is its style and resolution: "renders", a boolean, "width", a number
 * above zero, and "resolution", the pieces each segment is drawn in, from 1 to 1024. Its curves
 * are five blobs of little-endian arrays:
 *
 * - curves: each its type (0 poly, 1 Bezier, 2 Catmull-Rom, 3 NURBS), whether it is closed,
 *   whether it is weighted, its order, its number of points and its number of knots, six 32 bit
 *   numbers. The order and the knots are 0 for a type other than NURBS
 * - points: three floats each, in curve order. A Bezier point is three: its position, then its
 *   left and its right handle
 * - handles: each Bezier point's left and right handle type (0 free, 1 aligned, 2 vector,
 *   3 automatic), two bytes
 * - weights: one float for each point of a weighted NURBS curve
 * - knots: the floats of each NURBS curve's own knots
 *
 * A read refuses blobs whose counts do not add up to what the curves say they hold, and any
 * curve curve::problem() finds fault with.
 **/
class CurveCodec final : public Codec {
 public:
    static constexpr std::int64_t VERSION = 1;

    scene::item::Item::Kind kind() const noexcept override;
    std::string_view name() const noexcept override;
    std::int64_t version() const noexcept override;
    bool buffered() const noexcept override;
    std::vector<Buffer> buffers(const scene::item::Item& item) const override;
    boost::json::object payload(const scene::item::Item& item) const override;
    boost::shared_ptr<scene::item::Item> read(const boost::json::object& payload, const Blobs& blobs, std::string* problem) const override;

 private:
    /**
     * The size of one entry of each blob. A curve is six 32 bit numbers, a point three floats, a
     * Bezier point's handle types two bytes, and a weight or a knot one float.
     **/
    static constexpr std::size_t CURVE_SIZE = 6 * sizeof(std::uint32_t);
    static constexpr std::size_t POINT_SIZE = 3 * sizeof(float);
    static constexpr std::size_t HANDLE_SIZE = 2 * sizeof(std::uint8_t);
    static constexpr std::size_t NUMBER_SIZE = sizeof(float);

    /**
     * The blobs a curve item writes: its curves, points, handles, weights and knots.
     **/
    static constexpr std::size_t BLOBS = 5;

    /**
     * The most pieces a segment is drawn in. More would only slow the drawing.
     **/
    static constexpr float MOST_PIECES = 1024.0F;

    /**
     * Each type's number in the curves blob.
     **/
    enum class Type : std::uint8_t { Poly = 0, Bezier = 1, CatmullRom = 2, Nurbs = 3 };

    /**
     * A curve's entry in the curves blob.
     **/
    struct Record {
        Type type = Type::Poly;
        bool closed = false;
        bool weighted = false;
        std::uint32_t order = 0;
        std::uint32_t points = 0;
        std::uint32_t knots = 0;
    };

    static void put(std::vector<std::byte>* bytes, const Record& record);

    /**
     * Read every curve's entry, refusing one whose numbers no curve could have.
     **/
    static bool records(const Blob& blob, std::vector<Record>* read, std::string* problem);

    /**
     * The entries each blob needs to hold for the curves recorded.
     **/
    struct Totals {
        std::uint64_t points = 0;
        std::uint64_t handles = 0;
        std::uint64_t weights = 0;
        std::uint64_t knots = 0;
    };

    static Totals totals(const std::vector<Record>& read);

    static std::vector<float> floats(Cursor* cursor, std::uint32_t count);

    static std::vector<glm::vec3> vectors(Cursor* cursor, std::uint32_t count);

    /**
     * The cursors over the blobs a curve's numbers are read from.
     **/
    struct Cursors {
        Cursor points;
        Cursor handles;
        Cursor weights;
        Cursor knots;
    };

    /**
     * Make one curve from its entry and the blobs' next numbers, or nothing with the reason in
     * problem.
     **/
    static bool make(const Record& record, Cursors* from, v3d::curve::Curve* curve, std::string* problem);

    /**
     * Build the curves from their five blobs.
     **/
    static bool decode(const Blobs& blobs, v3d::curve::Curves* curves, std::string* problem);
};

};  // namespace v3d::editor::store
