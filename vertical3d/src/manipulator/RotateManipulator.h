/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Manipulator.h"

#include "../../../api/brep/BRep.h"
#include "../../../api/render/realtime/LineCanvas.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 * Turns the selection: a ring per axis, about the object's origin.
 *
 * An axis ring turns by however far the cursor swept round the origin on screen, which
 * is the gesture the ring invites. The centre handle tumbles the object about the
 * camera's own axes instead.
 **/
class RotateManipulator final : public Manipulator {
 public:
    RotateManipulator();

    // manipulator overrides
    bool grab(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
        const glm::vec2& cursor, Axis* axis) const override;
    void draw(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
        v3d::render::realtime::LineCanvas* canvas) const override;
    void apply(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
        const glm::vec2& from, const glm::vec2& to) const override;

 private:
    /**
     * How far the cursor swept round the manipulator's origin, in radians about a
     * handle's axis.
     *
     * Measured on the screen rather than in the world, because the ring is what the
     * gesture follows. Clip space points y down, so a rotation about an axis running
     * into the screen sweeps the opposite way round the origin from one about an axis
     * coming out of it, and the sign follows from which it is.
     **/
    /**
     * How close the cursor comes to the ring about one axis, in pixels.
     *
     * A ring is not one segment, so it is tested as the run of segments it is drawn
     * as. Nothing within tolerance of any of them, and nothing that projects at all,
     * both come back negative.
     **/
    float ringDistance(const Placement& placement, const glm::vec3& unit,
        const ViewPort& view, const glm::vec2& cursor) const;

    float swept(const ViewPort& view, const Placement& placement, Axis axis,
        const glm::vec2& from, const glm::vec2& to) const;
};

};  // namespace v3d::editor
