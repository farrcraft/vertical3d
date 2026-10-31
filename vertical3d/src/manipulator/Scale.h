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

namespace v3d::editor::manipulator {

/**
 * Resizes the selection: a box on a stalk per axis, and a box at the centre.
 *
 * Dragging a handle out to twice its length doubles the object along that axis. The
 * gesture is proportional, not a number of world units, so a small object and a large one
 * resize at the same rate under the cursor. The centre handle scales all three axes
 * together.
 *
 * The handles always follow the object's own axes. A scale is a vector in those axes, and
 * a scale along global axes could not be written back through the transform.
 **/
class Scale final : public Manipulator {
 public:
    Scale();

    // manipulator overrides
    void draw(const Target& target, const Screen& screen,
        v3d::render::realtime::canvas::Line* canvas) const override;

 protected:
    bool measure(const Placement& seat, const Screen& screen, const glm::vec2& from, const glm::vec2& to,
        Motion* motion) const override;

    /**
     * Always local, whatever the coordinate space says.
     **/
    Space effective() const noexcept override;

 private:
    /**
     * The box at the tip of a handle, and the one at the centre, as a fraction of a
     * handle's length.
     **/
    static constexpr float tipSize = 0.07F;
    static constexpr float centreSize = 0.06F;
};

};  // namespace v3d::editor::manipulator
