/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vertical3d/src/manipulator/Target.h>

#include <string>
#include <vector>

#include "Command.h"

#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 * One gesture of a manipulator on components: where the vertices it moved were, and the
 * normals of the faces around them, when the handle was grabbed and when it was let go.
 *
 * Undo and redo write the stored positions and normals back rather than applying the
 * inverse of the motion, so they restore both exactly. The objects' transforms are not part
 * of the record, because a component move leaves them alone. A gesture on components in
 * several objects is one command.
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
     * @param mode which manipulator made the change, which is the name it reports
     **/
    VertexCommand(const std::vector<Change>& changes, const std::string& mode);

    void undo() override;
    void redo() override;
    std::string name() const override;

 private:
    std::vector<Change> changes_;
    std::string mode_;
};

};  // namespace v3d::editor
