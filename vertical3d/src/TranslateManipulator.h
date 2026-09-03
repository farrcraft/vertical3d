/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Manipulator.h"

#include "../../api/brep/BRep.h"
#include "../../api/render/realtime/LineCanvas.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

    /**
     * Moves the selection: an arrow per axis, and a box at the centre.
     *
     * An axis handle moves along that axis by the drag's component along it. The centre
     * handle moves in the plane of the screen, which is what a gesture with no axis in mind
     * means.
     **/
    class TranslateManipulator final : public Manipulator {
     public:
        TranslateManipulator();

        // manipulator overrides
        void draw(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
            v3d::render::realtime::LineCanvas* canvas) const override;
        void apply(const boost::shared_ptr<v3d::brep::BRep>& mesh, const ViewPort& view,
            const glm::vec2& from, const glm::vec2& to) const override;
    };

};  // namespace v3d::editor
