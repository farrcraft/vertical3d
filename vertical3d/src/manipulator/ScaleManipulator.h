/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/render/realtime/LineCanvas.h>

#include "Manipulator.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 * Resizes the selection: a box on a stalk per axis, and a box at the centre.
 *
 * Dragging a handle out to twice its length doubles the object along that axis, so the
 * gesture is proportional rather than a number of world units - a small object and a
 * large one resize at the same rate under the cursor. The centre handle scales all
 * three axes together.
 *
 * The handles are always the object's own axes: a scale is a vector in those axes and
 * a global one could not be written back through the transform.
 **/
class ScaleManipulator final : public Manipulator {
 public:
    ScaleManipulator();

    // manipulator overrides
    void draw(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
        v3d::render::realtime::LineCanvas* canvas) const override;
    void apply(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
        const glm::vec2& from, const glm::vec2& to) const override;

 protected:
    /**
     * Always local, whatever the coordinate space says.
     **/
    Space effective() const noexcept override;
};

};  // namespace v3d::editor
