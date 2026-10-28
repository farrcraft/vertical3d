/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "VertexCommand.h"

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/Index.h>
#include <api/brep/Vertex.h>
#include <api/curve/Curve.h>
#include <api/curve/Curves.h>
#include <vertical3d/src/manipulator/Target.h>
#include <vertical3d/src/scene/CurveParts.h>

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

namespace {

/**
 * How far apart two points may be and still be the same point. A vertex is moved through
 * its object's matrix and back, so a gesture that measured nothing still changes the last
 * bits of it.
 **/
constexpr float tolerance = 1e-5F;

/**
 * Whether two curves are of one type and closed alike, with their points, handles and handle
 * types the same, to within the tolerance. A gesture moves no NURBS weight or knot, so those
 * are not compared.
 **/
bool sameCurve(const v3d::curve::Curve& one, const v3d::curve::Curve& other) {
    if (one.shape.index() != other.shape.index() || one.closed != other.closed) {
        return false;
    }
    const std::size_t count = controlCount(one);
    if (count != controlCount(other)) {
        return false;
    }
    for (std::size_t point = 0; point < count; point++) {
        if (glm::distance(controlPoint(one, point), controlPoint(other, point)) > tolerance) {
            return false;
        }
    }
    const auto* bezier = std::get_if<v3d::curve::Bezier>(&one.shape);
    if (bezier == nullptr) {
        return true;
    }
    const std::vector<v3d::curve::BezierPoint>& others = std::get<v3d::curve::Bezier>(other.shape).points;
    for (std::size_t point = 0; point < count; point++) {
        const v3d::curve::BezierPoint& mine = bezier->points[point];
        const v3d::curve::BezierPoint& theirs = others[point];
        if (mine.leftType != theirs.leftType || mine.rightType != theirs.rightType ||
            glm::distance(mine.left, theirs.left) > tolerance || glm::distance(mine.right, theirs.right) > tolerance) {
            return false;
        }
    }
    return true;
}

};  // namespace

/**
 **/
VertexCommand::Shape VertexCommand::Shape::of(const Target::Part& part) {
    Shape shape;
    if (!part.mesh) {
        return shape;
    }
    for (const v3d::brep::Index vertex : part.vertices) {
        shape.points.push_back(part.mesh->geometry()->vertex(vertex)->point());
    }
    for (const v3d::brep::Index face : part.faces) {
        shape.normals.push_back(part.mesh->geometry()->face(face)->normal());
    }
    return shape;
}

/**
 **/
void VertexCommand::Shape::applyTo(const Target::Part& part) const {
    if (!part.mesh) {
        return;
    }
    for (std::size_t index = 0; index < part.vertices.size() && index < points.size(); index++) {
        part.mesh->geometry()->vertex(part.vertices[index])->point(points[index]);
    }
    for (std::size_t index = 0; index < part.faces.size() && index < normals.size(); index++) {
        part.mesh->geometry()->face(part.faces[index])->normal(normals[index]);
    }
    // the parts were changed in place, which the mesh does not see for itself
    part.mesh->geometry()->touch();
}

/**
 **/
bool VertexCommand::Shape::same(const Shape& other) const {
    if (points.size() != other.points.size()) {
        return false;
    }
    for (std::size_t index = 0; index < points.size(); index++) {
        if (glm::distance(points[index], other.points[index]) > tolerance) {
            return false;
        }
    }
    return true;
}

/**
 **/
VertexCommand::Outline VertexCommand::Outline::of(const Target::CurvePart& part) {
    Outline outline;
    if (!part.curve) {
        return outline;
    }
    const v3d::curve::Curves& geometry = *part.curve->geometry();
    for (std::size_t index = 0; index < geometry.size(); index++) {
        outline.curves.push_back(geometry.curve(index));
    }
    return outline;
}

/**
 **/
void VertexCommand::Outline::applyTo(const Target::CurvePart& part) const {
    if (!part.curve) {
        return;
    }
    v3d::curve::Curves& geometry = *part.curve->geometry();
    for (std::size_t index = 0; index < geometry.size() && index < curves.size(); index++) {
        // every curve stored here was held by the geometry once, so none is refused
        if (geometry.curve(index) != curves[index]) {
            static_cast<void>(geometry.set(index, curves[index]));
        }
    }
}

/**
 **/
bool VertexCommand::Outline::same(const Outline& other) const {
    if (curves.size() != other.curves.size()) {
        return false;
    }
    for (std::size_t index = 0; index < curves.size(); index++) {
        if (!sameCurve(curves[index], other.curves[index])) {
            return false;
        }
    }
    return true;
}

/**
 **/
VertexCommand::VertexCommand(const std::vector<Change>& changes, const std::vector<CurveChange>& curves,
    const std::string& mode) :
    changes_(changes),
    curves_(curves),
    mode_(mode) {
}

/**
 **/
void VertexCommand::undo() {
    for (const Change& change : changes_) {
        change.before.applyTo(change.part);
    }
    for (const CurveChange& change : curves_) {
        change.before.applyTo(change.part);
    }
}

/**
 **/
void VertexCommand::redo() {
    for (const Change& change : changes_) {
        change.after.applyTo(change.part);
    }
    for (const CurveChange& change : curves_) {
        change.after.applyTo(change.part);
    }
}

/**
 **/
std::string VertexCommand::name() const {
    return mode_;
}

};  // namespace v3d::editor
