/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/scene/Item.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "Codec.h"

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

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

    Item::Kind kind() const noexcept override;
    std::string_view name() const noexcept override;
    std::int64_t version() const noexcept override;
    bool buffered() const noexcept override;
    std::vector<Buffer> buffers(const Item& item) const override;
    boost::json::object payload(const Item& item) const override;
    boost::shared_ptr<Item> read(const boost::json::object& payload, const Blobs& blobs, std::string* problem) const override;
};

};  // namespace v3d::editor
