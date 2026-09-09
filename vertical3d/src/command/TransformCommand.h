/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/dag/Transform.h>

#include <string>

#include "Command.h"

#include <boost/shared_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

/**
 * Where a mesh is, all three parts of it at once.
 *
 * A manipulator writes one of translation, rotation and scale per ADR-0015, but a
 * gesture is undone by putting the whole placement back: recording only the part that
 * moved would leave the record depending on which manipulator made it.
 **/
struct Placement {
    glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
    glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    /**
     **/
    static Placement of(const v3d::dag::Transform& transform);

    /**
     **/
    void applyTo(v3d::dag::Transform* transform) const;

    /**
     * @return whether the two are the same placement to within a tolerance
     *
     * A gesture that measured nothing still composes a rotation and adds a zero offset,
     * so the two ends of it differ in the last bits without the object having moved.
     **/
    bool same(const Placement& other) const;
};

/**
 * One gesture of a manipulator: where the mesh was when the handle was grabbed, and
 * where it was when the handle was let go.
 *
 * The drag itself has already been applied a motion event at a time - a gesture cannot
 * wait for its own end to show what it is doing - so the command records the two ends
 * of it rather than the offset, per ADR-0016.
 **/
class TransformCommand final : public Command {
 public:
    /**
     * @param mode which manipulator made the change, which is the name it reports
     **/
    TransformCommand(const boost::shared_ptr<v3d::brep::BRep>& mesh,
        const Placement& before,
        const Placement& after,
        const std::string& mode);

    void undo() override;
    void redo() override;
    std::string name() const override;

 private:
    boost::shared_ptr<v3d::brep::BRep> mesh_;
    Placement before_;
    Placement after_;
    std::string mode_;
};

};  // namespace v3d::editor
