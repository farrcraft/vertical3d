/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "VertexCommand.h"

#include <api/brep/BRep.h>
#include <api/brep/Face.h>
#include <api/brep/Index.h>
#include <api/brep/Vertex.h>
#include <vertical3d/src/manipulator/Target.h>

#include <cstddef>
#include <string>
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

};  // namespace

/**
 **/
VertexCommand::Shape VertexCommand::Shape::of(const Target::Part& part) {
    Shape shape;
    if (!part.mesh) {
        return shape;
    }
    for (const v3d::brep::Index vertex : part.vertices) {
        shape.points.push_back(part.mesh->vertex(vertex)->point());
    }
    for (const v3d::brep::Index face : part.faces) {
        shape.normals.push_back(part.mesh->face(face)->normal());
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
        part.mesh->vertex(part.vertices[index])->point(points[index]);
    }
    for (std::size_t index = 0; index < part.faces.size() && index < normals.size(); index++) {
        part.mesh->face(part.faces[index])->normal(normals[index]);
    }
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
VertexCommand::VertexCommand(const std::vector<Change>& changes, const std::string& mode) :
    changes_(changes),
    mode_(mode) {
}

/**
 **/
void VertexCommand::undo() {
    for (const Change& change : changes_) {
        change.before.applyTo(change.part);
    }
}

/**
 **/
void VertexCommand::redo() {
    for (const Change& change : changes_) {
        change.after.applyTo(change.part);
    }
}

/**
 **/
std::string VertexCommand::name() const {
    return mode_;
}

};  // namespace v3d::editor
