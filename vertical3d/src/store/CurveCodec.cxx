/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "CurveCodec.h"

#include <api/curve/Curve.h>
#include <api/curve/Curves.h>
#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/Item.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include "Bytes.h"
#include "Codec.h"
#include "Payload.h"

#include <boost/json.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

namespace {

/**
 * The size of one entry of each blob. A curve is six 32 bit numbers, a point three floats, a
 * Bezier point's handle types two bytes, and a weight or a knot one float.
 **/
constexpr std::size_t CURVE_SIZE = 6 * sizeof(std::uint32_t);
constexpr std::size_t POINT_SIZE = 3 * sizeof(float);
constexpr std::size_t HANDLE_SIZE = 2 * sizeof(std::uint8_t);
constexpr std::size_t NUMBER_SIZE = sizeof(float);

/**
 * The blobs a curve item writes: its curves, points, handles, weights and knots.
 **/
constexpr std::size_t BLOBS = 5;

/**
 * The most pieces a segment is drawn in. More would only slow the drawing.
 **/
constexpr float MOST_PIECES = 1024.0F;

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

void put(std::vector<std::byte>* bytes, const Record& record) {
    v3d::editor::put(bytes, static_cast<std::uint32_t>(record.type));
    v3d::editor::put(bytes, static_cast<std::uint32_t>(record.closed ? 1 : 0));
    v3d::editor::put(bytes, static_cast<std::uint32_t>(record.weighted ? 1 : 0));
    v3d::editor::put(bytes, record.order);
    v3d::editor::put(bytes, record.points);
    v3d::editor::put(bytes, record.knots);
}

/**
 * Read every curve's entry, refusing one whose numbers no curve could have.
 **/
bool records(const Blob& blob, std::vector<Record>* read, std::string* problem) {
    Cursor cursor(blob.data);
    for (std::int64_t entry = 0; entry < blob.count; entry++) {
        const auto type = cursor.next<std::uint32_t>();
        const auto closed = cursor.next<std::uint32_t>();
        const auto weighted = cursor.next<std::uint32_t>();
        if (type > static_cast<std::uint32_t>(Type::Nurbs) || closed > 1 || weighted > 1) {
            *problem = "a curve's type, closure or weighting is not one a curve can have";
            return false;
        }
        const Record record{ .type = static_cast<Type>(type), .closed = closed == 1, .weighted = weighted == 1,
            .order = cursor.next<std::uint32_t>(), .points = cursor.next<std::uint32_t>(), .knots = cursor.next<std::uint32_t>() };
        if (record.type != Type::Nurbs && (record.weighted || record.order != 0 || record.knots != 0)) {
            *problem = "a curve that is not NURBS has an order, weights or knots";
            return false;
        }
        read->push_back(record);
    }
    return true;
}

/**
 * The entries each blob needs to hold for the curves recorded.
 **/
struct Totals {
    std::uint64_t points = 0;
    std::uint64_t handles = 0;
    std::uint64_t weights = 0;
    std::uint64_t knots = 0;
};

Totals totals(const std::vector<Record>& read) {
    Totals needed;
    for (const Record& record : read) {
        needed.points += record.type == Type::Bezier ? 3ULL * record.points : record.points;
        needed.handles += record.type == Type::Bezier ? record.points : 0;
        needed.weights += record.weighted ? record.points : 0;
        needed.knots += record.knots;
    }
    return needed;
}

std::vector<float> floats(Cursor* cursor, std::uint32_t count) {
    std::vector<float> read;
    read.reserve(count);
    for (std::uint32_t each = 0; each < count; each++) {
        read.push_back(cursor->next<float>());
    }
    return read;
}

std::vector<glm::vec3> vectors(Cursor* cursor, std::uint32_t count) {
    std::vector<glm::vec3> read;
    read.reserve(count);
    for (std::uint32_t each = 0; each < count; each++) {
        read.push_back(cursor->vector());
    }
    return read;
}

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
bool make(const Record& record, Cursors* from, v3d::curve::Curve* curve, std::string* problem) {
    curve->closed = record.closed;
    switch (record.type) {
        case Type::Poly:
            curve->shape = v3d::curve::Poly{ vectors(&from->points, record.points) };
            break;
        case Type::CatmullRom:
            curve->shape = v3d::curve::CatmullRom{ vectors(&from->points, record.points) };
            break;
        case Type::Bezier: {
            v3d::curve::Bezier bezier;
            for (std::uint32_t each = 0; each < record.points; each++) {
                v3d::curve::BezierPoint point;
                point.position = from->points.vector();
                point.left = from->points.vector();
                point.right = from->points.vector();
                const auto left = from->handles.next<std::uint8_t>();
                const auto right = from->handles.next<std::uint8_t>();
                if (left > static_cast<std::uint8_t>(v3d::curve::Handle::Automatic) ||
                    right > static_cast<std::uint8_t>(v3d::curve::Handle::Automatic)) {
                    *problem = "a Bezier handle's type is not one a handle can have";
                    return false;
                }
                point.leftType = static_cast<v3d::curve::Handle>(left);
                point.rightType = static_cast<v3d::curve::Handle>(right);
                bezier.points.push_back(point);
            }
            curve->shape = bezier;
            break;
        }
        case Type::Nurbs: {
            v3d::curve::Nurbs nurbs;
            nurbs.order = record.order;
            nurbs.points = vectors(&from->points, record.points);
            if (record.weighted) {
                nurbs.weights = floats(&from->weights, record.points);
            }
            nurbs.knots = floats(&from->knots, record.knots);
            curve->shape = nurbs;
            break;
        }
    }
    return true;
}

/**
 * Build the curves from their five blobs.
 **/
bool decode(const Blobs& blobs, v3d::curve::Curves* curves, std::string* problem) {
    if (blobs.size() != BLOBS || !blobs.contains("curves") || !blobs.contains("points") || !blobs.contains("handles") ||
        !blobs.contains("weights") || !blobs.contains("knots")) {
        *problem = "it does not have exactly its curves, points, handles, weights and knots";
        return false;
    }
    const Blob& curveBlob = blobs.at("curves");
    const Blob& pointBlob = blobs.at("points");
    const Blob& handleBlob = blobs.at("handles");
    const Blob& weightBlob = blobs.at("weights");
    const Blob& knotBlob = blobs.at("knots");
    if (!sized(curveBlob, CURVE_SIZE, "curves", problem) || !sized(pointBlob, POINT_SIZE, "points", problem) ||
        !sized(handleBlob, HANDLE_SIZE, "handles", problem) || !sized(weightBlob, NUMBER_SIZE, "weights", problem) ||
        !sized(knotBlob, NUMBER_SIZE, "knots", problem)) {
        return false;
    }

    std::vector<Record> read;
    if (!records(curveBlob, &read, problem)) {
        return false;
    }
    // every count is checked before anything is read, so no cursor reads past its blob
    const Totals needed = totals(read);
    if (needed.points != static_cast<std::uint64_t>(pointBlob.count) ||
        needed.handles != static_cast<std::uint64_t>(handleBlob.count) ||
        needed.weights != static_cast<std::uint64_t>(weightBlob.count) || needed.knots != static_cast<std::uint64_t>(knotBlob.count)) {
        *problem = "its points, handles, weights or knots are not as many as its curves hold";
        return false;
    }

    Cursors from{ .points = Cursor(pointBlob.data), .handles = Cursor(handleBlob.data), .weights = Cursor(weightBlob.data),
        .knots = Cursor(knotBlob.data) };
    for (const Record& record : read) {
        v3d::curve::Curve curve;
        if (!make(record, &from, &curve, problem)) {
            return false;
        }
        const std::string refused = curves->add(curve);
        if (!refused.empty()) {
            *problem = "a curve cannot be drawn: " + refused;
            return false;
        }
    }
    return true;
}

};  // namespace

/**
 **/
Item::Kind CurveCodec::kind() const noexcept {
    return Item::Kind::Curve;
}

/**
 **/
std::string_view CurveCodec::name() const noexcept {
    return "curve";
}

/**
 **/
std::int64_t CurveCodec::version() const noexcept {
    return VERSION;
}

/**
 **/
bool CurveCodec::buffered() const noexcept {
    return true;
}

/**
 **/
boost::json::object CurveCodec::payload(const Item& item) const {
    const Curve& curve = static_cast<const Curve&>(item);
    boost::json::object payload;
    payload["renders"] = curve.style().renders;
    payload["width"] = curve.style().width;
    payload["resolution"] = curve.geometry()->resolution();
    return payload;
}

/**
 **/
std::vector<Buffer> CurveCodec::buffers(const Item& item) const {
    const v3d::curve::Curves& curves = *static_cast<const Curve&>(item).geometry();
    Buffer records{ .role = "curves", .count = curves.size() };
    Buffer points{ .role = "points" };
    Buffer handles{ .role = "handles" };
    Buffer weights{ .role = "weights" };
    Buffer knots{ .role = "knots" };
    auto vectors = [&points](const std::vector<glm::vec3>& from) {
        for (const glm::vec3& point : from) {
            v3d::editor::put(&points.data, point);
        }
        points.count += from.size();
    };
    for (std::size_t index = 0; index < curves.size(); index++) {
        const v3d::curve::Curve& curve = curves.curve(index);
        Record record{ .closed = curve.closed };
        std::visit(
            [&](const auto& shape) {
                using Shape = std::decay_t<decltype(shape)>;
                record.points = static_cast<std::uint32_t>(shape.points.size());
                if constexpr (std::is_same_v<Shape, v3d::curve::Poly>) {
                    record.type = Type::Poly;
                    vectors(shape.points);
                } else if constexpr (std::is_same_v<Shape, v3d::curve::CatmullRom>) {
                    record.type = Type::CatmullRom;
                    vectors(shape.points);
                } else if constexpr (std::is_same_v<Shape, v3d::curve::Bezier>) {
                    record.type = Type::Bezier;
                    for (const v3d::curve::BezierPoint& point : shape.points) {
                        vectors({ point.position, point.left, point.right });
                        v3d::editor::put(&handles.data, static_cast<std::uint8_t>(point.leftType));
                        v3d::editor::put(&handles.data, static_cast<std::uint8_t>(point.rightType));
                        handles.count++;
                    }
                } else {
                    record.type = Type::Nurbs;
                    record.weighted = !shape.weights.empty();
                    record.order = shape.order;
                    record.knots = static_cast<std::uint32_t>(shape.knots.size());
                    vectors(shape.points);
                    for (const float weight : shape.weights) {
                        v3d::editor::put(&weights.data, weight);
                    }
                    weights.count += shape.weights.size();
                    for (const float knot : shape.knots) {
                        v3d::editor::put(&knots.data, knot);
                    }
                    knots.count += shape.knots.size();
                }
            },
            curve.shape);
        put(&records.data, record);
    }
    return { records, points, handles, weights, knots };
}

/**
 **/
boost::shared_ptr<Item> CurveCodec::read(const boost::json::object& payload, const Blobs& blobs, std::string* problem) const {
    if (!exactly(payload, { "renders", "width", "resolution" }, problem)) {
        return nullptr;
    }
    Curve::Style style;
    float resolution = 0.0F;
    if (!payload.at("renders").is_bool() || !numbers(payload.at("width"), 1, &style.width) ||
        !numbers(payload.at("resolution"), 1, &resolution)) {
        *problem = "its style is not a boolean and a finite width, or its resolution is not a number";
        return nullptr;
    }
    style.renders = payload.at("renders").get_bool();
    if (!style.valid()) {
        *problem = "its width is not above zero";
        return nullptr;
    }
    if (resolution < 1.0F || resolution > MOST_PIECES || std::trunc(resolution) != resolution) {
        *problem = "its resolution is not a whole number from 1 to 1024";
        return nullptr;
    }

    const boost::shared_ptr<v3d::curve::Curves> geometry = boost::make_shared<v3d::curve::Curves>();
    geometry->resolution(static_cast<unsigned int>(resolution));
    if (!decode(blobs, geometry.get(), problem)) {
        return nullptr;
    }
    const boost::shared_ptr<Curve> curve = boost::make_shared<Curve>(geometry);
    curve->style(style);
    return curve;
}

};  // namespace v3d::editor
