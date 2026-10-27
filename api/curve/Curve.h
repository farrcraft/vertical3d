/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::curve {

/**
 * A curve of straight lines through its points.
 **/
struct Poly final {
    std::vector<glm::vec3> points;

    bool operator==(const Poly& other) const = default;
};

/**
 * How a Bezier point's handle is placed.
 *
 * Automatic and Vector handles are placed from the points around them, and settle() places
 * them again whenever a point moves. Aligned and Free handles stay where they were put.
 **/
enum class Handle : std::uint8_t {
    Free,       /**< where it was put, in any direction **/
    Aligned,    /**< where it was put, in line with the point's other handle **/
    Vector,     /**< a third of the way toward the neighbouring point on its side **/
    Automatic   /**< along the line through the neighbours, so the curve is smooth there **/
};

/**
 * A point a Bezier curve passes through, and the two handles that shape it.
 *
 * left shapes the segment that arrives at the point, and right the segment that leaves it.
 **/
struct BezierPoint final {
    glm::vec3 position{ 0.0F };
    glm::vec3 left{ 0.0F };
    glm::vec3 right{ 0.0F };
    Handle leftType = Handle::Automatic;
    Handle rightType = Handle::Automatic;

    bool operator==(const BezierPoint& other) const = default;
};

/**
 * A curve of cubic segments between points, each shaped by the handles at its two ends.
 **/
struct Bezier final {
    std::vector<BezierPoint> points;

    bool operator==(const Bezier& other) const = default;
};

/**
 * A smooth curve through every one of its points, each segment shaped by the points either side
 * of it.
 *
 * It is uniform Catmull-Rom. An open curve's two end segments treat the end point as its own
 * neighbour, so the curve starts and ends on its first and last points.
 **/
struct CatmullRom final {
    std::vector<glm::vec3> points;

    bool operator==(const CatmullRom& other) const = default;
};

/**
 * A non-uniform rational B-spline.
 *
 * order is the degree plus one, so 2 is straight lines and 4 is cubic. weights is either empty,
 * when every point weighs 1, or one positive weight per point. knots is either empty or holds
 * the number of points plus the order, never falling.
 *
 * Empty knots are worked out. An open curve's are uniform and clamped, so the curve starts on
 * its first point and ends on its last. A closed curve's are uniform, and a closed curve cannot
 * be given knots of its own.
 **/
struct Nurbs final {
    static constexpr unsigned int CUBIC = 4;

    std::vector<glm::vec3> points;
    std::vector<float> weights;
    std::vector<float> knots;
    unsigned int order = CUBIC;

    bool operator==(const Nurbs& other) const = default;
};

/**
 * One curve of any type.
 *
 * A closed curve joins its last point back to its first. Its points are in the space of the
 * item that holds it.
 **/
struct Curve final {
    std::variant<Poly, Bezier, CatmullRom, Nurbs> shape;
    bool closed = false;

    bool operator==(const Curve& other) const = default;
};

/**
 * Whether a curve can be evaluated.
 *
 * A curve needs two points, and a NURBS curve as many as its order, which is at least 2. Every
 * point and handle is finite. A NURBS curve's weights and knots are as Nurbs describes, and its
 * knots leave it a domain longer than nothing.
 *
 * @return why the curve cannot be evaluated, or empty when it can
 **/
std::string problem(const Curve& curve);

/**
 * The points a curve passes through, for drawing it and picking it.
 *
 * Each segment is cut into resolution pieces, evenly in its parameter. A poly curve gives its
 * points as they are. A closed curve ends with its first point again. A NURBS segment is a span
 * between two knots, and spans of no length are skipped.
 *
 * @param curve one that problem() finds nothing wrong with
 * @param resolution at least 1
 **/
std::vector<glm::vec3> evaluate(const Curve& curve, unsigned int resolution);

/**
 * Place the Automatic and Vector handles of a Bezier curve from its points.
 *
 * An automatic handle lies along the line from the point before to the point after, a third of
 * the distance to the neighbour on its side. A vector handle points at the neighbour on its
 * side, a third of the way there. At an open curve's ends the handle that faces away from the
 * curve mirrors the one that faces into it, and an automatic handle there points at the
 * neighbour like a vector one. Aligned and Free handles are left where they are.
 **/
void settle(Bezier* bezier, bool closed);

/**
 * Which handle of a Bezier point.
 **/
enum class Side : std::uint8_t { Left, Right };

/**
 * Move one handle of a Bezier point, as a user dragging it does.
 *
 * A moved Vector handle becomes Free. A moved Automatic handle becomes Aligned, and so does the
 * point's other handle when it is Automatic. When the moved handle is Aligned and so is the
 * other, the other turns to lie on the far side of the point, in line, and keeps its length.
 *
 * @param point an index into bezier->points
 **/
void moveHandle(Bezier* bezier, std::size_t point, Side side, const glm::vec3& position);

};  // namespace v3d::curve
