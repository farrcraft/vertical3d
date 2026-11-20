/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "Bezier.h"
#include "CatmullRom.h"
#include "Nurbs.h"
#include "Poly.h"

#include <glm/vec3.hpp>

/**
 * Poly, Bezier, Catmull-Rom and NURBS curves, their evaluation, and their conversion to the basis
 * curves a renderer takes.
 **/
namespace v3d::curve {

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
 * @param bezier the curve, changed in place
 * @param point an index into bezier->points
 * @param side which of the point's handles moves
 * @param position where the handle moves to, in the same space as the point
 **/
void moveHandle(Bezier* bezier, std::size_t point, Side side, const glm::vec3& position);

};  // namespace v3d::curve
