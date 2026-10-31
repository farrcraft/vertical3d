/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/render/realtime/canvas/Line.h>

#include "Manipulator.h"
#include "Screen.h"
#include "Target.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 * Turns the selection: a ring per axis, about the object's origin.
 *
 * An axis ring turns by however far the cursor swept round the origin on screen. The
 * centre handle tumbles the object about the camera's own axes instead.
 **/
class RotateManipulator final : public Manipulator {
 public:
    RotateManipulator();

    // manipulator overrides
    bool grab(const Target& target, const Screen& screen,
        const glm::vec2& cursor, Axis* axis) const override;
    void draw(const Target& target, const Screen& screen,
        v3d::render::realtime::canvas::Line* canvas) const override;

 protected:
    bool measure(const Placement& seat, const Screen& screen, const glm::vec2& from, const glm::vec2& to,
        Motion* motion) const override;

 private:
    /**
     * How many segments a ring is drawn with.
     **/
    static constexpr unsigned int ringSides = 48;
    /**
     * The centre handle's box, as a fraction of a ring's radius.
     **/
    static constexpr float centreSize = 0.06F;
    /**
     * Radians a pixel, for the tumble the centre handle drives. A drag the width of a
     * handle turns the object about a third of a turn.
     **/
    static constexpr float tumbleRate = 0.0075F;
    /**
     * How face on a ring has to be before a click can be meant for it, as the cosine of
     * the angle between its axis and the direction of view.
     *
     * A ring seen edge on projects to a line through the middle of the manipulator. The
     * line crosses every other handle. Its ends land exactly on the rim of the ring
     * facing the camera, so a click meant for that rim would be taken by a ring whose
     * shape the user cannot even see. An axis view is the case that matters. Two of the
     * three rings are exactly edge on there, and only the third can be turned.
     **/
    static constexpr float faceOn = 0.15F;
    /**
     * An angle brought back into the half turn either side of zero, so a sweep across
     * the line where atan2 wraps is a small rotation rather than a whole turn.
     **/
    static float wrap(float angle);

    /**
     * How close the cursor comes to the ring about one axis, in pixels.
     *
     * A ring is not one segment, so it is tested as the run of segments it is drawn
     * as. The result is negative when no segment is within tolerance, or when no
     * segment projects onto the screen at all.
     **/
    static float ringDistance(const Placement& placement, const glm::vec3& unit,
        const Screen& screen, const glm::vec2& cursor);

    /**
     * How far the cursor swept round the manipulator's origin, in radians about a
     * handle's axis.
     *
     * Measured on the screen rather than in the world, because the gesture follows the
     * ring as drawn. Clip space points y down. A rotation about an axis running into
     * the screen sweeps the opposite way round the origin from one about an axis coming
     * out of it, and the sign follows the axis's direction.
     **/
    static float swept(const Screen& screen, const Placement& placement, Axis axis,
        const glm::vec2& from, const glm::vec2& to);
};

};  // namespace v3d::editor
