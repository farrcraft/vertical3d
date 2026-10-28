/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/curve/Curve.h>
#include <vertical3d/src/manipulator/Target.h>

#include <string>
#include <vector>

#include "Command.h"

#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 * One gesture of a manipulator on components. It stores where the vertices it moved were, and
 * the normals of the faces around them, when the handle was grabbed and when it was let go. It
 * also stores the curves whose control points or handles it moved.
 *
 * Undo and redo write the stored positions and normals back rather than applying the
 * inverse of the motion, so they restore both exactly. A curve is stored whole, so the types a
 * moved handle was given are restored with its place. The objects' transforms are not part of
 * the record, because a component move leaves them alone. A gesture on components in several
 * objects is one command.
 **/
class VertexCommand final : public Command {
 public:
    /**
     * Where one object's vertices are, and the normals of the faces around them, in the
     * order its Target::Part lists them.
     **/
    struct Shape {
        std::vector<glm::vec3> points;
        std::vector<glm::vec3> normals;

        /**
         **/
        static Shape of(const Target::Part& part);

        /**
         * Write the shape back into the part's mesh.
         **/
        void applyTo(const Target::Part& part) const;

        /**
         * @return whether every point is where the other's is, to within a tolerance. The
         *         normals follow the points
         **/
        bool same(const Shape& other) const;
    };

    /** One object's vertices, and where they were before the change and after it. **/
    struct Change {
        Target::Part part;
        Shape before;
        Shape after;
    };

    /**
     * Every curve of one curve item, in order.
     **/
    struct Outline {
        std::vector<v3d::curve::Curve> curves;

        /**
         **/
        static Outline of(const Target::CurvePart& part);

        /**
         * Write the curves back into the part's geometry.
         **/
        void applyTo(const Target::CurvePart& part) const;

        /**
         * @return whether every curve has the other's type and handle types, and every point
         *         and handle is where the other's is, to within a tolerance
         **/
        bool same(const Outline& other) const;
    };

    /** One curve item's curves, before the change and after it. **/
    struct CurveChange {
        Target::CurvePart part;
        Outline before;
        Outline after;
    };

    /**
     * @param curves the curve items the gesture changed, which may be none
     * @param mode which manipulator made the change, which is the name it reports
     **/
    VertexCommand(const std::vector<Change>& changes, const std::vector<CurveChange>& curves, const std::string& mode);

    void undo() override;
    void redo() override;
    std::string name() const override;

 private:
    std::vector<Change> changes_;
    std::vector<CurveChange> curves_;
    std::string mode_;
};

};  // namespace v3d::editor
